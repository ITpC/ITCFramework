/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
  * $Id: TCPListener.h 1 2015-02-28 13:30:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 * 06.01.2018 - gracefull shutdown.
 **/

#ifndef __TCPLISTENER_H__
#define	__TCPLISTENER_H__
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <cstdint>

#include <sys/synclock.h>
#include <TCPSocketDef.h>
#include <abstract/IController.h>
#include <abstract/Runnable.h>
#include <sys/CancelableThread.h>
#include <Singleton.h>

namespace itc
{
  /**
   * \@brief a listener class which supposed to run as a CancelableThread. 
   * Do not run this listener as a ThreadPool runner, it will block the ThreadPool
   * indefinitely. The only task this class does is to accept new inbound 
   * connections and notify the associated view (usually a worker class).
   **/
  class TCPListener: public ::itc::abstract::IRunnable, public ::itc::abstract::IController<CSocketSPtr>
  {
  private:
    std::mutex        mMutex;
    std::string       mAddress;
    int               mPort;
    ServerSocket      mServerSocket;
    ViewTypeSPtr      mSocketsHandler;
    std::atomic<bool> doRun;
    std::atomic<bool> canDestroy;
    
	public:
   typedef ModelType value_type;
   
    explicit TCPListener(const std::string& address,const int port,const ViewTypeSPtr& sh)
    : mMutex(), mAddress(address), mPort(port), mServerSocket(mAddress,mPort),
      mSocketsHandler(sh),doRun(true),canDestroy(false)
    {
      if(!mSocketsHandler.lock())
        throw std::runtime_error("The connection handler view does not exists (nullptr)");
      itc::getLog()->debug(__FILE__,__LINE__,"TCP Listener %p started on address %s and port %d", this, address.c_str(), port);
    }
    
    TCPListener(const TCPListener&)=delete;
    TCPListener(TCPListener&)=delete;
    
    void execute()
    {
      while(doRun.load())
      {
        SyncLock sync(mMutex);
        value_type newClient(
          itc::Singleton<TCPSocketsFactory>::getInstance<
            size_t,size_t
          >(5,10)->getBlindSocket()
        );
        try {
          if(mServerSocket.accept(newClient) == -1)
          {
            throw std::system_error(errno,std::system_category(),"TCPListener::execute()::ServerSocket.accept(ClientSocket)");
          }
          else
          { 
            std::string peeraddress;
            newClient->getpeeraddr(peeraddress);
            
            if(peeraddress.empty())
            {
              itc::getLog()->debug(__FILE__,__LINE__,"TCPListener %p invalid peer address, closing connection",this);
              newClient->close();
            }
            else 
            {
              itc::getLog()->debug(__FILE__,__LINE__,"TCPListener %p accepting connection from %s",this,peeraddress.c_str());
              if(!notify(newClient,mSocketsHandler))
              {
                doRun.store(false);
                itc::getLog()->debug(__FILE__,__LINE__,"TCPListener %p is going down (shutdown is requested from external component)",this);
                break;
              }
            }
          }
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Exception: %s",e.what());
          doRun.store(false);
          mServerSocket.close();
          break;
        }
      }
      canDestroy.store(true);
    }
    
    void onCancel()
    {
      this->shutdown();
    }
    void shutdown()
    {
      doRun.store(false);
      itc::getLog()->debug(__FILE__,__LINE__,"TCP Listener %p: shutdown() is called",this);
      while(!canDestroy.load())
      {
        if(mAddress == "0.0.0.0")
        {
          try{
            itc::ClientSocket aSocket("127.0.0.1",mPort);
            aSocket.close();
          }catch(const std::system_error& e)
          {
            itc::getLog()->info(__FILE__,__LINE__,"Listener went down, it is safe to shutdown");
            break;
          }
        }
        else
        {
          try{
            itc::ClientSocket aSocket(mAddress,mPort);
            aSocket.close();
          }catch(const std::system_error& e)
          {
            itc::getLog()->info(__FILE__,__LINE__,"Listener went down, it is safe to shutdown");
            break;
          }
        }
        sched_yield();
      }
    }
    ~TCPListener()
    {
      this->shutdown();
    }
  };
}

typedef std::shared_ptr<itc::TCPListener> TCPListenerSPtr;
typedef itc::sys::CancelableThread<itc::TCPListener> TCPListenerThread;
typedef std::shared_ptr<TCPListenerThread>  TCPListenerThreadSPtr;
typedef std::weak_ptr<TCPListenerThread>  TCPListenerThreadWPtr;

#endif	/* __TCPLISTENER_H__ */

