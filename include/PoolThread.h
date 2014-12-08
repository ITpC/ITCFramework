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
 * $Id: PoolThread.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/
#ifndef CLEANABLETHREAD_H_
#    define CLEANABLETHREAD_H_

#    include <moemory>

#    include <InterfaceCheck.h>
#    include <abstract/Runnable.h>

#    include <sys/Thread.h>
#    include <sys/Semaphore.h>
#    include <abstract/Cleanable.h>
#    include <abstract/Runnable.h>

#    include <abstract/IController.h>

#    include <ITCException.h>

#    include <errno.h>



namespace itc
{

    enum ThreadState
    {
        INIT, WAIT, RUN, DONE, CANCELED
    };

    typedef std::pair<pthread_t, ThreadState> ModelType;
    typedef abstract::IView<ModelType> TMIView;
    typedef std::shared_ptr<TMIView> IViewSPtr;
    typedef std::shared_ptr<abstract::IRunnable> RunnableSPtr;

    class PoolThread : public Thread, abstract::Cleanable, IController<ModelType>
    {
    private:
        friend void* invoke(Thread*);
        friend void cleanup_handler(abstract::Cleanable*);

        Mutex mMutex;
        Semaphore mIteration;

        RunnableSPtr mRunnable;
        IViewSPtr mThreadPoller;

        ThreadState mState;
        bool mOK;

        inline abstract::IRunnable* getRunnable()
        {
            SyncLock sync(mMutex);
            return mRunnable.get();
        }

        inline void setState(const ThreadState& pState)
        {
            SyncLock sync(mMutex);
            mState = pState;
            notify(ModelType(getThreadId(), mState));
        }

        inline const bool isOK() const
        {
            SyncLock sync(mMutex);
            return mOK;
        }

        inline void setOK(const bool& val)
        {
            SyncLock sync(mMutex);
            mOK = val;
        }

        inline const ThreadState getState() const
        {
            SyncLock sync(mMutex);
            return mState;
        }

        inline abstract::IRunnable* getNextRunnable()
        {
            setState(WAIT);
            mIteration.wait();
            SyncLock sync(mMutex);
            return mRunnable.get();
        }

        inline void init()
        {
            setState(INIT);
            begin();
            SyncLock sync(mMutex);
            if (mRunnable.get())
            {
                mIteration.post();
            }
        }
    protected:

        inline void notify(const ModelType& ref)
        {
            if (PollerType * ptr = mThreadPoller.get())
            {
                ptr->update(ref);
            }
        }

    public:

        explicit PoolThread(const IView* pPoller = NULL)
        : Thread(), mRunnable(RunnableSPtr(0)), mThreadPoller(pPoller), mOK(true)
        {
            init();
        }

        explicit PoolThread(const IViewSPtr& pPoller = IViewSPtr(0))
        : Thread(), mRunnable(RunnableSPtr(0)), mThreadPoller(pPoller), mOK(true)
        {
            init();
        }

        /**
         * @brief sets the next runnable to execute. No queueing, jsut reset the member mRunnable
         * to new value if the state is WAIT, and throws an exception otherways.
         *
         * @param New task to execute, that type is subclassed from IRunnable
         *
         * @exception exceptions::Can_not_assign_runnable
         **/
        template <typename TRunnable> void setNextRunnable(const std::shared_ptr<TRunnable>& pRunnable)
        {
            typedef abstract::IRunnable RunnableInterface;
            STATIC_CHECKER3MSG(
                               CheckRelationship(
                                                 TRunnable, subclassof, RunnableInterface
                                                 ),
                               TRunnable, _is_not_a_subclass_of_, RunnableInterface
                               );

            if (isOk() && (getState() == WAIT))
            {
                SyncLock sync(mMutex);
                mRunnable = pRunnable;
                mIteration.post();
            }
            else
            {
                throw ITCException(EBUSY, exceptions::Can_not_assign_runnable);
            }
        }


    protected:

        inline void run()
        {
            pthread_cleanup_push((void (*)(void*))cleanup_handler, this);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
            while (isOK())
            {
                if (abstract::IRunnable * ptr = getNextRunnable())
                {
                    setState(RUN);
                    ptr->execute();
                    setState(DONE);
                }
            }
            pthread_cleanup_pop(0);
        }

        inline void cleanup()
        {
            setOK(false);
            setState(CANCELLED);

            if (TRunnable * ptr = getRunnable())
            {
                ptr->onCancel();
            }

            setNextRunnable(NULL);
        }

        virtual ~PoolThread()
        {
            setOK(false);
            if (TRunnable * ptr = getRunnable())
            {
                ptr->shutdown();
            }
            getLog()->debug(__FILE__, __LINE__, "Calling cancel() for TID: %d\n", this->getThreadId());
            cancel();
            getLog()->debug(__FILE__, __LINE__, "Calling finish() for TID: %d\n", this->getThreadId());
            finish();
            getLog()->debug(__FILE__, __LINE__, "finished TID: %d\n", this->getThreadId());
        }
    };
}
