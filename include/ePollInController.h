/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ePollInController.h 25 Апрель 2015 г. 5:49 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __EPOLLINCONTROLLER_H__
#  define	__EPOLLINCONTROLLER_H__

#include <map>
#include <memory>
#include <mutex>
#include <atomic>

#include <sys/ePoll.h>
#include <abstract/IController.h>
#include <abstract/Runnable.h>
#include <sys/synclock.h>
#include <sys/Nanosleep.h>
#include <Singleton.h>

namespace itc
{
  typedef sys::ePoll<sys::PTYPE_POLLIN> ePollInType;
  typedef abstract::IController<ePollInType::FDEventPair> ePollInCtlType;
  typedef ePollInCtlType::ViewType ePollInViewType;
  typedef std::shared_ptr<ePollInViewType> ePollSubscriberSPtr;
  typedef std::pair<int,ePollSubscriberSPtr> FD2SubscriberPair;
  typedef std::map<int,ePollSubscriberSPtr> FS2SubscriberMap;
  
  class ePollInController
  : public ePollInType,
    public ePollInCtlType,
    public abstract::IRunnable
  {
   private:
    std::mutex mMutex;
    std::atomic<bool> mayRun;
    FS2SubscriberMap mSubscribers;
    sys::Nap  mNap;
    
   public:
    explicit ePollInController(const uint32_t& max) 
    : ePollInType(max), mMutex(), mayRun(true)
    {
    }
    
    void subscribe(const int fd, const ePollSubscriberSPtr& ptr)
    {
      SyncLock sync(mMutex);
      mSubscribers.insert(FD2SubscriberPair(fd,ptr));
      this->add(fd);
      getLog()->debug(__FILE__,__LINE__,"Added subscriber %jx for events of fd %u",ptr.get(),fd);
    }
    
    void execute()
    {
      while(mayRun)
      {
        std::vector<FDEventPair> anevents=this->poll(-1);
        if(anevents.empty())
        {
          mNap.usleep(10000);
        }
        else
        {
          SyncLock sync(mMutex);
          std::for_each(
            anevents.begin(),anevents.end(),
            [this](const FDEventPair& ep)
            {
              FS2SubscriberMap::iterator it=mSubscribers.find(ep.first);
              if(it!=mSubscribers.end())
              {
                notify(ep,it->second);
                getLog()->debug(__FILE__,__LINE__,"Subscriber %jx has been notified of events of fd %u",it->second.get(),ep.first);
                mSubscribers.erase(it);
                it=mSubscribers.begin();
              }
            }
          );
        }
      }
    }
    void onCancel()
    {
      mayRun=false;
      SyncLock sync(mMutex);
      mSubscribers.clear();
    }
    void shutdown()
    {
      mayRun=false;
      SyncLock sync(mMutex);
      mSubscribers.clear();
    }
  };
  typedef ePollInController::ViewType ePollInViewInterface;
  typedef std::shared_ptr<ePollInController> InPollerSPtr;
  typedef sys::CancelableThread<ePollInController> InPollerThread;
  
  const InPollerSPtr getInPoller()
  {
    return Singleton<InPollerThread>::getInstance(Singleton<ePollInController>::getInstance(1000))->getRunnable();
  }
  
}


#endif	/* __EPOLLINCONTROLLER_H__ */

