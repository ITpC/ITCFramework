/**
 * Copyright (c) 2007, Pavel Kraynyukhov.
 *  
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted under the terms of the General Public License version 2
 * (GPLv2), provided that the above copyright notice and this paragraph and the
 * following two paragraphs and the "LICENSE" file appear in all modified or
 * unmodified copies of the software "AS IS" and without any changes.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 * 
 * 
 * $Id: WorkflowControl.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __WORKFLOWCONTROL_H__
#define __WORKFLOWCONTROL_H__
#include <map>

#include <abstract/IMessageListener.h>
#include <abstract/IWorkflowState.h>
#include <WorkflowEntity.h>
#include <WorkflowStorableState.h>
#include <sys/synclock.h>
#include <TSLog.h>
#include <ITCException.h>
#include <ThreadSafeLocalQueue.h>

typedef std::map<uint32_t, itc::workflow::WorkflowStorableState> WFStorableStatesMap;
typedef WFStorableStatesMap::iterator WFStorableStatesSetIterator;

namespace itc {
    namespace workflow {
        using namespace ::itc::sys;

        template <typename WFEntity> class WorkflowControl 
        : public ::itc::IMessageListener<WFEntity, ::itc::ThreadSafeLocalQueue>
        {
        public:
            typedef typename itc::IMessageListener<WFEntity, itc::ThreadSafeLocalQueue> IMLType;
            typedef typename IMLType::TQueueImpl QueueType;
            typedef typename IMLType::QueueSharedPtr QueueSharedPtrType;
            typedef typename IMLType::QueueWeakPtr QueueWeakPtrType;
            typedef typename itc::abstract::IWorkflowState* IWFStatePtr;
            typedef QueueType* QueuePtrType;

        private:
            WFStorableStatesMap mWFStatesMap;
            std::map<uint32_t, uint32_t> mTransitionsMap;
            QueueSharedPtrType mDeadLetterQueue;
            bool mDLQueueValid;
            bool mWFQueueValid;
            std::mutex mMutex;
        public:

            explicit WorkflowControl(
                    QueueSharedPtrType& pQueue,
                    QueueSharedPtrType& pDeadLetterQueue,
                    WFStorableStatesMap& ref,
                    const std::map<uint32_t, uint32_t>& pTransMap
                    )
            : IMessageListener<WFEntity, ThreadSafeLocalQueue>(pQueue),
            mWFStatesMap(ref), mTransitionsMap(pTransMap),
            mDeadLetterQueue(pDeadLetterQueue),
            mDLQueueValid(true), mWFQueueValid(true) {
            }

            void onMessage(WFEntity& msg) {
                try {
                    SyncLock synchronize(mMutex); // protecting WorkflowState pointers (they are shared).

                    if (mDLQueueValid && mWFQueueValid) {
                        if (msg.getNextStateId() == workflow::end) {
                            return;
                        }

                        WFStorableStatesSetIterator it = mWFStatesMap.find(msg.getNextStateId());
                        if (it == mWFStatesMap.end()) {
                            WFStorableStatesSetIterator errIt = mWFStatesMap.find(msg.getErrorStateId());

                            if (errIt != mWFStatesMap.end()) {
                                if (::itc::abstract::IWorkflowState * aWFState = errIt->second.getWFStateSharedPtr().get()) {
                                    ::itc::getLog()->debug(
                                            __FILE__, __LINE__, "Before call of StateTask, WFControl has address: %x", this
                                            );
                                    aWFState->StateTask(&msg);
                                    ::itc::getLog()->debug(
                                            __FILE__, __LINE__, "After call of StateTask, WFControl has address: %x", this
                                            );
                                } else {
                                    ::itc::getLog()->fatal(
                                            __FILE__, __LINE__, "No error state provided, got NULL instead of state address"
                                            );
                                }
                            } else {
                                if (QueuePtrType aQueueAddr = mDeadLetterQueue.get()) {
                                    if (aQueueAddr->send(msg) == false) {
                                        mWFQueueValid = false;
                                        ::itc::getLog()->error(
                                                __FILE__, __LINE__,
                                                "Deadletter queue is not available, use persistent queue type if you do not wish to drop messages"
                                                );
                                    }
                                } else {
                                    ::itc::getLog()->error(
                                            __FILE__, __LINE__,
                                            "Deadletter queue is not available, use persistent queue type if you do not wish to drop messages"
                                            );
                                    mDLQueueValid = false;
                                }
                            }
                        } else {

                            if (::itc::abstract::IWorkflowState * aWFState = (it->second).getWFStateSharedPtr().get()) {
                                // do the task that related to workflow step.
                                aWFState->StateTask(&msg);

                                // prepare to transition.
                                uint32_t aCurrentCommitedState = msg.getNextStateId();

                                std::map<uint32_t, uint32_t>::iterator transIt = mTransitionsMap.find(aCurrentCommitedState);

                                uint32_t aNextState = workflow::end;
                                uint32_t aPrevState = workflow::end;
                                if (transIt == mTransitionsMap.end()) // is there no transition available ?
                                {
                                    // transition to deadletter queue
                                    return;

                                } else {
                                    aNextState = transIt->second;
                                    aPrevState = aCurrentCommitedState;
                                }


                                // we do not want to push the workflow message to the workflow queue
                                // if its transition points to the end.
                                if (aNextState == end) {
                                    return;
                                }

                                msg.setPrevStateId(aPrevState);
                                msg.setNextStateId(aNextState);

                                if (QueuePtrType aQueueAddr = this->getQueueWeakPtr().lock().get()) {
                                    if (aQueueAddr->send(msg) == false) {
                                        mWFQueueValid = false;
                                        ::itc::getLog()->error(
                                                __FILE__, __LINE__,
                                                "Queue is no more accessible, use persistent queue type if you do not wish to drop messages"
                                                );
                                    }
                                } else // the workflow queue is destroyed
                                {
                                    ::itc::getLog()->error(
                                            __FILE__, __LINE__,
                                            "Queue is no more accessible, use persistent queue type if you do not wish to drop messages"
                                            );

                                    mWFQueueValid = false;
                                }
                            } else {
                                ::itc::getLog()->fatal(
                                        __FILE__, __LINE__,
                                        "Internal states transition map is modified. This is normally not allowed, make interface more safe !!!"
                                        );
                            }
                        }
                    } else {
                        ::itc::getLog()->fatal(
                                __FILE__, __LINE__,
                                "Both queues (workflow and deadletter) are dead, so how do we get messages here ?"
                                );
                    }
                } catch (ITCException& e) {
                    ::itc::getLog()->fatal(
                            __FILE__, __LINE__, "Exception in WorkflowControl::onMessage(). Detailed: %s",
                            e.what()
                            );
                }
            }

            inline WFStorableStatesMap getWFStatesCopy() {
                SyncLock synchronize(mMutex); // propagate exception further if it case, no try-catch block here.
                WFStorableStatesMap tmp(mWFStatesMap);
                return tmp;
            }

            inline void onQueueDestroy() {
                SyncLock synchronize(mMutex);

                mDLQueueValid = false;
                mWFQueueValid = false;
                ::itc::getLog()->debug(__FILE__, __LINE__, "Workflow queue destroyed, listener address: %x", this);
            }

            inline void onCancel() {
                SyncLock synchronize(mMutex);

                mDLQueueValid = false;
                mWFQueueValid = false;
                ::itc::getLog()->debug(__FILE__, __LINE__, "cancellation called for listener at address: %x", this);
            }

            virtual ~WorkflowControl() {
                SyncLock synchronize(mMutex);
                mDLQueueValid = false;
                mWFQueueValid = false;
                ::itc::getLog()->debug(__FILE__, __LINE__, "destructor called for listener at address: %x", this);
            }
        };
    }
}

#endif /*__WORKFLOWCONTROL_H__*/
