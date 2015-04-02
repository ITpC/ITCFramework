/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: QueueTxn.h 24 Март 2015 г. 10:15 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef QUEUETXN_H
#  define	QUEUETXN_H
#  include <memory>
#  include <Val2Type.h>
#  include <LMDBWriter.h>
#  include <abstract/IQTxn.h>
#  include <DBKeyType.h>
#  include <QueueObject.h>
#  include <atomic>
#  include <mutex>
#  include <limits>

namespace itc
{

  template <typename lmdb::WObjectOP operation> class QueueTxn:public abstract::IQTxn
  {
   public:
    typedef std::shared_ptr<lmdb::SyncDBWriterAdapter> SyncDBWAdapterSPtr;
    typedef lmdb::DBWriterSPtr DBWriterSPtrType;
   private:
    std::mutex mMutex;
    DBWriterSPtrType mDBWriter;
    SyncDBWAdapterSPtr mWAdapter;
    std::atomic<bool> mAbort;
    QueueMessageSPtr mData;
    std::atomic<bool> mCommited;
    utils::Int2Type<operation> mOperation;

   public:

    explicit QueueTxn(const DBWriterSPtrType& ref, const QueueMessageSPtr& dref)
    :mMutex(), mDBWriter(ref), mWAdapter(std::make_shared<lmdb::SyncDBWriterAdapter>(ref)), mAbort(false),
     mData(dref), mCommited(false)
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      itc::getLog()->trace(__FILE__, __LINE__, "QTxn with adapter address: %jx, for DB: %s is emmited", mWAdapter.get(), mDBWriter.get()->getDatabase()->getName().c_str());
    }

    explicit QueueTxn(const QueueTxn&) = delete;
    explicit QueueTxn(QueueTxn&) = delete;

    const QueueMessageSPtr& getData()
    {
      std::lock_guard<std::mutex> dosync(mMutex);

      return mData;
    }

    const DBKey& getKey()
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      return mData.get()->getKey();
    }

    void abort()
    {

      mAbort = true;
      mCommited = false;
    }

    const bool commit()
    {

      return commit(mOperation);
    }

    ~QueueTxn() noexcept
    {

      if(!mCommited) itc::getLog()->error(__FILE__, __LINE__, 
        "Queue Txn with key %jx:%jx and address %jx is not commited:",
        mData.get()->getKey().left, mData.get()->getKey().right, this
      );
    }
   private:

    const bool commit(const utils::Int2Type<lmdb::DEL>& fictive)
    {
      if((!mCommited)&&(!mAbort))
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mData.get()->theOP = lmdb::DEL;
        try
        {
          mWAdapter->write(mData);
          mCommited = true;
          mAbort = false;
          return true;
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Exception on write to LMDB backend: %s", e.what());
          mAbort = true;
          mCommited = false;

          return false;
        }
      }
      return false;
    }

    const bool commit(const utils::Int2Type<lmdb::ADD>& fictive)
    {
      if((!mCommited)&&(!mAbort))
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mData.get()->theOP = lmdb::ADD;
        try
        {
          mWAdapter->write(mData);
          mCommited = true;
          return true;
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Exception on write to LMDB backend: %s", e.what());
          mAbort = true;
          mCommited = false;
          return false;
        }
      }
      return false;
    }
  };
}

#endif	/* QUEUETXN_H */

