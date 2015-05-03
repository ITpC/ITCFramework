/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: PollController.h 26 Апрель 2015 г. 1:54 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __POLLCONTROLLER_H__
#  define	__POLLCONTROLLER_H__
#include <sys/Poll.h>

#include <abstract/IController.h>

namespace itc
{
  typedef sys::Poll PollInType;
  typedef sys::Poll PollOutType;
  typedef abstract::IController<sys::pollFDEVENTPair> PollInCtlType;
  typedef PollInCtlType::ViewType PollInViewType;
  typedef std::shared_ptr<PollInViewType> PollSubscriberSPtr;
  typedef std::pair<int,PollSubscriberSPtr> FD2SubscriberPair;
  typedef std::map<int,PollSubscriberSPtr> FS2SubscribersMap;


  /**
   * @brief by default In-Type fd poller, use modify() to change specific fd to 
   * Out-Type poll. 
   * NOTE: This class suppose to be a singletone. There is a data race guarantee
   * in void execute(), if more then one instance of this class are running.
   * 
   **/
  class PollController
  : public PollInType,
    public PollInCtlType,
    public abstract::IRunnable
  {
   private:
    std::mutex mMutex;
    std::atomic<bool> mayRun;
    FS2SubscribersMap mSubscribers;
    
   public:
    PollController(const size_t max):PollInType(max),mMutex(),mayRun(true){}
    
    void subscribe(const int fd, const PollSubscriberSPtr& view)
    {
      SyncLock sync(mMutex);
      mSubscribers.insert(std::pair<int,PollSubscriberSPtr>(fd,view));
      if(mayRun) this->poll_ctl_add(fd);
    }

    void watch(const int fd)
    {
      SyncLock sync(mMutex);
      if(mayRun) this->poll_ctl_enable(fd);
    }
    
    void unwatch(const int fd)
    {
      SyncLock sync(mMutex);
      if(mayRun) this->poll_ctl_disable(fd);
    }
    
    void remove(const int fd)
    {
      SyncLock sync(mMutex);
      if(mayRun) this->poll_ctl_delete(fd);
    }
    
    void execute()
    {
      while(mayRun)
      {
        mMutex.lock();
        std::vector<pollfd> watchlist;
        this->fill(watchlist);
        mMutex.unlock();
        if((!watchlist.empty())&&(this->poll(watchlist,10)>0))
        {
          for(std::vector<pollfd>::iterator it=watchlist.begin();it!=watchlist.end();++it)
          {
            if(it->revents!=0)
            {
              FS2SubscribersMap::iterator retit=mSubscribers.find(it->fd);
              if(retit!=mSubscribers.end())
              {
                notify(std::pair<int,short>(it->fd,it->revents),retit->second);
              }
            }
          } 
        }
      }
    }
    void onCancel()
    {
      SyncLock sync(mMutex);
      mayRun=false;
      this->clear();
    }
    void shutdown()
    {
      SyncLock sync(mMutex);
      mayRun=false;
      this->clear();      
    }
  };

  typedef PollController::ViewType PollViewInterface;
  typedef std::shared_ptr<PollController> PollerSPtr;
  typedef sys::CancelableThread<PollController> PollerThread;
  
  const PollerSPtr getPoller()
  {
    return Singleton<PollerThread>::getInstance(Singleton<PollController>::getInstance(1000))->getRunnable();
  }

}

#endif	/* __POLLCONTROLLER_H__ */

