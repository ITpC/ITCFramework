/* 
 * File:   LMDBWriter.h
 * Author: pk
 *
 * Created on 22 Март 2015 г., 13:44
 */

#ifndef LMDBWRITER_H
#  define	LMDBWRITER_H

#  include <abstract/Runnable.h>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <memory>
#  include <queue>
#  include <algorithm>
#  include <functional>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBWOTxn.h>
#  include <LMDBWObject.h>
#  include <abstract/IController.h>
#  include <sys/AtomicBool.h>
#  include <sys/AtomicDigital.h>
#  include <map>
#  include <stdint.h> 


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
      sys::Mutex mQueueProtect;
      sys::Mutex mWriteProtect;
      sys::Semaphore mQEvent;
      LMDBSPtr mDB;
      WObjectsQueue mQueue;
      sys::AtomicBool mayDestroy;
      sys::AtomicBool mayRun;
      SubscribersMap mSMap;

     public:

      explicit DBWriter(const LMDBSPtr& ref)
        : mQueueProtect(), mWriteProtect(), mDB(ref),
        mayDestroy(false), mayRun(true)
      {
        itc::getLog()->info("DBWriter::DBWriter(): Starting a database writer for database %s", mDB.get()->getName().c_str());
      }
      explicit DBWriter(const DBWriter&) = delete;
      explicit DBWriter(DBWriter&) = delete;

      void write(const WObjectSPtr& ref, const ViewTypeSPtr& vref)
      {
        if(!mayRun)
        {
          throw TITCException<exceptions::MDBWriteFailed>(exceptions::MDBGeneral);
        }
        sys::SyncLock synchronize(mWriteProtect);
        uint64_t key;
        memcpy(&key, ref.get()->key.mv_data, sizeof(uint64_t));
        mSMap.insert(SubscriberDescriptor(key, vref));
        push(ref);
        mQEvent.post();
        //mQEvent.post();
        //mQEvent.post();
      }
      
      const size_t size()
      {
        sys::SyncLock dosync(mQueueProtect);
        return mQueue.size();        
      }
      
      void execute()
      {
        while(mayRun)
        {
          //itc::getLog()->debug(__FILE__, __LINE__, "DBWriter::execue(): qDepth %ju; SemDepth: %ju", mQueue.size(), mQEvent.getValue());
          mQEvent.wait();
          if(empty())
          {
            // throw TITCException<exceptions::QueueOutOfSync>(exceptions::ITCGeneral); // I don't care.
          }
          try
          {
            dbWrite();
          }catch(TITCException<exceptions::MDBKeyNotFound>& e)
          {
            itc::getLog()->debug(__FILE__, __LINE__, "Ignoring exception %s.", e.what());
          }
        }
        // write all remaining messages
        try
        {
          while(!empty())
          {
            dbWrite();
          }
        }catch(std::exception& e)
        {
          //log all another exceptions; can't do anything now 
          itc::getLog()->error(__FILE__, __LINE__, "Exception writing remaining messages into LMDB: %s", e.what());
        }
        mayDestroy = true;
      }

      void onCancel()
      {
        wait_save();
      }

      void shutdown()
      {
        wait_save();
      }

      ~DBWriter()
      {
        itc::getLog()->info("[trace] in -> DBWriter::~DBWriter(): Writing remaining objects into database %s", mDB.get()->getName().c_str());
        wait_save();
        itc::getLog()->info("[trace] out <- DBWriter::~DBWriter(). All remining objects are written into database.");
      }

      const LMDBSPtr& getDatabase() const
      {
        return mDB;
      }

     
     private:

      void push(const WObjectSPtr& ref)
      {
        sys::SyncLock dosync(mQueueProtect);
        mQueue.push(ref);
      }

      void pop()
      {
        sys::SyncLock dosync(mQueueProtect);
        mQueue.pop();
      }

      const WObjectSPtr& front()
      {
        sys::SyncLock dosync(mQueueProtect);
        return mQueue.front();
      }

      bool empty()
      {
        sys::SyncLock dosync(mQueueProtect);
        return mQueue.empty();
      }      

      void wait_save()
      {
        mayRun = false;
        while(!mayDestroy)
        {
          sched_yield();
        }
      }

      void dbWrite()
      {
        if(empty())
        {
          return;
        }
        WObject* ptr = front().get();
        uint64_t key;
        if(ptr)
        {
          try
          {
            WOTransaction aTxn(mDB);
            memcpy(&key, ptr->key.mv_data, sizeof(uint64_t));
            switch(ptr->theOP){
              case ADD:
                if(aTxn.put(ptr->key, ptr->data))
                {
                  sys::SyncLock synchronize(mWriteProtect);
                  SubscribersMap::iterator it = mSMap.find(key);
                  if(it != mSMap.end())
                  {
                    notify(Model(true, it->first), it->second);
                    mSMap.erase(it);
                  }
                }
                break;
              case DEL:
                if(aTxn.del(ptr->key))
                {
                  sys::SyncLock synchronize(mWriteProtect);
                  SubscribersMap::iterator it = mSMap.find(key);
                  if(it != mSMap.end())
                  {
                    notify(Model(true, it->first), it->second);
                    mSMap.erase(it);
                  }
                }
                break;
              default:
                throw TITCException<exceptions::ITCGeneral>(exceptions::IndexOutOfRange);
            }
            pop();
          }catch(TITCException<exceptions::MDBKeyNotFound>& e)
          {
            sys::SyncLock synchronize(mWriteProtect);
            if(mSMap.find(key) != mSMap.end())
            {
              notify(Model(true, key), mSMap[key]); //  report ok, when the key wasn't found
            }
          }catch(std::exception& e)
          {
            sys::SyncLock synchronize(mWriteProtect);
            if(mSMap.find(key) != mSMap.end())
            {
              notify(Model(false, key), mSMap[key]); // report subscriber about write error
            }
            throw; //rethrow the exception
          }
        }
      }
    };

    typedef std::shared_ptr<DBWriter> DBWriterSPtr;

    class SyncDBWriterAdapter : public abstract::IView<Model>, public std::enable_shared_from_this<SyncDBWriterAdapter>
    {
     private:
      sys::Mutex mWriteMutex;
      sys::Mutex mQueueProtect;
      sys::Semaphore mWaitWriteEnd;
      DBWriterSPtr mDBW;
      std::queue<Model> retQueue;
      sys::AtomicBool mayWrite;

      typedef itc::abstract::IView<Model> ViewModelType;

     public:

      explicit SyncDBWriterAdapter(const DBWriterSPtr& dbref)
        : mWriteMutex(), mQueueProtect(), mDBW(dbref), mayWrite(true)
      {
        sys::SyncLock dosync(mQueueProtect);
        sys::SyncLock synchOnWrite(mWriteMutex);
      }
        
      explicit SyncDBWriterAdapter(const SyncDBWriterAdapter&) = delete;
      explicit SyncDBWriterAdapter(SyncDBWriterAdapter&) = delete;

      /**
       * @brief simulates synchronous write
       * @param ref
       */
      void write(const WObjectSPtr& ref)
      {
        if(mayWrite)
        {
          try
          {
            sys::SyncLock synchOnWrite(mWriteMutex);
            mDBW.get()->write(ref, shared_from_this());
          }catch(...)
          {
            itc::getLog()->error(__FILE__,__LINE__,"Unexpected exception");
            throw;
          }
          mWaitWriteEnd.wait();
          try // push stack
          {
            sys::AtomicBool result(front().first);
            pop();
            if(result == false)
            {
              itc::getLog()->error(__FILE__, __LINE__, "Write operation failed for key %ju", retQueue.front().second);
              throw TITCException<exceptions::CanNotWrite>(exceptions::ITCGeneral);
            }
          }catch(...) // rethrow
          {
            throw;
          }
        }else
          throw TITCException<exceptions::CanNotWrite>(exceptions::ITCGeneral);
      }

      void onUpdate(const Model& ref)
      {
        push(ref);
        mWaitWriteEnd.post();
      }

      ~SyncDBWriterAdapter()
      {
        mayWrite = false;
        sys::SyncLock dosync(mQueueProtect);
        sys::SyncLock synchOnWrite(mWriteMutex);
      }
     private:

      void push(const Model& ref)
      {
        sys::SyncLock dosync(mQueueProtect);
        retQueue.push(ref);
      }

      void pop()
      {
        sys::SyncLock dosync(mQueueProtect);
        retQueue.pop();
      }

      const Model& front()
      {
        sys::SyncLock dosync(mQueueProtect);
        return retQueue.front();
      }

      bool empty()
      {
        sys::SyncLock dosync(mQueueProtect);
        return retQueue.empty();
      }
    };
  }
}




#endif	/* LMDBWRITER_H */

