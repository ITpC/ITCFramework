/* 
 * File:   LMDBTxn.h
 * Author: pk
 *
 * Created on 19 Март 2015 г., 12:40
 */

#ifndef LMDBTXN_H
#  define	LMDBTXN_H
#  include <memory>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <TSLog.h>

namespace itc
{

  namespace lmdb
  {

    /**
     * @brief Read-only transaction. Use the instances of this class on stack
     * or wrap within std::shared_ptr. The transaction begins with instantiation
     * of the object and lasts until the object is destroyed. The only NOTLS
     * concept of LMDB environment is supported.
     **/
    class ROTxn
    {
     private:
      MDB_txn* handle;
      std::shared_ptr<Database> mDB;
      std::shared_ptr<Environment> mDBEnv;
     public:

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
      explicit ROTxn(const std::shared_ptr<Database>& ref)
        : mDB(ref),mDBEnv(ref.get()->mEnvironment)
      {
        try
        {
            handle = mDBEnv->beginROTxn();
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       * @param parent - parent transaction
       **/
      explicit ROTxn(const std::shared_ptr<Database>& ref, const ROTxn& parent)
        : mDB(ref),mDBEnv(ref.get()->mEnvironment)
      {
        try
        {
            handle = mDBEnv->beginROTxn(parent.handle);
        }catch(std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief forbid the copy constructor
       * 
       * @param ref - treference
       **/
      ROTxn(const ROTxn& ref) = delete;

      /**
       * @brief get the value from the database related to provided key
       * 
       * @param key - the key
       * @param result - storage for the value
       * 
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBClosed)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::InvalidException)
       **/
      template <typename T> bool get(const size_t& key, T& result)
      {
        MDB_val data;
        MDB_val dbkey;
        
        dbkey.mv_size=sizeof(key);
        dbkey.mv_data=(void*)(&key);

        int ret = mdb_get(handle, mDB->dbi, &dbkey, &data);

        switch(ret)
        {
          case MDB_NOTFOUND:
          {
            return false;
          }
          case EINVAL:
          {
            itc::getLog()->error(__FILE__, __LINE__, "ROTxn::get() either the key is invalid or something is wrong with source code, that is handling this method");
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          }
          case 0:
            memcpy(((void*) &result), data.mv_data, data.mv_size);
            return true;
          default:
            itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::get() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
            throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
        }
      }

      /**
       * @brief dtor, - the transactin handle
       * 
       **/
      ~ROTxn()
      {
          mDBEnv->ROTxnCommit(handle);
      }
    };

    /**
     * @brief Write-only transaction. Use the instances of this class on stack
     * or wrap within std::shared_ptr. The transaction begins with instantiation
     * of the object and lasts until the object is destroyed. The only NOTLS
     * concept of LMDB environment is supported. The transaction will prevent
     * any other thread from writing to DB.
     * 
     **/
    class WOTxn
    {
     private:
      std::shared_ptr<Database> mDB;
      std::shared_ptr<Environment> mDBEnv;
      MDB_txn* handle;
      bool is_not_aborted;

     public:

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
      explicit WOTxn(const std::shared_ptr<Database>& ref)
        : mDB(ref), mDBEnv(ref.get()->mEnvironment), is_not_aborted(true)
      {
        try
        {
            handle = mDBEnv->beginWOTxn();
        }catch(std::exception& e)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Transaction may not begin, reason: %s\n", e.what());
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
        : mDB(ref), mDBEnv(ref.get()->mEnvironment), is_not_aborted(true)
      {
        try
        {
            handle = mDBEnv->beginROTxn(parent.handle);
        }catch(std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      template <typename T> const bool put(size_t& key, T& data)
      {
        try
        {
            return mDB.get()->put<T>(key, data, handle);
        }catch(std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Database write operation has failed, reason: %s\n", e.what());
          mDBEnv->WOTxnAbort(handle);
          is_not_aborted = false;
          return false;
        }
      }

      /**
       * @brief forbid copy constructor
       * 
       * @param reference
       **/
      WOTxn(const WOTxn& ref) = delete;

      /**
       * @brief dtor, - the transactin handle
       * 
       **/
      ~WOTxn()
      {
        if(is_not_aborted)
        {
          itc::getLog()->debug(__FILE__, __LINE__, "~WOTxn() commit transaction");
          mDBEnv->WOTxnCommit(handle);
        }
      }
    };
  }
}
#endif	/* LMDBTXN_H */

