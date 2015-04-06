/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: PQueueDataType.h 4 Апрель 2015 г. 0:15 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __PQUEUEDATATYPE_H__
#  define	__PQUEUEDATATYPE_H__

#  include <memory>
#  include <vector>
#  include <atomic>
#  include <mutex>
#  include <functional>
#  include <sys/synclock.h>

namespace itc
{
  namespace lmdb
  {
    class PQDTAccess;
  }

  class PQueueDataType
  {
   private:
    typedef std::vector<uint8_t> DataChunk;
    typedef std::shared_ptr<DataChunk> DataChunkSPtr;
    friend lmdb::PQDTAccess;

    std::recursive_mutex mMutex;
    DataChunkSPtr mData;

   public:

    explicit PQueueDataType(const uint64_t& sz, const uint8_t* ptr)
      :mMutex(), mData(set(sz, ptr))
    {
    }

    explicit PQueueDataType(PQueueDataType& ref)
      :mMutex(), mData(set(ref))
    {
    }
    
    PQueueDataType& operator=(PQueueDataType& ref)
    {
      RSyncLock sync(mMutex);
      ref.lock();
      mData=ref.mData;
      ref.unlock();
      return (*this);
    }
    
   private:

    size_t size() const
    {
      return mData.get()->size();
    }

    void lock()
    {
      mMutex.lock();
    }

    void unlock()
    {
      mMutex.unlock();
    }

    const DataChunkSPtr& get()
    {
      RSyncLock sync(mMutex);
      return mData;
    }
        
    const DataChunkSPtr& set(PQueueDataType& ref)
    {
      RSyncLock sync(mMutex);
      return ref.get();
    }


    DataChunkSPtr set(const uint64_t& size, const uint8_t* ptr)
    {
      RSyncLock sync(mMutex);
      DataChunkSPtr tmp(std::make_shared<DataChunk>(size));
      if((size>0)&&(ptr != nullptr))
      {
        memcpy(tmp.get()->data(), ptr, size);
      }
      return tmp;
    }
  };

}


#endif	/* __PQUEUEDATATYPE_H__ */

