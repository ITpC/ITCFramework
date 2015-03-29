/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: PQMessageListener.h 1 2015-03-20 10:53:11Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef __PQMESSAGELISTENER_H__
#  define __PQMESSAGELISTENER_H__

#  include <memory>

#  include <sys/Thread.h>
#  include <abstract/Runnable.h>
#  include <Exceptions.h>
#  include <TSLog.h>
#  include <abstract/QueueInterface.h>
#  include <QueueTxn.h>
#  include <LMDBWObject.h>
#  include <mutex>
#  include <memory>

namespace itc
{

  template <
  typename DataType,
  template <class> class QueueImpl
  > class PQMessageListener : public itc::abstract::IRunnable
  {
   public:
    typedef QueueImpl<DataType> TQueueImpl;
    typedef typename std::shared_ptr<TQueueImpl> QueueSharedPtr;
    typedef typename std::weak_ptr<TQueueImpl> QueueWeakPtr;
    typedef QueueTxn<DataType, lmdb::DEL> QueueTxnDelType;
    typedef std::shared_ptr<QueueTxnDelType> QueueTxnDELSPtr;



   private:
    std::mutex mMutex;
    std::atomic<bool> mayRun;
    QueueWeakPtr mQueue;

   public:

    explicit PQMessageListener(QueueSharedPtr& pQueue)
      : mayRun(false), mQueue(pQueue)
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      if(!(mQueue.lock().get()))
        throw NullPointerException(EFAULT);
      mayRun = true;
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::PQMessageListener(QueueSharedPtr& pQueue)", this);
    }

    void shutdown()
    {
      mayRun = false;
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::shutdown()", this);
    }

    QueueWeakPtr getQueueWeakPtr()
    {
      return mQueue;
    }

    void execute()
    {
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() has been started", this);
      while(mayRun)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        try
        {
          if(TQueueImpl * queue_addr = mQueue.lock().get())
          {
            try
            {
              onMessage(queue_addr->recv());
            }catch(const std::exception& e)
            {
              mayRun = false;
              itc::getLog()->error(__FILE__, __LINE__, "Listener at address %x, has cought an exception on receiving a message: %s", this, e.what());
            }
          }else
          {
            mayRun = false;
            itc::getLog()->error(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() - QueueWeakPtr does not exists anymore, calling oQueueDestroy", this);
          }
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "PQMessageListener::execute() - Queue Transaction is aborted with exception: %s", e.what());
        }
      }
      itc::getLog()->error(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() has been finished", this);
    }

   protected:
    virtual void onMessage(const QueueTxnDELSPtr&) = 0;
    virtual void onQueueDestroy() = 0;

    ~PQMessageListener()
    {
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "~PQMessageListener()", this);
    }
  };
}

#endif /*__PQMESSAGELISTENER_H__*/
