/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: SyncWriterAdapter.h 27 Март 2015 г. 17:43 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __SYNCWRITERADAPTER_H__
#  define	__SYNCWRITERADAPTER_H__
#  include <LMDBWriter.h>
#  include <map>
#  include <memory>
#  include <mutex>
#  include <atomic>

namespace itc
{
  namespace lmdb
  {
    typedef std::shared_ptr<DBWriter> DBWriterSPtr;

    class SyncDBWriterAdapter : public abstract::IView<Model>, public std::enable_shared_from_this<SyncDBWriterAdapter>
    {
     private:
      std::mutex mWriteMutex;
      std::mutex mQueueProtect;
      sys::Semaphore mWaitWriteEnd;
      DBWriterSPtr mDBW;
      std::queue<Model> retQueue;
      std::atomic<bool> mayWrite;
      std::atomic<bool> TxnActive;

      typedef itc::abstract::IView<Model> ViewModelType;

     public:

      explicit SyncDBWriterAdapter(const DBWriterSPtr& dbref)
        : mWriteMutex(), mQueueProtect(), mDBW(dbref), mayWrite(true),
          TxnActive(false)
      {
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        std::lock_guard<std::mutex> synchOnWrite(mWriteMutex);
        itc::getLog()->trace(__FILE__,__LINE__,"[SyncDBWriterAdapter] is started at address: %jx",this);
      }

      explicit SyncDBWriterAdapter(const SyncDBWriterAdapter&) = delete;
      explicit SyncDBWriterAdapter(SyncDBWriterAdapter&) = delete;

      /**
       * @brief simulates synchronous write
       * @param ref
       */
      void write(const QueueMessageSPtr& ref)
      {
        if(mayWrite)
        {
          try
          {
            std::lock_guard<std::mutex> dosync(mWriteMutex);
            TxnActive=true;
            mDBW.get()->write(ref, shared_from_this());
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__, __LINE__, "Unexpected exception %s", e.what());
            throw;
          }
          try{
            mWaitWriteEnd.wait();
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__, __LINE__, "Semaphore is destroyed %s",e.what());
            throw TITCException<exceptions::CanNotWrite>(exceptions::ITCGeneral);
          }
          TxnActive=false;
          try // push stack
          {
            if(!empty())
            {
              std::atomic<bool> result(front().first);
              pop();
              if(result == false)
              {
                itc::getLog()->error(__FILE__, __LINE__, "Write operation failed for key %jx:%jx", front().second.left,front().second.right);
                throw TITCException<exceptions::CanNotWrite>(exceptions::ITCGeneral);
              }
            }
          }catch(...) // rethrow
          {
            throw;
          }
        }else
        {
          itc::getLog()->error(__FILE__, __LINE__, "SyncWriterAdapter going down");
          throw TITCException<exceptions::CanNotWrite>(exceptions::ITCGeneral);
        }
      }

      void onUpdate(const Model& ref)
      {
        push(ref);
        mWaitWriteEnd.post();
      }

      ~SyncDBWriterAdapter()
      {
        mayWrite = false;
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        std::lock_guard<std::mutex> synchOnWrite(mWriteMutex);
        while(TxnActive&&mWaitWriteEnd.getValue()>0)
        {
          sched_yield();
        }
        itc::getLog()->trace(__FILE__,__LINE__,"[SyncDBWriterAdapter] at address: %jx is down",this);
      }
     private:

      void push(const Model& ref)
      {
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        retQueue.push(ref);
      }

      void pop()
      {
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        retQueue.pop();
      }

      const Model& front()
      {
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        return retQueue.front();
      }

      bool empty()
      {
        std::lock_guard<std::mutex> dosync(mQueueProtect);
        return retQueue.empty();
      }
    };
  }
}

#endif	/* __SYNCWRITERADAPTER_H__ */

