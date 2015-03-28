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
#  include <LMDBWObject.h>
#  include <abstract/IQTxn.h>
#  include <atomic>
#  include <mutex>
#  include <limits>

namespace itc
{

  template <typename T, typename lmdb::WObjectOP operation> class QueueTxn : public abstract::IQTxn
  {
   public:
    typedef std::shared_ptr<lmdb::SyncDBWriterAdapter> SyncDBWAdapterSPtr;
    typedef lmdb::DBWriterSPtr DBWriterSPtrType;
   private:
    std::mutex mMutex;
    DBWriterSPtrType mDBWriter;
    SyncDBWAdapterSPtr mWAdapter;
    std::atomic<bool> mAbort;
    std::atomic<uint64_t> mKey;
    std::shared_ptr<T> mData;
    std::atomic<bool> mCommited;
    utils::Int2Type<operation> mOperation;

   public:

    explicit QueueTxn(
      const DBWriterSPtrType& ref, const std::shared_ptr<T>& dref,
      const uint64_t& key
      )
      : mMutex(), mDBWriter(ref), mWAdapter(std::make_shared<lmdb::SyncDBWriterAdapter>(ref)), mAbort(false),
      mKey(key), mData(dref), mCommited(false)
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      itc::getLog()->trace(__FILE__,__LINE__,"QTxn with adapter address: %jx, for DB: %s is emmited", mWAdapter.get(), mDBWriter.get()->getDatabase()->getName().c_str());
    }

    explicit QueueTxn(const QueueTxn&) = delete;
    explicit QueueTxn(QueueTxn&) = delete;

    const std::shared_ptr<T>& getData()
    {
      std::lock_guard<std::mutex> dosync(mMutex);

      return mData;
    }

    uint64_t getKey()
    {

      return uint64_t(mKey);
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

      if(!mCommited) itc::getLog()->error(__FILE__, __LINE__, "Queue Txn with key %ju and address %jx is not commited:", uint64_t(mKey), this);
    }
   private:

    const bool commit(const utils::Int2Type<lmdb::DEL>& fictive)
    {
      if((!mCommited)&&(!mAbort))
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        lmdb::WObjectSPtr tmp(std::make_shared<itc::lmdb::WObject>());
        uint64_t key = mKey;
        tmp->key.mv_data = &key;
        tmp->key.mv_size = sizeof(uint64_t);
        tmp->theOP = lmdb::DEL;
        try
        {
          mWAdapter->write(tmp);
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
        lmdb::WObjectSPtr tmp(std::make_shared<itc::lmdb::WObject>());
        uint64_t key = mKey;
        tmp->key.mv_data = &key;
        tmp->key.mv_size = sizeof(uint64_t);
        tmp->data.mv_data = mData.get();
        tmp->data.mv_size = sizeof(T);
        tmp->theOP = lmdb::ADD;
        try
        {
          mWAdapter->write(tmp);
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

