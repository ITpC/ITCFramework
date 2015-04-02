/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: LMDBWOTxn.h 21 Март 2015 г. 0:51 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef LMDBWOTXN_H
#  define	LMDBWOTXN_H
#  include <memory>
#  include <LMDB.h>
#  include <LMDBEnv.h>
#  include <LMDBException.h>
#  include <TSLog.h>
#  include <stdint.h>
#  include <atomic>
#  include <mutex>
#  include <DBKeyType.h>
#  include <QueueObject.h>


namespace itc
{
  namespace lmdb
  {

    /**
     * @brief Write-only transaction. Use the instances of this class on stack
     * or wrap within std::shared_ptr. The transaction begins with instantiation
     * of the object and lasts until the object is destroyed. The only NOTLS
     * concept of LMDB environment is supported. The transaction will prevent
     * any other thread from writing to DB. Expect deadlocks on write transactions
     * running in parallel threads. Only one writer thread may be allowed.
     * 
     **/
    class WOTxn
    {
     private:
      typedef ::itc::lmdb::Database Database;
      std::mutex mMutex;
      std::shared_ptr<Database> mDB;
      MDB_txn* handle;
      std::atomic<bool> is_not_aborted;

     public:

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
      explicit WOTxn(const std::shared_ptr<Database>& ref)
        : mMutex(), mDB(ref), is_not_aborted(true)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        try
        {
          ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> Transaction ctor, before begin %jx", pthread_self());
          handle = (mDB.get()->getDBEnv().get())->beginWOTxn();
          ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- Transaction ctor, got the handle, %jx", pthread_self());
        }catch(std::exception& e)
        {
          ::itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       * @param parent - parent transaction
       **/
      explicit WOTxn(const std::shared_ptr<Database>& ref, const WOTxn& parent)
        : mMutex(), mDB(ref), is_not_aborted(true)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        try
        {
          ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> Transaction ctor, before begin %jx", pthread_self());
          handle = (mDB.get()->getDBEnv().get())->beginWOTxn(parent.handle);
          ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- Transaction ctor, got the handle, %jx", pthread_self());
        }catch(std::exception& e)
        {
          ::itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief put data into the database (insert or update). The data related
       * the key which is already in the database, will be updated. New key-value
       * pair will be inserted.
       * 
       * @param key - a key
       * @param data - a value
       * @return true on success, exception otherwise.
       **/
      const bool put(const QueueMessageSPtr& ref)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> Write to database WOTxn::put(), %jx", pthread_self());
        MDB_val dbkey, dbdata;
        ref.get()->prep_mdb_key(&dbkey);
        ref.get()->prep_mdb_data(&dbdata);
        
        int ret = mdb_put(handle, mDB.get()->dbi, &dbkey, &dbdata, 0);
        switch(ret){
          case MDB_MAP_FULL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBMapFull>(exceptions::MDBGeneral);
            break;
          case MDB_TXN_FULL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBTxnFull>(exceptions::MDBGeneral);
            break;
          case EACCES:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBTEAccess>(exceptions::MDBGeneral);
            break;
          case EINVAL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBInvalParam>(exceptions::MDBGeneral);
          case 0:
            is_not_aborted=true;
            ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- Write to database WOTxn::put(), %jx", pthread_self());
            return true;
          default:
            break;
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() something is generally wrong. This message should never appear in the log.");
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() continue: Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }

      /**
       * @brief put data into the database (insert or update). The data related
       * the key which is already in the database, will be updated. New key-value
       * pair will be inserted.
       * 
       * @param key - a key
       * @param data - a value
       * @return true on success, exception otherwise.
       **/
      bool put(MDB_val& key, MDB_val& data)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> Write to database WOTxn::put(), %jx", pthread_self());
        int ret = mdb_put(handle, mDB.get()->dbi, &key, &data, 0);
        switch(ret){
          case MDB_MAP_FULL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBMapFull);
            break;
          case MDB_TXN_FULL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTxnFull);
            break;
          case EACCES:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTEAccess);
            break;
          case EINVAL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            break;
          case 0:
            is_not_aborted=true;
            ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- Write to database WOTxn::put(), %jx", pthread_self());
            return true;
          default:
            break;
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() something is generally wrong. This message should never appear in the log.");
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() continue: Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }

      void abort()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> WOTxn::abort(), %jx", pthread_self());
        is_not_aborted = false;
        (mDB.get()->getDBEnv().get())->WOTxnAbort(handle);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- WOTxn::abort(), TRANSACTION ABORTED %jx", pthread_self());
      }

      const bool del(const DBKey& key)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> Write to database WOTxn::del(), %jx", pthread_self());
        MDB_val dbkey;
        dbkey.mv_data = (void*)(&key);
        dbkey.mv_size = sizeof(DBKey);

        int ret = mdb_del(handle, mDB.get()->dbi, &dbkey, NULL);
        switch(ret){
          case MDB_NOTFOUND:
            is_not_aborted = false;
            return false;
          case EACCES:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBTEAccess>(exceptions::MDBGeneral);
            break;
          case EINVAL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            break;
          case 0:
            is_not_aborted=true;
            ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out -> Write to database WOTxn::del(), %jx", pthread_self());
            return true;
          default:
            break;
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() something is generally wrong. This message should never appear in the log.");
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() continue: Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }

      const bool del(MDB_val& dbkey)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> WOTxn::del(), %jx", pthread_self());
        int ret = mdb_del(handle, mDB.get()->dbi, &dbkey, NULL);
        switch(ret){
          case MDB_NOTFOUND:
            is_not_aborted = false;
            return false;
          case EACCES:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBTEAccess>(exceptions::MDBGeneral);
            break;
          case EINVAL:
            is_not_aborted = false;
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            break;
          case 0:
            is_not_aborted=true;
            ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] out <- WOTxn::del(), %jx", pthread_self());
            return true;
          default:
            break;
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::del() something is generally wrong. This message should never appear in the log.");
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::del() continue: Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }

      /**
       * @brief forbid copy constructor
       * 
       * @param reference
       **/
      explicit WOTxn(const WOTxn& ref) = delete;
      explicit WOTxn(WOTxn& ref) = delete;

      /**
       * @brief dtor, - the transactin handle
       * 
       **/
      ~WOTxn() noexcept
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> comit to database, %jx", pthread_self());
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> before comit to database, %jx", pthread_self());
        (mDB.get()->getDBEnv().get())->WOTxnCommit(handle); // commit aborted txns anyway, otherways the LMDB will hang on mutex
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> commited intto database, %jx", pthread_self());
      }
    };
  }
}

#endif	/* LMDBWOTXN_H */

