/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: PersistentQueue.h 1 2015-03-21 20:32:15Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __PERSISTENTQUEUE_H__
#  define __PERSISTENTQUEUE_H__

#  include <time.h>
#  include <errno.h>
#  include <string.h>
#  include <stdint.h>
#  include <sched.h>
#  include <queue>
#  include <mutex>
#  include <atomic>

#  include <abstract/QueueInterface.h>
#  include <abstract/Cleanable.h>

#  include <sys/Semaphore.h>
#  include <sys/Nanosleep.h>

#  include <TSLog.h>
#  include <Val2Type.h>

#  include <ITCException.h>

#  include <LMDB.h>
#  include <LMDBEnv.h>
#  include <LMDBROTxn.h>
#  include <LMDBWOTxn.h>
#  include <LMDBWriter.h>
#  include <SyncWriterAdapter.h>

#  include <QueueTxn.h>
#  include <QueueObject.h>




namespace itc
{

  /**
   * @brief a parsistent queue with LMDB backend. Due to nature of LMDB there
   * is only one writer thread is allowed, so this queue must interract with
   * a writer thread (itc::lmdb::DBWriter). Writing to this queue is as fast as 
   * the LMDB is allowing, reading is asynchronous. However after you got the 
   * message and processed it in any way you require, you have to call 
   * QueueTxnDELSPtr::get()->commit() method, to remove the message from database.
   * Uncommited messages are reside in the database until they'll used again, 
   * with load() method.
   * 
   **/

  template<typename DataType> class PersistentQueue:public QueueInterface<QueueMessageSPtr>
  {
   public:
    typedef std::shared_ptr<lmdb::ROTxn::Cursor> CursorSPtr;
    typedef QueueTxn<lmdb::DEL> QueueTxnDelType;
    typedef QueueTxn<lmdb::ADD> QueueTxnAddType;
    typedef std::shared_ptr<QueueTxnDelType> QueueTxnDELSPtr;
    typedef std::shared_ptr<QueueTxnAddType> QueueTxnADDSPtr;
    typedef QueueMessageSPtr Storable;

   private:
    std::mutex mMutex;
    std::mutex mCommitProtect;
    sys::Semaphore mMsgTrigger;
    std::queue<Storable> mDataQueue;
    std::queue<DBKey> mKeysQueue;
    lmdb::DBWriterSPtr mDBWriter;
    std::atomic<bool> maySend;
    std::atomic<bool> mayRecv;


   public:

    explicit PersistentQueue(const lmdb::DBWriterSPtr& ref)
      :QueueInterface<QueueMessageSPtr>(), mMutex(), mCommitProtect(), mMsgTrigger(),
      mDBWriter(ref), maySend(true), mayRecv(true)
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      load();
    }

    bool send(const QueueMessageSPtr& pData)
    {
      if(maySend)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        QueueTxnADDSPtr qtmp(std::make_shared<QueueTxnAddType>(mDBWriter, pData));
        if(qtmp.get()->commit())
        {
          mDataQueue.push(pData);
          mKeysQueue.push(pData.get()->getKey());
          mMsgTrigger.post();
          return true;
        }
        return false;
      }
      return false;
    }

    QueueTxnDELSPtr recv()
    {
      mMsgTrigger.wait();
      if(mayRecv)
      {
        std::lock_guard<std::mutex> sync(mMutex);
        if(!(mDataQueue.empty() && (!mKeysQueue.empty())))
        {
          Storable tmpdata(mDataQueue.front());

          mKeysQueue.pop();
          mDataQueue.pop();
          return std::make_shared<QueueTxnDelType>(mDBWriter, tmpdata);
        }
        throw TITCException<exceptions::QueueOutOfSync>(exceptions::ITCGeneral);
      }
      throw TITCException<exceptions::QueueIsGoingDown>(exceptions::ITCGeneral);
    }

    bool recv(QueueMessageSPtr& pData)
    {
      throw TITCException<exceptions::ImplementationForbidden>(exceptions::ITCGeneral);
    }

    uint64_t depth()
    {
      std::lock_guard<std::mutex> sync(mMutex);
      return mDataQueue.size();
    }

    void destroy()
    {
      maySend = false;
      itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> On Queue destroy, waiting to cleanup remaining messages");
      sys::Nap s;
      while((mMsgTrigger.getValue() > 0)&&(depth() > 0))
      {
        s.usleep(100000);
      }
      mayRecv = false;
      itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> On Queue destroy, the queue is clean as a virgin");
      mMsgTrigger.post();
      itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> On Queue destroy, before mutex lock");
      std::lock_guard<std::mutex> sync(mMutex);
      itc::getLog()->debug(__FILE__, __LINE__, "[trace] <- On Queue destroy, after mutex lock");
    }

    ~PersistentQueue()
    {
      destroy();
    }
   private:

    void load()
    {
      lmdb::ROTxn txn(mDBWriter.get()->getDatabase());
      CursorSPtr cursor(txn.getCursor());
      size_t count = 0;
      try
      {

        QueueMessageSPtr last(cursor->getLast());
        QueueMessageSPtr current(cursor->getFirst());

        if(current.get()->getKey() != last.get()->getKey())
        {
          do
          {
            mKeysQueue.push(current.get()->getKey());
            mDataQueue.push(current);
            mMsgTrigger.post();
            ++count;
          }
          while((current = cursor->getNext()));
        }
      }catch(TITCException<exceptions::MDBKeyNotFound>& e)
      {
        itc::getLog()->info("PersistentQueue::load(): %ju messages loaded", count);
      }
    }
  };
}

#endif /*THREADSAFELOCALQUEUE_H_*/
