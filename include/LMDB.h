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
#  include <lmdb.h>
#  include <LMDBEnv.h>
#  include <LMDBException.h>
#  include <queue>

namespace itc
{
  namespace lmdb
  {
    class WOTxn;
    class ROTxn;
    
    class Database
    {
     private:
      sys::Mutex mMutex;
      sys::RecursiveMutex mWOMutex;
      std::string mDBEnvPath;
      std::shared_ptr<Environment> mEnvironment;
      std::string mDBName;
      int mDBMode;
      MDB_dbi dbi;
      MDB_txn *txn;
      bool isopen;
      friend class ROTxn;
      friend class WOTxn;

     public:

      /**
       * @brief Symas LMDB wrapper around DB functionality as a backend for 
       * persistent queues[WIP, nothing good yet here]
       *
       * @param path - LMDB database environment path
       * @param dbname - name of the database (optional)
       * @param mode - database open mode and settings (optional, default: create
       * database if it is not exists, the keys are of size_t type
       **/
      explicit Database(
        const std::string& path,
        const std::string& dbname="",
        const int mode=MDB_CREATE | MDB_INTEGERKEY
      )
      : mMutex(),mWOMutex(),mDBEnvPath(path),
        mDBMode(mode),isopen(false)
      {
        sys::SyncLock synchronize(mMutex);
       
        if(dbname.empty())
        {
          mEnvironment = std::make_shared<Environment>(mDBEnvPath,0);
        }
        else
        {
          mDBName=path+"/"+dbname;
          mEnvironment = std::make_shared<Environment>(mDBEnvPath,1);
        }
      }

      Database(const Database&) = delete;
      /**
       * @brief create the environment and open the database
       **/
      void open()
      {
        itc::getLog()->debug(__FILE__,__LINE__,"[trace] -> Database::open()");
        sys::SyncLock synchRO(mMutex);
        sys::RecursiveSyncLock synchWO(mWOMutex);
      
        if(isopen)
        {
          itc::getLog()->debug(__FILE__,__LINE__,"[trace] -> Database::open(), database is already open");
          return;
        }
        
        int ret = mdb_txn_begin(mEnvironment.get()->getEnv(), NULL, 0, &txn);
        if(ret) LMDBExceptionParser onTxnOpen(ret);
        if(mDBName.empty())
        {
          ret = mdb_dbi_open(txn, NULL, mDBMode , &dbi);
        }
        else
        {
          ret = mdb_dbi_open(txn, mDBName.c_str(), mDBMode , &dbi);
        }
        if(ret) LMDBExceptionParser onDbiOpen(ret);
        mdb_txn_commit(txn);
        isopen=true;
      }
      
      void shutdown()
      {
        sys::SyncLock synchRO(mMutex);
        sys::RecursiveSyncLock synchRW(mWOMutex);       
      }

      ~Database()
      {
        this->shutdown();
      }
      
     private:
     

      template <typename T> const bool put(size_t& key, T& data, MDB_txn*& handle)
      {
        sys::RecursiveSyncLock synchronize(mWOMutex);
        MDB_val dbkey,dbdata;
        dbkey.mv_size=sizeof(key);
        dbkey.mv_data=&key;
        dbdata.mv_size=sizeof(data);
        dbdata.mv_data=&data;
        int ret = mdb_put(handle, dbi, &dbkey, &dbdata, 0);
        switch(ret)
        {
          case MDB_MAP_FULL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBMapFull);
          case MDB_TXN_FULL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTxnFull);
          case  EACCES:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTEAccess);
          case EINVAL:
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          case 0:
            return true;
          default:
            break;
        }
        itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::get() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
        throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
      }
    };
  }
}


#endif	/* LMDB_H */

