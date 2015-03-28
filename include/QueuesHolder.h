/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: QueuesHolder.h 26 Март 2015 г. 12:36 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __QUEUESHOLDER_H__
#  define	__QUEUESHOLDER_H__
#  include <abstract/IMessageListener.h>
#  include <PersistentQueue.h>
#  include <ServiceFacade.h>
#  include <LMDBWriter.h>
#  include <string>
#  include <memory>
#  include <map>

namespace itc
{
  class QueuesHolder : public ServiceFacade
  {
   public:
    typedef itc::PQMessageListener<uint64_t, itc::PersistentQueue> IMLType;
    typedef IMLType::TQueueImpl QueueType;
    typedef IMLType::QueueSharedPtr QueueSPtrType;
    typedef IMLType::QueueWeakPtr QueueWeakPtrType;
    
    typedef std::shared_ptr<itc::lmdb::Database> LMDBSPtr;
    typedef std::shared_ptr<itc::lmdb::DBWriter> LMDBWSPtr;
    
    struct PQueueTriplet
    {
      LMDBSPtr mDatabase;
      LMDBWSPtr mDBWriter;
      QueueSPtrType mQueue;
      explicit PQueueTriplet(const LMDBSPtr& db, const LMDBWSPtr& writer, const QueueSPtrType& queue)
      : mDatabase(db), mDBWriter(writer),mQueue(queue)
      {
        
      }
    };
    
    typedef std::shared_ptr<PQueueTriplet> PQStorableSPtr;
    typedef std::map<std::string, PQStorableSPtr> QueuesMap;
    
   private:
    itc::sys::Mutex mMutex;
    QueuesMap mQueues;

   public:

    explicit QueuesHolder(const std::string& queues_home) : ServiceFacade("QueuesHolder"), mMutex()
    {
    }
    explicit QueuesHolder(const QueuesHolder&) = delete;
    explicit QueuesHolder(QueuesHolder&) = delete;

    void create(const std::string& qname);
    void destroy(const std::string& qname);
    QueueSharedPtrType& find(const std::string& qname)
    {
      
    }


  };
}
#endif	/* __QUEUESHOLDER_H__ */

