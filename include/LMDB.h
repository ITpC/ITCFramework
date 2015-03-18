/* 
 * File:   LMDB.h
 * Author: pk
 *
 * Created on 12 Март 2015 г., 17:07
 */

#ifndef LMDB_H
#  define	LMDB_H
#  include <string>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <ServiceFacade.h>
#  include <LMDBEnv.h>
#  include <lmdb.h>
#  include <LMDBException.h>

namespace itc
{
  namespace lmdb
  {

    class Database : public ServiceFacade
    {
     private:
      sys::Mutex mL2Mutex;
      std::shared_ptr<Environment> mEnvironment;
      std::string mDBName;
      MDB_dbi dbi;
      MDB_txn *txn;
      bool canSet;

     public:

      /**
       * @brief Symas LMDB wrapper around DB functionality
       * 
       * @param env_path - path do mdb environment
       * @param db_name - the name of the database
       * 
       * @exception TITCException<exceptions::ExternalLibraryException>(errno)
       * @exception TITCException<exceptions::MDBEAgain>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBEAccess>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBNotFound>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBInvalid>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBVersionMissmatch>(exceptions::ExternalLibraryException)
       **/
      explicit Database()
        : ServiceFacade("itc::lmdb::Database"),
        mL2Mutex(),canSet(true)
      {
      }
      
        void setInstancePath(const std::string& env_path, const std::string& db_name)
        {
          if(canSet)
          {
            mEnvironment=std::make_shared<Environment>(env_path,1);
            mDBName=db_name;
            canSet=false;
          }
        }
      
      /**
       * @brief implementation of onStart of ServiceFacade. This method opens the LMDB database
       * or throws an exception.
       **/
      void onStart()
      {
        sys::SyncLock synchronize(mL2Mutex);
        if(!canSet) throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEAgain);
        int ret = mdb_txn_begin(mEnvironment.get()->getEnv(), NULL, 0, &txn);
        if(ret) LMDBExceptionParser onTxnOpen(ret);

        ret = mdb_dbi_open(txn, mDBName.c_str(), 0, &dbi);
        if(ret) LMDBExceptionParser onDbiOpen(ret);
      }

      void onStop()
      {
        sys::SyncLock synchronize(mL2Mutex);
        mdb_txn_commit(txn);
        mdb_close(mEnvironment.get()->getEnv(), dbi);
      }

      void onDestroy()
      {

      }

      ~Database()
      {
        mdb_txn_commit(txn);
        mdb_close(mEnvironment.get()->getEnv(), dbi);
      }
    };
  }
}


#endif	/* LMDB_H */

