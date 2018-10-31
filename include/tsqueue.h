/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: tsqueue.h 3 Май 2015 г. 17:02 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __TSQUEUE_H__
#  define	__TSQUEUE_H__

#include <queue>
#include <memory>
#include <mutex>
#include <sys/synclock.h>
/**
 * @brief thread safe STL queue. It is the same STL <queue> just protected by 
 * mutexes against data race
 **/
template <typename T> class tsqueue
{
private:
  std::mutex mMutex;
  std::queue<T> mQueue;
public:
  explicit tsqueue():mMutex(){}
  
  const bool empty()
  {
    STDSyncLock sync(mMutex);
    return mQueue.empty();
  }
  
  const tsqueue& push(const T& ref)
  {
    STDSyncLock sync(mMutex);
    mQueue.push(ref);
    return *this;
  }
  
  const tsqueue& push(const T&& ref)
  {
    STDSyncLock sync(mMutex);
    mQueue.push(ref);
    return *this;
  }
  
  const T& front()
  {
    STDSyncLock sync(mMutex);
    return mQueue.front();
  }
  
  const T& top()
  {
    STDSyncLock sync(mMutex);
    return mQueue.front();
  }
  
  const tsqueue& pop()
  {
    STDSyncLock sync(mMutex);
    mQueue.pop();
    return *this;
  }
  
  const size_t size()
  {
    STDSyncLock sync(mMutex);
    return mQueue.size();
  }
  
};


#endif	/* __TSQUEUE_H__ */

