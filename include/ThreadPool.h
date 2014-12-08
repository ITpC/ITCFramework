/*
 * File:   ThreadPool.h
 * Author: pk
 *
 * $Id: ThreadPool.h 22 2010-11-23 12:53:33Z pk $
 */

#include <map>
#include <vector>
#include <queue>


#include <memory>

#include <Val2Type.h>

#include <sys/Mutex.h>
#include <sys/SyncLock.h>

#include <abstract/Runnable.h>
#include <abstract/IThreadPool.h>
#include <abstract/IMessageListener.h>

#include <PoolThread.h>

#include <ThreadSafeLocalQueue.h>

#include <bdmap.h>



#ifndef __THREADPOOL_H__
#    define    __THREADPOOL_H__


namespace itc
{
typedef std::shared_ptr<PoolThread> PoolThreadSPtr;
typedef std::shared_ptr<IRunnable> IRunnableSPtr;

template <
EnqueueBlockPolicy TBlockPolicy = ASYNC,
EnqueueDiscardPolicy TDiscardPolicy = ERROR
> class ThreadPool : public abstract::IThreadPool, IView<::itc::ModelType>
{
private:
    typedef bdmap<pthread_t, std::vector::size_type>::type Intersection;

    sys::Mutex mMutex;
    sys::Semaphore mHavePassiveThreads;

    std::vector<PoolThreadSPtr> mActiveThreads;
    std::queue<PoolThreadSPtr> mPassiveThreads;

    std::queue<IRunnableSPtr> mTaskQueue;

    Intersection mThread2PoolIdx;

    size_t mThreadsStarted;
    size_t mMinThreads;
    size_t mMaxThreads;


public:

    ThreadPool(const size_t pMinThreads = 0, const size_t pMaxThreads = 10)
    {
        sys::SyncLock sync(mMutex);

        mMinThreads = pMinThreads;
        mMaxThreads = pMaxThreads;

        for (mThreadsStarted = 0; mThreadsStarted < mMinThreads; mThreadsStarted++)
        {
            PoolThreadSPtr tmp(new PoolThread(this));
            mPassiveThreads.push(tmp);
            mHavePassiveThreads.post();
            mThreadsStarted++;
        }
    }

    template<typename TRunnable> const bool enqueue(TRunnable* ptr)
    {
        return enqueue<TRunnable > (
                                    ptr,
                                    utils::Int2Type<TBlockPolicy>,
                                    utils::Int2Type<TDiscardPolicy>
                                    );
    }

protected:

    /**
     * @brief asynchronously assign a runnable to the PoolThread or queue the requests
     *        if there no free threads in pool and number of threads reached the mMaxThreads.
     *        It may happen that on exception created PoolThread or moved from passive pool
     *        would be wiped out of control, so mThreadsStarted will be decreased.
     *        Uncontrolled threads are canceled automatically after the the stack pops.
     *
     * @return false on exception, true on success.
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<ASYNC>& fictive,
                                                     const utils::Int2Type<ERROR>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            mTaskQueue.push(ptr);
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
            return false;
        }
        return true;
    }

    /**
     * @brief asynchronously assign a runnable to the PoolThread or queue the requests
     *        if there no free threads in pool and number of threads reached the mMaxThreads.
     *        It may happen that on exception created PoolThread or moved from passive pool one,
     *        would be wiped out of control, so mThreadsStarted will be decreased.
     *        Uncontrolled threads are canceled automatically after the stack pops.
     *
     * @return always true.
     *
     * @exception propagates any exception further
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<ASYNC>& fictive,
                                                     const utils::Int2Type<EXCEPTION>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            mTaskQueue.push(ptr);
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
            throw ITCException(EINVAL, exceptions::Can_not_assign_runnable);
        }
        return true;
    }

    /**
     * @brief Synchronously assign a Runnable to a PoolThread and  wait for free threads if there
     * is no one free and mThreadsStarted reached mMaxThreads.
     *
     * @exception itc::exceptions::Can_not_assign_runnable
     *
     * @return always true
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<SYNC>& fictive,
                                                     const utils::Int2Type<EXCEPTION>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
            throw ITCException(EINVAL, exceptions::Can_not_assign_runnable);
        }

        mHavePassiveThreads.wait();
        return enqueue(fictive, fictive1);
    }

    /**
     * @brief Synchronously assign a Runnable to a PoolThread or wait for free thread if there is no one free
     *        and mThreadsStarted reached mMaxThreads.
     *
     * @return true on success or false on exception
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<SYNC>& fictive,
                                                     const utils::Int2Type<ERROR>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(
                                         __FILE__, __LINE__,
                                         "Exception %s in ThreadPool::enqueue() at address %x",
                                         e.what(), this
                                         );
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(
                                         __FILE__, __LINE__,
                                         "Exception %s in ThreadPool::enqueue() at address %x",
                                         e.what(), this
                                         );
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(
                                 __FILE__, __LINE__,
                                 "Exception %s in ThreadPool::enqueue() at address %x",
                                 e.what(), this
                                 );
            return false;
        }

        mHavePassiveThreads.wait();
        return enqueue(fictive, fictive1);
    }

    /**
     * @brief assign Runnable to the PollThread or discard a request
     *        when no free threads are available and number
     *        of threads in pool has reached mMaxThreads;
     *
     * @return true on success or false on error.
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<DISCARD>& fictive,
                                                     const utils::Int2Type<ERROR>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(
                                         __FILE__, __LINE__,
                                         "Exception %s in ThreadPool::enqueue() at address %x",
                                         e.what(), this
                                         );
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(
                                         __FILE__, __LINE__,
                                         "Exception %s in ThreadPool::enqueue() at address %x",
                                         e.what(), this
                                         );
                    return false;
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(
                                 __FILE__, __LINE__,
                                 "Exception %s in ThreadPool::enqueue() at address %x",
                                 e.what(), this
                                 );
            return false;
        }
        return false;
    }

    /**
     * @brief assign Runnable to the PollThread or discard a request when no free threads
     *           available and number of threads in pool has reached mMaxThreads;
     *
     * @exception ITCException(EBUSY,exceptions::Can_not_assign_runnable)
     *               on discard condition and propagates all exceptions.
     *
     * @return true on success or throws an exception otherways.
     **/
    template <typename TRunnable> const bool enqueue(
                                                     TRunnable* ptr,
                                                     const utils::Int2Type<DISCARD>& fictive,
                                                     const utils::Int2Type<EXCEPTION>& fictive1
                                                     )
    {
        try
        {
            sys::SyncLock sync(mEnqueueMutex);
            if (!mPassiveThreads.empty())
            {
                PoolThreadSPtr tmp(mPassiveThreads.front());
                mPassiveThreads.pop();
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
            else if (mThreadsStarted < mMaxThreads)
            {
                PoolThreadSPtr tmp(new PoolThread(this));
                try
                {
                    size_t idx = mActiveThreads.size();
                    pthread_t TID = tmp.get()->getThreadId();
                    mActiveThreads.push_back(tmp);
                    mThread2PoolIdx.insert(TID, idx);
                    mThreadsStarted++;
                }
                catch (std::exception& e)
                {
                    mThreadsStarted--;
                    itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
                    throw ITCException(ENOMEM, exceptions::Can_not_assign_runnable);
                }
                tmp.get()->setNextRunnable<TRunnable > (ptr);
                return true;
            }
        }
        catch (std::exception& e)
        {
            itc::getLog()->error(__FILE__, __LINE__, "Exception %s in ThreadPool::enqueue() at address %x", e.what(), this);
            throw ITCException(EINVAL, exceptions::Can_not_assign_runnable);
        }
        throw ITCException(EBUSY, exceptions::Can_not_assign_runnable);
    }

    void update(const ModelType& ref)
    {
        sys::SyncLock sync(mMutex);

        switch (ref.second)
        {
            case WAIT:
            {
                Intersection::iterator it = mThread2PoolIdx.find<first > (ref.first);
                if (it != mThread2PoolIdx.end())
                {
                    size_t idx = it->second;

                }
                else
                {
                    // WTF ????? it must not come to this section. TODO: make here very rude stop!
                }
            }
                break;
            case CANCELLED:
            {

            }
                break;
            default:
            {

            }
                break;
        }
    }
};
}

#endif    /* _THREADPOOL_H */

