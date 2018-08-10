/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ClientSocketsFactory.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef __CLIENT_SOCKETS_FACTORY_H__
#  define __CLIENT_SOCKETS_FACTORY_H__

#  include <memory>
#  include <cstdint>
#  include <queue>
#  include <mutex>
#  include <InterfaceCheck.h>
#  include <net/NSocket.h>
#  include <sys/synclock.h>

template <uint64_t SOpts = CLIENT_SOCKET> class ClientSocketsFactory
{
 private:
  std::mutex mMutex;
 public:
  typedef ::itc::net::Socket<SOpts,0> ClientSocketType;
  typedef ::std::shared_ptr<ClientSocketType> SharedClientSocketPtrType;

  explicit ClientSocketsFactory(size_t maxPrebuild, size_t minQL)
    :mMutex(), mMaxQueueLength(maxPrebuild), mMinQueueLength(minQL)
  {
    static_assert(SOpts < SERVER_SOCKET, "Must be a tcp client socket type");
    static_assert(SOpts > CLIENT_SOCKET, "Must be a tcp client socket type");
    static_assert(CLN_TCP_KA_TND < SERVER_SOCKET, "WTF ? can't you count ?!");
    static_assert(CLN_TCP_KA_TD < SERVER_SOCKET, "WTF ? can't you count ?!");
    
    SyncLock sync(mMutex);
    
    for(
      size_t i = 0;i < mMaxQueueLength;
      mPreBuildSockets.push(
        std::move(std::make_shared<ClientSocketType>())
      ), i++
    );
  }

  auto getBlindSocket()
  {
    SyncLock sync(mMutex);
    if(size_t depth = mPreBuildSockets.size() > mMinQueueLength)
    {
      auto ptr = std::move(mPreBuildSockets.front());
      mPreBuildSockets.pop();
      return ptr;
    }
    else
    {
      for(
        size_t i = depth;i < mMaxQueueLength;
          mPreBuildSockets.push(
            std::move(std::make_shared<ClientSocketType>())
          ), i++
        );
      auto ptr = std::move(mPreBuildSockets.front());
      mPreBuildSockets.pop();
      return ptr;
    }
  }

 private:
  std::queue<SharedClientSocketPtrType> mPreBuildSockets;
  size_t mMaxQueueLength;
  size_t mMinQueueLength;
};

#endif /*__CLIENT_SOCKETS_FACTORY_H__*/
