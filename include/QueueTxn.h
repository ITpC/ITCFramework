/* 
 * File:   QueueTxn.h
 * Author: pk
 *
 * Created on 24 Март 2015 г., 10:15
 */

#ifndef QUEUETXN_H
#  define	QUEUETXN_H
#  include <sys/AtomicBool.h>
#  include <sys/AtomicDigital.h>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <LMDBWriter.h>
#  include <memory>

namespace itc
{

  template <typename T> class QueueTxn
  {
   public:
    typedef std::shared_ptr<lmdb::SyncDBWriterAdapter> SyncDBWAdapterSPtr;
   private:
    sys::Mutex mMutex;
    SyncDBWAdapterSPtr mWAdapter;
    sys::AtomicBool mAbort;
    sys::AtomicUInt64 mKey;
    std::shared_ptr<T> mData;
    
   public:

    explicit QueueTxn(const SyncDBWAdapterSPtr& ref, const std::shared_ptr<T>& dref, const uint64_t key)
      : mMutex(), mWAdapter(ref), mAbort(false), mKey(key), mData(dref)
    {
      sys::SyncLock dosync(mMutex);
    }
    explicit QueueTxn(const QueueTxn&) = delete;
    explicit QueueTxn(QueueTxn&) = delete;

    const std::shared_ptr<T>& getData()
    {
      sys::SyncLock dosync(mMutex);
      return mData;
    }

    void abort()
    {
      mAbort = true;
    }

    ~QueueTxn()
    {
      if(!mAbort)
      {
        sys::SyncLock dosync(mMutex);
        lmdb::WObjectSPtr tmp(std::make_shared<itc::lmdb::WObject>());
        uint64_t key = mKey;
        tmp->key.mv_data = &key;
        tmp->key.mv_size = sizeof(uint64_t);
        tmp->theOP = lmdb::DEL;
        mWAdapter->write(tmp);
      }
    }
  };
}

#endif	/* QUEUETXN_H */

