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
#  include <sched.h>
#  include <queue>
#  include <mutex>
#  include <atomic>

#  include <abstract/QueueInterface.h>
#  include <sys/Semaphore.h>
#  include <TSLog.h>
#  include <Val2Type.h>
#  include <abstract/Cleanable.h>
#  include <ITCException.h>
#  include <LMDBWriter.h>
#  include <SyncWriterAdapter.h>
#  include <LMDBWObject.h>
#  include <LMDBROTxn.h>
#  include <LMDBWObject.h>
#  include <stdint.h>
#  include <QueueTxn.h>
#  include <abstract/TxnOwner.h>


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
  template <
  typename DataType
  > class PersistentQueue : public QueueInterface<DataType>,
  public std::enable_shared_from_this<PersistentQueue<DataType> >
  {
   public:
    typedef std::shared_ptr<lmdb::ROTxn::Cursor> CursorSPtr;
    typedef std::shared_ptr<itc::lmdb::WObject> WObjectSPtr;
    typedef QueueTxn<DataType,lmdb::DEL> QueueTxnDelType;
    typedef QueueTxn<DataType,lmdb::ADD> QueueTxnAddType;
    typedef std::shared_ptr<QueueTxnDelType> QueueTxnDELSPtr;
    typedef std::shared_ptr<QueueTxnAddType> QueueTxnADDSPtr;
    typedef std::shared_ptr<DataType> Storable;

   private:
    std::mutex mMutex;
    std::mutex mCommitProtect;
    sys::Semaphore mMsgTrigger;
    std::queue<Storable> mDataQueue;
    std::queue<uint64_t> mKeysQueue;
    lmdb::DBWriterSPtr mDBWriter;
    std::atomic<uint64_t> mNewKey;
    std::atomic<bool> maySend;
    std::atomic<bool> mayRecv;


   public:

    explicit PersistentQueue(const lmdb::DBWriterSPtr& ref)
      : QueueInterface<DataType>(), mMutex(), mCommitProtect(), mMsgTrigger(),
      mDBWriter(ref),maySend(true),mayRecv(true)
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      load();
    }

    bool send(const DataType& pData)
    {
      if(maySend)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        Storable tmp(std::make_shared<DataType>(pData));
        QueueTxnADDSPtr qtmp(std::make_shared<QueueTxnAddType>(mDBWriter,tmp,mNewKey));
        ++mNewKey;
        if(qtmp.get()->commit())
        {
          mDataQueue.push(tmp);
          mKeysQueue.push(qtmp.get()->getKey());
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
          uint64_t key=mKeysQueue.front();

          mKeysQueue.pop();
          mDataQueue.pop();
          return std::make_shared<QueueTxnDelType>(mDBWriter, tmpdata, key);
        }
        throw TITCException<exceptions::QueueOutOfSync>(exceptions::ITCGeneral);
      }
      throw TITCException<exceptions::QueueIsGoingDown>(exceptions::ITCGeneral);
    }

    bool recv(DataType& pData)
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
      itc::getLog()->debug(__FILE__,__LINE__,"[trace] -> On Queue destroy, waiting to cleanup remaining messages");
      sys::SemSleep s;
      while((mMsgTrigger.getValue()>0)&&(depth()>0))
      {
        s.usleep(100000);
      }
      mayRecv=false;
      itc::getLog()->debug(__FILE__,__LINE__,"[trace] -> On Queue destroy, the queue is clean as a virgin");
      mMsgTrigger.post();
      itc::getLog()->debug(__FILE__,__LINE__,"[trace] -> On Queue destroy, before mutex lock");
      std::lock_guard<std::mutex> sync(mMutex);
      itc::getLog()->debug(__FILE__,__LINE__,"[trace] <- On Queue destroy, after mutex lock");
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
        DataType edata, cdata;
        size_t last = cursor->getLast<DataType>(edata);
        size_t current = cursor->getFirst<DataType>(cdata);

        if(current != last) do
          {
            mKeysQueue.push(current);
            mDataQueue.push(std::make_shared<DataType>(cdata));
            mMsgTrigger.post();
            ++count;
            mNewKey=current;
          }while((current = cursor->getNext<DataType>(cdata)));
      }catch(TITCException<exceptions::MDBKeyNotFound>& e)
      {
        itc::getLog()->info("PersistentQueue::load(): %ju messages loaded", count);
        ++mNewKey;
      }
    }
  };
}

#endif /*THREADSAFELOCALQUEUE_H_*/
