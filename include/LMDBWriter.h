/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: LMDBWriter.h 1 2015-03-22 13:44:11Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/



#ifndef LMDBWRITER_H
#  define	LMDBWRITER_H

#  include <abstract/Runnable.h>
#  include <memory>
#  include <queue>
#  include <mutex>
#  include <algorithm>
#  include <functional>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBWOTxn.h>
#  include <LMDBWObject.h>
#  include <abstract/IController.h>
#  include <map>
#  include <stdint.h> 
#  include <atomic>
#  include <limits>


namespace itc
{
  namespace lmdb
  {
    typedef std::pair<bool, uint64_t> Model;
    typedef std::shared_ptr<itc::lmdb::Database> LMDBSPtr;
    typedef std::shared_ptr<itc::lmdb::WObject> WObjectSPtr;

    class DBWriter : public ::itc::abstract::IRunnable, ::itc::abstract::IController<Model>
    {
     public:
      typedef std::queue<WObjectSPtr> WObjectsQueue;
      typedef itc::lmdb::WOTxn WOTransaction;
      typedef std::map<uint64_t, ViewTypeSPtr> SubscribersMap;
      typedef std::pair<uint64_t, ViewTypeSPtr> SubscriberDescriptor;

     private:
      std::mutex mWriteProtect;
      std::mutex mServiceQMutex;
      std::recursive_mutex mWorkQueueMutex;
      LMDBSPtr mDB;
      WObjectsQueue mServiceQueue;
      WObjectsQueue mWorkQueue;
      std::atomic<bool> mayRun;
      SubscribersMap mSMap;
      sys::Semaphore mQEvent;
      std::list<uint64_t> mTrackKeys;

     public:

      explicit DBWriter(const LMDBSPtr& ref)
      : mWriteProtect(), mServiceQMutex(), mWorkQueueMutex(), 
        mDB(ref), mayRun(true),mQEvent()
      {
        itc::getLog()->info("DBWriter::DBWriter(): Starting a database writer for database %s", mDB.get()->getName().c_str());
      }
      explicit DBWriter(const DBWriter&) = delete;
      explicit DBWriter(DBWriter&) = delete;

      void write(const WObjectSPtr& ref, const ViewTypeSPtr& vref)
      {
        std::lock_guard<std::mutex> dosync(mServiceQMutex);
        if(!mayRun)
        {
          throw TITCException<exceptions::MDBWriteFailed>(exceptions::MDBGeneral);
        }else
        {
          std::lock_guard<std::mutex> maplock(mWriteProtect);
          uint64_t key;
          memcpy(&key, ref.get()->key.mv_data, sizeof(uint64_t));
          mSMap.insert(SubscriberDescriptor(key, vref));
          mServiceQueue.push(ref);
          mTrackKeys.push_back(key);
          mQEvent.post();
        }
      }

      const size_t service_queue_depth()
      {
        std::lock_guard<std::mutex> dosync(mServiceQMutex);
        return mServiceQueue.size();
      }

      void execute()
      {
        mQEvent.wait();
        while(mayRun)
        {
          std::lock_guard<std::recursive_mutex> dosync(mWorkQueueMutex);
          try
          {
            std::lock_guard<std::mutex> dosync(mServiceQMutex);
            while(!mServiceQueue.empty())
            {
              mWorkQueue.push(mServiceQueue.front());
              mServiceQueue.pop();
            }
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__, __LINE__, "Unexpected error: %s", e.what());
            itc::getLog()->fatal(__FILE__, __LINE__, "[666]: Queues are out of order. DBWriter at address %jx is shutdown.", this);
            mayRun = false;
          }
          try
          {
            processWorkQueue();
          }catch(const TITCException<exceptions::MDBKeyNotFound>& e)
          {
            itc::getLog()->debug(__FILE__, __LINE__, "Ignoring exception %s.", e.what());
          }
        }
      }

      void onCancel()
      {
        shutdown();
      }

