/**
 * Copyright (c) 2003-2011, Pavel Kraynyukhov.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * $Id:$
 * $Author: pk$
 * File:   WorkerThread.h
 *
 **/

#ifndef __WORKERTHREAD_H__
#    define	__WORKERTHREAD_H__

#    include <abstract/Runnable.h>
#    include <sys/CancelableThread.h>
#    include <abstract/IController.h>
#    include <sys/SyncLock.h>
#    include <sys/Semaphore.h>
#    include <abstract/IView.h>

namespace itc
{
    typedef abstract::IRunnable RunnableInterface;
    typedef abstract::IController ControllerInterface;

    enum WorkerState
    {
        BUSY, READY, CANCEL, KO, FINISH
    };

    struct WorkerStats
    {
        pthread_t mThreadID;
        WorkerState mWorkerState;

        explicit WorkerStats(const WorkerState& pState = READY, const mThreadID& = 0)
        {
        }

        WorkerStats(const WorkerStats & ref) : mThreadID(ref.mThreadID), mWorkerState(ref.mWorkerState)
        {
        }

        const WorkerStats & operator=(const WorkerStats & ref)
        {
            mThreadID(ref.mThreadID);
            mWorkerState(ref.mWorkerState);
        }
    };

    typedef abstract::IView<WorkerStats> ViewInterface;

    class WorkerThread : public RunnableInterface, ControllerInterface
    {
    protected:

        void execute()
        {
            do
            {
                try
                {
                    mNext.wait();
                    if (getState() == READY)
                    {
                        setState(BUSY);
                        if (mTask) mTask->execute();
                        setState(READY);
                    }
                }
                catch (const std::exception& e)
                {
                    setState(NOK);
                    getLog()->error(__FILE__, __LINE__, "Unexpected exception: %s", e.what());
                    throw TITCException<exceptions::WorkerKOException > (EINVAL);
                }
            }
            while (((const WorkerState ret=getState()) != FINISH) && (ret != KO));
        }

    public:

        explicit WorkerThread(const ViewInterface*& ref)
        {

        }

        inline const WorkerState& getState()
        {
            sys::SyncLock sync(mMutex);
            return mStats.mWorkerState;
        }

        inline void assignTask(const RunnableInterface*& pTask, const pthread_t& pTID)
        {

        }

    private:
        friend class WorkersController;
        sys::Mutex mMutex;
        sys::Semaphore mNext;
        WorkerStats mStats;
        ViewInterface* mThreadPool;


        inline void setThreadID(const pthread_t& pTID)
        {
            sys::SyncLock sync(mMutex);
            mStats.mThreadID = pTID;
        }

        inline void setState(const WorkerState& pState)
        {
            sys::SyncLock sync(mMutex);
            mStats.mWorkerState = pState;
        }
    };
}

#endif	/* __WORKERTHREAD_H__ */

