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
 * $Id: WorkflowStorableState.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef WORKFLOWSTORABLESTATE_H_
#define WORKFLOWSTORABLESTATE_H_

#include <memory>

#include <abstract/IWorkflowState.h>

typedef std::shared_ptr<itc::abstract::IWorkflowState> IWorkflowStateSPType;
typedef std::weak_ptr<itc::abstract::IWorkflowState> IWorkflowStateWPType;

namespace itc {
    namespace workflow {

        /**
         * This class makes possible to store any derivates of IWorkflowState
         * (thier pointers actually) into STL containers.
         * To be pointer-safe, do not give to constrctor the addresses of any
         * stack-allocated IWorkflowState derivates and externally controlled
         * pointers too.
         * Right way to use this class is to create WorkflowStorableState on stack
         * and allocate new instance of IWorkflowState derivates within constructor.
         * The instance of IWorkflowState derivate will be deallocated on
         * WorkflowStorableState destruction.
         *
         * Examples of right use:
         * 	std::set<WorkflowStorableState> aWFState;
         * 	aWFState.insert(new Derivate_of_IWorkflowState(some arguments if any));
         *
         *  or
         *
         *  WorkflowStorableState test(
         * 	new Derivate_of_IWorkflowState(some arguments if any)
         *  );
         *
         * Do not create  WorkflowStorableState instances, those sharing instances of
         * IWorkflowState derivates because it is thread unsafe !
         * Thread safety is not added to WorkflowStorableState because of unnecessary
         * overhead. Following example is realy wrong usage:
         * 
         * Example of wrong use:
         * 
         * shared_ptr<Derivate_of_IWorkflowState> test(new Derivate_of_IWorkflowState(some arguments if any));
         * WorkflowStorableState wrong(test.get());
         * 
         * Never use such construction.
         * 
         * 
         **/
        class WorkflowStorableState {
        private:
            IWorkflowStateSPType mWorkflowStateSPtr;

        public:

            explicit WorkflowStorableState(::itc::abstract::IWorkflowState* ptr) : mWorkflowStateSPtr(ptr) {
            }

            explicit WorkflowStorableState(const WorkflowStorableState& ref)
            : mWorkflowStateSPtr(ref.mWorkflowStateSPtr) {
            }

            inline bool operator==(const WorkflowStorableState& ref) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();
                ::itc::abstract::IWorkflowState* aRPtr = ref.mWorkflowStateSPtr.get();

                return ( aLPtr && aRPtr ? (aLPtr->getWorkflowStateId() == aRPtr->getWorkflowStateId()) : false);
            }

            inline bool operator>(const WorkflowStorableState& ref) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();
                ::itc::abstract::IWorkflowState* aRPtr = ref.mWorkflowStateSPtr.get();

                return ( aLPtr && aRPtr ? (aLPtr->getWorkflowStateId() > aRPtr->getWorkflowStateId()) : false);
            }

            inline bool operator<(const WorkflowStorableState& ref) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();
                ::itc::abstract::IWorkflowState* aRPtr = ref.mWorkflowStateSPtr.get();

                return ( aLPtr && aRPtr ? (aLPtr->getWorkflowStateId() < aRPtr->getWorkflowStateId()) : false);
            }

            inline IWorkflowStateSPType getWFStateSharedPtr() {
                return mWorkflowStateSPtr;
            }

            inline bool operator==(const uint32_t id) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();

                return ( aLPtr ? (aLPtr->getWorkflowStateId() == id) : false);
            }

            inline bool operator>(const uint32_t id) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();

                return ( aLPtr ? (aLPtr->getWorkflowStateId() > id) : false);
            }

            inline bool operator<(const uint32_t id) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();

                return ( aLPtr ? (aLPtr->getWorkflowStateId() < id) : false);
            }

            inline bool operator!=(const uint32_t id) const {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();

                return ( aLPtr ? (aLPtr->getWorkflowStateId() != id) : false);
            }

            inline operator uint32_t() {
                ::itc::abstract::IWorkflowState* aLPtr = mWorkflowStateSPtr.get();

                return ( aLPtr ? aLPtr->getWorkflowStateId() : ::itc::workflow::err);
            }
        };
    }
}

#endif /*WORKFLOWSTORABLESTATE_H_*/
