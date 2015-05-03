/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: tsbqueue.h 3 Май 2015 г. 17:05 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __TSBQUEUE_H__
#  define	__TSBQUEUE_H__

#include <tsqueue.h>
#include <sys/PosixSemaphore.h>
#include <sys/synclock.h>

/**
 * @brief thread safe blocking queue. The only two operations are available:
 * send and recv. recv will block if the queue is empty, until the queue is 
 * not empty.
 */
template <typename DataType> class tsbqueue : public tsqueue<DataType>
{
private:
 std::mutex mMutexL2;
 itc::sys::Semaphore mEvent;
 
public:
 explicit tsbqueue():tsqueue<DataType>(),mMutexL2(),mEvent(){};

 /**
  * @brief send message of DataType to the queue.
  * @param ref message to be sent.
  */
 void send(const DataType& ref)
 {
   SyncLock sync(mMutexL2);
   this->push(ref);
   mEvent.post();
 }
 
 /**
  * @brief receive message from queue upon arrival.
  * @return DataType
  */
 DataType recv()
 {
   mEvent.wait();
   try
   {
     SyncLock sync(mMutexL2);
     DataType result(this->front());
     this->pop();
     return result;
   }catch(...)
   {
     throw;
   }
 }
};

#endif	/* __TSBQUEUE_H__ */

