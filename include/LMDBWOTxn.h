/* 
 * File:   LMDBWOTxn.h
 * Author: pk
 *
 * Created on 21 Март 2015 г., 0:51
 */

#ifndef LMDBWOTXN_H
#  define	LMDBWOTXN_H
#  include <memory>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBException.h>
#  include <TSLog.h>


namespace itc
{
  namespace lmdb
  {

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
      MDB_txn* handle;
      bool is_not_aborted;

     public:

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
      explicit WOTxn(const std::shared_ptr<Database>& ref)
        : mDB(ref), is_not_aborted(true)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginWOTxn();
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
        : mDB(ref),is_not_aborted(true)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginROTxn(parent.handle);
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
      template <typename T> const bool put(size_t& key, T& data)
      {
        MDB_val dbkey, dbdata;
        dbkey.mv_size = sizeof(key);
        dbkey.mv_data = &key;
        dbdata.mv_size = sizeof(data);
        dbdata.mv_data = &data;
        int ret = mdb_put(handle, mDB.get()->dbi, &dbkey, &dbdata, 0);
        switch(ret){
          case MDB_MAP_FULL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBMapFull);
          case MDB_TXN_FULL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTxnFull);
          case EACCES:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTEAccess);
          case EINVAL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          case 0:
            return true;
          default:
            break;
        }
        ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::get() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
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
          ::itc::getLog()->debug(__FILE__, __LINE__, "~WOTxn() commit transaction");
          (mDB.get()->mEnvironment.get())->WOTxnCommit(handle);
        }
      }
    };
  }
}

#endif	/* LMDBWOTXN_H */

