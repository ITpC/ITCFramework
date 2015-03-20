/* 
 * File:   LMDBROTxn.h
 * Author: pk
 *
 * Created on 21 Март 2015 г., 0:51
 */

#ifndef LMDBROTXN_H
#  define	LMDBROTXN_H
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
     * @brief Read-only transaction object. Use the instances of this class on 
     * stack. Never share this object between the threads. Every thread must 
     * allocate those objects in its own space and never share the objects with 
     * others. The transaction begins with instantiation of the object and lasts
     * until the object is destroyed. 
     **/
    class ROTxn
    {
     private:
      MDB_txn* handle;
      std::shared_ptr<Database> mDB;
     public:

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
      explicit ROTxn(const std::shared_ptr<Database>& ref)
        : mDB(ref)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginROTxn();
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
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
        : mDB(ref)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginROTxn(parent.handle);
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

        dbkey.mv_size = sizeof(key);
        dbkey.mv_data = (void*) (&key);

        int ret = mdb_get(handle, mDB->dbi, &dbkey, &data);

        switch(ret){
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
        (mDB.get()->mEnvironment.get())->ROTxnCommit(handle);
      }
    };
  }
}

#endif	/* LMDBROTXN_H */

