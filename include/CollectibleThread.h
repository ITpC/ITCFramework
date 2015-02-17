/* 
 * File:   CollectibleTread.h
 * Author: pk
 *
 * Created on 17 Февраль 2015 г., 0:05
 */

#ifndef COLLECTIBLETREAD_H
#define	COLLECTIBLETREAD_H

#include <sys/CancelableThread.h>
#include <abstract/Runnable.h>
#include <gc.h>
#include <abstract/gcClient.h>
#include <sys/SyncLock.h>
#include <sys/Mutex.h>
#include <memory>
#include <InterfaceCheck.h>

namespace itc
{
    template <typename T> class CollectibleThread : public itc::sys::CancelableThread<T>, public abstract::gcClient
    {
    private:
        bool        mCanRemove;
        itc::sys::Mutex mMutex;
    public:
        CollectibleThread(const std::shared_ptr<T>& ref)
        : mCanRemove(false),itc::sys::CancelableThread<T>(ref)
        {
            STATIC_CHECKER3MSG(
               CheckRelationship(
                 T, subclassof, gcClient
                 ),
               T, _is_not_a_subclass_of_, gcClient
           );
        }
        bool canRemove()
        {
            itc::sys::SyncLock  dosync(mMutex);
            return this->getRunnable()->canRemove();
        }
        ~CollectibleThread()
        {
            this->getRunnable()->shutdown();
            itc::getLog()->debug(__FILE__,__LINE__,"CollectibleThread::~CollectibleThread(), %u", this->getThreadId());
        }
    };
}



#endif	/* COLLECTIBLETREAD_H */

