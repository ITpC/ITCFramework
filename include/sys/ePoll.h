/* 
 * File:   ePoll.h
 * Author: pk
 *
 * Created on 25 Февраль 2015 г., 11:44
 */

#ifndef EPOLL_H
#define	EPOLL_H

#include <string.h>
#include <map>
#include <queue>
#include <sys/epoll.h>
#include <abstract/IView.h>
#include <abstract/IController.h>
#include <abstract/Runnable.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <TSLog.h>
#include <ITCException.h>
#include <Val2Type.h>
#include <cstdint>
#include <unistd.h>

namespace itc
{
  namespace sys
  {
    enum POLLTYPE{POLLIN,POLLOUT};

    template <POLLTYPE PT_Value> class ePoll : public itc::abstract::IController<uint32_t>
    {
    public:
      typedef std::map<int,ViewTypeSPtr>          ObserversContainer;
      typedef std::pair<int,ViewTypeSPtr>         PairType;
      typedef std::queue<PairType>                RequestListType;
      typedef struct epoll_event                  ePollEventType;
      typedef std::unique_ptr<ePollEventType[]>   ePollEventUPtr;

      explicit ePoll(const size_t& nevents)
      :
          mMutex(),mPollMutex(), mMaxEvents(nevents),
          events(new ePollEventType[mMaxEvents]),mDestroy(false)
      {
          SyncLock sync1(mMutex);
          SyncLock sync2(mPollMutex);

          if((mPollFD = epoll_create1(0))<0)
          {
              throw TITCException<exceptions::NoEPoll>(errno);
          }
      }

      inline void add(int fd, const ViewTypeSPtr& ref)
      {
          SyncLock synchronize(mMutex);
          mRequests.push(PairType(fd,ref));
          itc::getLog()->debug(__FILE__,__LINE__,"added a request to be an observer for fd %d with address %jx",fd,ref.get());
      };


      int poll()
      {
        SyncLock sync1(mPollMutex);
        if(!mDestroy)
        {
          while(!mRequests.empty())
          {
            SyncLock sync2(mMutex);
            utils::Int2Type<PT_Value> fictive;
            add(mRequests.front().first, mRequests.front().second,fictive);
            mObservers[mRequests.front().first]=mRequests.front().second;
            itc::getLog()->debug(__FILE__,__LINE__,"ePoll::poll() - added an observer for fd %d with address %jx",mRequests.front().first,mRequests.front().second.get());
            mRequests.pop();
          }
          int ret=epoll_wait(mPollFD, events.get(), mMaxEvents,-1);
          if(ret>0)
          {
            for(int i=0;i<ret;i++)
            {
              if(((events.get())[i].events) & (EPOLLRDHUP|EPOLLERR|EPOLLHUP))
              {
                ObserversContainer::iterator it=mObservers.find((events.get())[i].data.fd);

                if(it!=mObservers.end())
                {
                  if(it->second)
                  {
                    notify((events.get())[i].events,it->second);
                    itc::getLog()->trace(__FILE__,__LINE__,"ePoll::poll() - observer %jx is notified about event %u",it->second.get(),(events.get())[i].events);
                  }
                  itc::getLog()->trace(__FILE__,__LINE__,"ePoll::poll() - erasing observer %jx",it->second.get());
                  mObservers.erase(it);
                  del((events.get())[i].data.fd);
                }
              }
              else
              {
                itc::getLog()->trace(
                    __FILE__,__LINE__,
                    "ePoll::poll() - notifieng an observer %jx for fd %d of events %u",
                     mObservers[(events.get())[i].data.fd].get(),
                     (events.get())[i].data.fd,(events.get())[i].events
                );
                notify((events.get())[i].events,mObservers[(events.get())[i].data.fd]);
              }
            }
            return 1;
          }
          else if(ret == -1)
          {
            ::itc::getLog()->error(__FILE__,__LINE__,"in ePoll()::poll() on epoll_wait(): %s",strerror(errno));
            return -1;
          }
          else if(ret == 0)
          {
            ::itc::getLog()->error(__FILE__,__LINE__,"in ePoll()::poll() nothing to poll");
            return 0;
          }
        }
        else
        {
          ::itc::getLog()->error(__FILE__,__LINE__,"calling  ePoll()::poll() on object destroy");
          return -1;
        }
        ::itc::getLog()->error(__FILE__,__LINE__,"ePoll()::poll(), - error in source code. Never should appear hereafter");
        return -1;
      }

      ~ePoll()
      {
          SyncLock sync1(mPollMutex);
          SyncLock sync2(mMutex);
          mDestroy=true;
          while(!mRequests.empty()) mRequests.pop();
          mObservers.clear();
      }

    private:
      Mutex               mMutex;
      Mutex               mPollMutex;
      size_t              mMaxEvents;
      ePollEventUPtr      events;
      bool                mDestroy;
      int                 mPollFD;
      ObserversContainer  mObservers;
      RequestListType     mRequests;

      inline  void add(int fd, const ViewTypeSPtr& ref, const utils::Int2Type<POLLIN>& polltype)
      {
        struct epoll_event event;

        event.events=EPOLLIN|EPOLLRDHUP|EPOLLPRI|EPOLLERR|EPOLLHUP;

        if(!mDestroy)
        {
          if(epoll_ctl(mPollFD,EPOLL_CTL_ADD,fd,&event)==-1)
          {
            throw TITCException<exceptions::EPollCTLError>(errno);
          }
        }
      }

      inline void add(int fd, const ViewTypeSPtr& ref, const utils::Int2Type<POLLOUT>& polltype)
      {
        struct epoll_event event;

        event.events=EPOLLOUT|EPOLLRDHUP|EPOLLPRI|EPOLLERR|EPOLLHUP;

        if(!mDestroy)
        {
          if(epoll_ctl(mPollFD,EPOLL_CTL_ADD,fd,&event)==-1)
          {
            throw TITCException<exceptions::EPollCTLError>(errno);
          }
        }
      }

      inline  void del(int fd)
      {
        // inclompatible to kernels before 2.6.9, the event array is NULL here.
        if(!mDestroy)
        {
          if(epoll_ctl(mPollFD,EPOLL_CTL_DEL,fd,(epoll_event*)0)==-1)
          {
              throw TITCException<exceptions::EPollCTLError>(errno);
          }
        }
      }
    };
  }    
}

#endif	/* EPOLL_H */