      void shutdown()
      {
        itc::getLog()->info("[trace] -> DBWriter::shutdown() for database %s is in progress",mDB.get()->getName().c_str());
        mayRun=false;
        sys::Nap s;
        while((mQEvent.getValue()>0)&&(service_queue_depth()>0))
        {
          s.usleep(100000);
        }
      }

      ~DBWriter()
      {
        shutdown();
        std::lock_guard<std::recursive_mutex> syncwq(mWorkQueueMutex);
        std::lock_guard<std::mutex> dosync(mServiceQMutex);
        std::lock_guard<std::mutex> maplock(mWriteProtect);
        itc::getLog()->info("[trace] out <- DBWriter for database %s is down",mDB.get()->getName().c_str());
      }

      const LMDBSPtr& getDatabase() const
      {
        return mDB;
      }


     private:

      void processWorkQueue()
      {
        std::lock_guard<std::recursive_mutex> dosync(mWorkQueueMutex); // be overprotective about memory barriers
        while(!mWorkQueue.empty())
        {
          uint64_t key=std::numeric_limits<uint64_t>::max();
          WObjectSPtr ptr(mWorkQueue.front());
          mWorkQueue.pop();
          if(ptr.get() == nullptr)
          {
            itc::getLog()->error(__FILE__, __LINE__, "Invalid shared pointer to writable object has been found in DBWriter's processing queue");
            
            throw TITCException<exceptions::ITCGeneral>(exceptions::NullPointerException);
          }
          memcpy(&key, ptr->key.mv_data, sizeof(uint64_t));
          try
          {
            WOTransaction aTxn(mDB);
            switch(ptr->theOP){
              case ADD:
                if(aTxn.put(ptr->key, ptr->data))
                {
                  notify(key,true);
                }
                break;
              case DEL:
                if(aTxn.del(ptr->key))
                {
                  notify(key,true);
                }
                break;
              default:
                itc::getLog()->error(__FILE__, __LINE__, "Unexpected instruction for transactions processing. Kick the programmer in the ass!");
                throw TITCException<exceptions::IndexOutOfRange>(exceptions::ITCGeneral);
            }
          }catch(const TITCException<exceptions::MDBKeyNotFound>& e)
          {
            notify(key,true);
            throw;
          }catch(const std::exception& e)
          {
            notify(key,false);
            throw; //rethrow the exception
          }
        }
      }
      
      void notify(const uint64_t key, const bool& res)
      {
        std::lock_guard<std::mutex> maplock(mWriteProtect);

        SubscribersMap::iterator it = mSMap.find(key);
        if(it != mSMap.end())
        {
          static_cast<::itc::abstract::IController<Model>*>(this)->notify(Model(res, it->first), it->second);
          mSMap.erase(it);
        }else
        {
          logSubscribers(key);
          throw TITCException<exceptions::IndexOutOfRange>(exceptions::ITCGeneral);
        }
      }
      
      void logSubscribers(const uint64_t& key)
      {
        itc::getLog()->error(__FILE__, __LINE__, "There is no owner for transaction with the key: %ju", key);
        itc::getLog()->error(__FILE__, __LINE__, "See the following list of subscribers.");
        std::for_each(mSMap.begin(), mSMap.end(),
          [](const SubscriberDescriptor & desc){
            itc::getLog()->error(__FILE__, __LINE__, "Key %ju , subscriber %jx", desc.first, desc.second.get());
          }
        );
        size_t count = 0;
        std::for_each(mTrackKeys.begin(), mTrackKeys.end(),
          [&count, &key](const uint64_t refkey){
            if(refkey == key)
            {
              itc::getLog()->error(__FILE__, __LINE__, "The key %ju, is recorded in the tracklist, at position %ju", refkey, count);
            }
            ++count;
          }
        );
        itc::getLog()->flush();
      }
    };
    typedef std::shared_ptr<DBWriter> DBWriterSPtrType;
  }
}




#endif	/* LMDBWRITER_H */

