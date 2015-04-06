/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: SyncWriter.h 4 Апрель 2015 г. 1:03 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __SYNCWRITER_H__
#  define	__SYNCWRITER_H__

#  include <memory>
#  include <vector>
#  include <atomic>
#  include <mutex>
#  include <functional>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBWOTxn.h>
#  include <Singleton.h>
#  include <PQueueDataType.h>
#  include <DBKey.h>
#  include <ITCException.h>
#  include <TSLog.h>
#  include <sys/synclock.h>

namespace itc
{
  namespace lmdb
  {

    typedef std::vector<uint8_t> DataChunk;
    typedef std::shared_ptr<DataChunk> DataChunkSPtr;
    typedef std::shared_ptr<itc::lmdb::Database> LMDBSPtr;
    typedef itc::lmdb::WOTxn WOTransaction;

    class SyncWriterLock
    {
     private:
      std::recursive_mutex mMutex;

     public:
      virtual ~SyncWriterLock() = default;
      SyncWriterLock() = default;

      void use(PQueueDataType& ref, const DBKey& key, std::function<void(const MDB_val&, const MDB_val&) > writef)
      {
        RSyncLock sync(mMutex);
        ref.lock();
        DBKeyAccess key_access(key);
        MDB_val dbkey;
        key_access.use(
          [&dbkey,&ref,&writef](const MDB_val & _key){
            dbkey.mv_data = _key.mv_data;
            dbkey.mv_size = _key.mv_size;
            
            MDB_val data;
            data.mv_data = ref.get().get()->data();
            data.mv_size = ref.get().get()->size();
            writef(dbkey, data);
          }
        );
        ref.unlock();
      }

      void use(const DBKey& key, std::function<void(const MDB_val&) > delf)
      {
        RSyncLock sync(mMutex);
        DBKeyAccess key_access(key);
        MDB_val dbkey;
        key_access.use(
          [&dbkey,&delf](const MDB_val & ref)
          {
            dbkey.mv_data = ref.mv_data;
            dbkey.mv_size = ref.mv_size;
            delf(dbkey);
          }
        );
      }

      void use(const DBKey& key, std::function<void(const uint64_t&,const uint64_t&) > usef)
      {
        RSyncLock sync(mMutex);
        DBKeyAccess key_access(key);
        uint64_t u64a[2];
        key_access.use(
          [&u64a,&key](const uint64_t& l,const uint64_t& r)
          {
            u64a[0]=l;
            u64a[1]=r;
          }
        );
        usef(u64a[0],u64a[1]);
      }
            
      void use(PQueueDataType& ref, std::function<void(const uint8_t*, const uint64_t&)> accessf)
      {
        RSyncLock sync(mMutex);
        ref.lock();
        accessf(ref.get()->data(), ref.get()->size());
        ref.unlock();
      }

      void assign(PQueueDataType& src, PQueueDataType& dst)
      {
        RSyncLock sync(mMutex);
        src.lock();
        dst.lock();
        dst.mData = src.mData;
        src.unlock();
        dst.unlock();
      }
      
      void use(const DBKey& key, PQueueDataType& data, std::function<void(const uint64_t&, const uint64_t& r, const uint64_t&, const uint8_t*)> accessf)
      {
        RSyncLock sync(mMutex);
        {
          data.lock();
          DBKeyAccess key_access(key);
          DBKeyAccess::dbk _key;
          key_access.use(
            [&_key,this](const uint64_t& l, const uint64_t& r)
            {
              _key.left=l;
              _key.right=r;
            }
          );
          accessf(_key.left,_key.right,data.mData.get()->size(),data.mData.get()->data());
          data.unlock();
        }
      }
    };

    class SyncWriter
    {
     private:
      std::mutex mMutex;
      LMDBSPtr mDB;
      SyncWriterLock mWLock;

     public:

      explicit SyncWriter(const LMDBSPtr& db)
        :mMutex(), mDB(db)
      {
      }

      const LMDBSPtr& getDatabase()
      {
        return mDB;
      }

      bool write(const DBKey& key, PQueueDataType& ref)
      {
        SyncLock sync(mMutex);
        itc::getLog()->trace(__FILE__, __LINE__, "Writing to LMDB synchronously ...");
        bool result;
        mWLock.use(ref, key,
          [&result, this](const MDB_val& key, const MDB_val & data){
            try
            {
              WOTransaction wTxn(mDB);
              if(wTxn.put(key, data))
              {
                result = true;
              }
              result = false;
            }catch(const TITCException<exceptions::MDBGeneral>& e)
            {
              itc::getLog()->fatal(__FILE__, __LINE__, "Exception on write to the database %s: %s. Bailing out!", mDB.get()->getName().c_str(), e.what());
              throw;
            }
          }
        );
        return result;
      }

      bool del(const DBKey& key)
      {
        SyncLock sync(mMutex);
        itc::getLog()->trace(__FILE__, __LINE__, "Deleting the key ...");
        bool result;
        mWLock.use(key,
          [&result, this](const MDB_val & _key){
            try
            {
              WOTransaction wTxn(mDB);
              if(wTxn.del(_key))
              {
                result = true;
              }
              else
              {
                result = false;
              }
            }catch(const std::exception& e)
            {
              itc::getLog()->fatal(__FILE__, __LINE__, "Exception on write to the database %s: %s. Bailing out!", mDB.get()->getName().c_str(), e.what());
              throw;
            }
          }
        );
        return result;
      }

    };
    typedef std::shared_ptr<SyncWriter> LMDBSWriterSPtr;
  }
}

#endif	/* __SYNCWRITER_H__ */

