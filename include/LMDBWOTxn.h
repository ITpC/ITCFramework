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
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBException.h>
#  include <TSLog.h>
#  include <stdint.h>
#  include <atomic>
#  include <mutex>

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
        :mMutex(), mDB(ref), is_not_aborted(true)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        try
        {
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] in -> Transaction ctor, before begin", pthread_self());
          handle = (mDB.get()->mEnvironment.get())->beginWOTxn();
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- Transaction ctor, got the handle", pthread_self());
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
        :mMutex(), mDB(ref), is_not_aborted(true)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        try
        {
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] in -> Transaction ctor, before begin", pthread_self());
          handle = (mDB.get()->mEnvironment.get())->beginWOTxn(parent.handle);
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- Transaction ctor, got the handle", pthread_self());
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
      bool put(const MDB_val& key, const MDB_val& data)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] in -> Write to database WOTxn::put()", pthread_self());
        int ret = mdb_put(handle, mDB.get()->dbi, (MDB_val*)(&key), (MDB_val*)(&data), 0);
        if(ret == 0)
        {
          is_not_aborted = true;
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- Write to database WOTxn::put()", pthread_self());
          return true;
        }
        else
        {
          switch(ret)
          {
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
            default:
              is_not_aborted = false;
              break;
          }
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() something is generally wrong. This message should never appear in the log.");
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:WOTxn::put() continue: Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }

      void abort()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] in -> WOTxn::abort()", pthread_self());
        is_not_aborted = false;
        (mDB.get()->mEnvironment.get())->WOTxnAbort(handle);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- WOTxn::abort(), TRANSACTION ABORTED", pthread_self());
      }

      const bool del(const MDB_val& dbkey)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[[%jx] in -> WOTxn::del()", pthread_self());
        int ret = mdb_del(handle, mDB.get()->dbi, (MDB_val*)(&dbkey), NULL);
        if(ret == 0)
        {
          is_not_aborted = true;
          ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- WOTxn::del()", pthread_self());
          return true;
        }
        else
        {
          switch(ret)
          {
            case MDB_NOTFOUND:
              is_not_aborted = false;
              ::itc::getLog()->error(__FILE__, __LINE__, "[%jx] WOTxn::del(), key %jx:%jx is not found ", pthread_self(), ((uint64_t*)(dbkey.mv_data))[0], ((uint64_t*)(dbkey.mv_data))[1]);
              return false;
            case EACCES:
              is_not_aborted = false;
              throw TITCException<exceptions::MDBTEAccess>(exceptions::MDBGeneral);
            case EINVAL:
              is_not_aborted = false;
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            default:
              break;
          }
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
        (mDB.get()->mEnvironment.get())->WOTxnCommit(handle);// commit aborted txns anyway, otherways the LMDB will hang on mutex
        ::itc::getLog()->trace(__FILE__, __LINE__, "[trace] in -> commited intto database, %jx", pthread_self());
      }
    };
  }
}

#endif	/* LMDBWOTXN_H */

