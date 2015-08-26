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
 **/

#ifndef __TCPLISTENER_H__
#define	__TCPLISTENER_H__
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <cstdint>

#include <net/NSocket.h>
#include <sys/synclock.h>
#include <TCPSocketDef.h>
#include <abstract/IController.h>
#include <abstract/Runnable.h>
#include <Singleton.h>

namespace itc
{

  class TCPListener: public ::itc::abstract::IRunnable, public ::itc::abstract::IController<CSocketSPtr>
  {
  private:
    std::mutex        mMutex;
    std::string       mAddress;
    int               mPort;
    ServerSocket      mServerSocket;
    ViewTypeSPtr      mSocketsHandler;
    std::atomic<bool> mNotEnd;

	public:
   typedef ModelType value_type;
   
    explicit TCPListener(const std::string& address,const int port,const ViewTypeSPtr& sh)
    : mMutex(), mAddress(address), mPort(port), mServerSocket(mAddress,mPort),
      mSocketsHandler(sh),mNotEnd(true)
    {
      if(mSocketsHandler.get()==nullptr)
        throw std::runtime_error("The connection handler view does not exists (nullptr)");
    }
    
    TCPListener(const TCPListener&)=delete;
    TCPListener(TCPListener&)=delete;
    
    void execute()
    {
      while (mNotEnd)
      {
        SyncLock sync(mMutex);

        value_type newClient(
          itc::Singleton<TCPSocketsFactory>::getInstance<
            size_t,size_t
          >(100,1000)->getBlindSocket()
        );

        if(int ret=mServerSocket.accept(newClient) == -1)
        {
          throw TITCException<exceptions::InvalidSocketException>(errno);
        }
        else
        {
          std::string peeraddr;
          newClient.get()->getpeeraddr(peeraddr);
          if(peeraddr.empty())
          {
            itc::getLog()->error(__FILE__,__LINE__,"Socket is already closed. It seems like a connections DDOS");
          }
          else
          {
            itc::getLog()->info("Inbound connection from %s", peeraddr.c_str());
            notify(newClient,mSocketsHandler);
          }
        }
      }
    }
    void onCancel()
    {
      SyncLock sync(mMutex);
      mNotEnd=false;
      mServerSocket.close();
    }
    void shutdown()
    {
      SyncLock sync(mMutex);
      mNotEnd=false;
      mServerSocket.close();
    }
  };
}


#endif	/* __TCPLISTENER_H__ */

