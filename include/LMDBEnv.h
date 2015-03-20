/* 
 * File:   LMDBEnv.h
 * Author: pk
 *
 * Created on 13 Март 2015 г., 14:10
 */

#ifndef LMDBENV_H
#  define	LMDBENV_H
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <ITCException.h>
#  include <LMDBException.h>
#  include <TSLog.h>

#  include <lmdb.h>
#  include <memory>
#  include <queue>

namespace itc
{
  namespace lmdb
  {
    /**
     * @brief wrapper around Symas LMDB. Environment setup.
     **/
    class Environment
    {
     public:
      typedef std::shared_ptr<MDB_stat> MDBEnvStats;
      typedef std::shared_ptr<MDB_envinfo> MDBEnvInfo;
     private:
      itc::sys::Mutex mMutex;
      std::string mPath;
      MDB_env *mEnv;
      MDB_dbi mDbs;
      bool mActive;
      MDBEnvStats mStats;
      MDBEnvInfo mInfo;
      std::queue<MDB_txn*> mROTxnsPool;

     public:

      /**
       * @brief constructor for LMDB Environment 
       * 
       **/
      explicit Environment(const std::string& path, const int& dbs)
        : mMutex(), mPath(path), mEnv((MDB_env*) 0), mDbs(dbs),
        mActive(false), mStats(std::make_shared<MDB_stat>())
      {
        itc::sys::SyncLock synchronize(mMutex);
        //        itc::sys::RecursiveSyncLock (mWOMutex);
        // Create an LMDB environment handle.
        int ret = mdb_env_create(&mEnv);
        if(ret)
        {
          itc::getLog()->error(__FILE__, __LINE__, "LMDB: Can not create database environment in %s", mPath.c_str());
          mdb_env_close(mEnv);
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
        ret = mdb_env_set_maxdbs(mEnv, mDbs);
        if(ret)
        {
          itc::getLog()->error(__FILE__, __LINE__, "LMDB: Can not set max_dbs for %s", mPath.c_str());
          LMDBExceptionParser onMaxDBsSet(ret);
        }
        // Open an environment handle
        ret = mdb_env_open(mEnv, mPath.c_str(), MDB_NOTLS, 0664);
        if(ret)
        {
          itc::getLog()->error(__FILE__, __LINE__, "LMDB: Can not open environment %s", mPath.c_str());
          mdb_env_close(mEnv);
          LMDBExceptionParser onEnvOpen(ret);
        }
        mActive = true;
      };

      MDB_env* getEnv()
      {
        itc::sys::SyncLock synchronize(mMutex);
        if(mActive)
        {
          return mEnv;
        }
        throw TITCException<exceptions::MDBClosed>(exceptions::ApplicationException);
      }

      const MDBEnvStats& getStats()
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_stat(mEnv, mStats.get());
        return mStats;
      }

      const MDBEnvInfo& getInfo()
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_info(mEnv, mInfo.get());
        return mInfo;
      }

      void Sync()
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_sync(mEnv, 0);
      }

      void SyncForce()
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_sync(mEnv, 1);
      }

      unsigned int getFlags()
      {
        itc::sys::SyncLock synchronize(mMutex);
        unsigned int flags;
        mdb_env_get_flags(mEnv, &flags);
        return flags;
      }

      void setFlags(const unsigned int flags)
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_set_flags(mEnv, flags, 1);
      }

      void unsetFlags(const unsigned int flags)
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_set_flags(mEnv, flags, 0);
      }

      /**
       * @brief adjusts the mapsize of the environment, if it was changed by
       * external programm. TODO: make an utility to change the mapsize.
       **/
      void adjustMapSize()
      {
        itc::sys::SyncLock synchronize(mMutex);
        mdb_env_set_mapsize(mEnv, 0);
      }

      const bool isActive() const
      {
        return mActive;
      }

      const std::string& getPath() const
      {
        return mPath;
      }

      /** =====================================================================
       * @brief begin read-only transaction
       * 
       * @param parent - a parent transaction (if any)
       * @return transaction handle
       **/
      MDB_txn* beginROTxn(MDB_txn *parent = static_cast<MDB_txn*> (0))
      {
        sys::SyncLock synchronize(mMutex);
        if(isActive())
        {
          MDB_txn *tmp;
          if(mROTxnsPool.empty())
          {
            int ret = mdb_txn_begin(mEnv, parent, MDB_RDONLY, &tmp);
            if(ret) LMDBExceptionParser onTxnBegin(ret);
          }else
          {
            tmp = mROTxnsPool.front();
            mROTxnsPool.pop();
            int ret = mdb_txn_renew(tmp);
            if(ret) LMDBExceptionParser onTxnRenew(ret);
          }
          return tmp;
        }else
        {
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEInval);
        }
      }

      /** =====================================================================
       * @brief commit (reset) RO transaction
       * 
       * @param prt - a transaction handle
       **/
      void ROTxnCommit(MDB_txn *ptr)
      {
        sys::SyncLock synchronize(mMutex);
        if(ptr)
        {
          MDB_env *env = mdb_txn_env(ptr);
          if(mEnv == env)
          {
            mdb_txn_reset(ptr);
            mROTxnsPool.push(ptr);
          }else
          {
            throw TITCException<exceptions::ITCGeneral>(exceptions::MDBEnvWrong);
          }
        }else throw TITCException<exceptions::ITCGeneral>(exceptions::MDBTxnNULL);
      }

      /**
       * @brief begin write-only transaction
       * @param parent a parent transaction
       * @return 
       */
      MDB_txn* beginWOTxn(MDB_txn *parent = static_cast<MDB_txn*> (0))
      {
        itc::sys::SyncLock synchronize(mMutex);
        itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> in Database::beginWOTxn()");

        MDB_txn *tmp;
        int ret = mdb_txn_begin(mEnv, parent, 0, &tmp);
        if(ret) LMDBExceptionParser onTxnBegin(ret);
        itc::getLog()->debug(__FILE__, __LINE__, "[trace] <- normal out of Database::beginWOTxn()");
        return tmp;
      }

      void WOTxnCommit(MDB_txn *ptr)
      {
        itc::sys::SyncLock synchronize(mMutex);
        if(ptr)
        {
          MDB_env *env = mdb_txn_env(ptr);
          if(mEnv == env)
          {
            int ret = mdb_txn_commit(ptr);
            switch(ret){
              case EINVAL:
                throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEInval);
              case ENOSPC:
                throw TITCException<exceptions::ITCGeneral>(ENOSPC);
              case EIO:
                throw TITCException<exceptions::ITCGeneral>(EIO);
              case ENOMEM:
                throw TITCException<exceptions::ITCGeneral>(ENOMEM);
              case 0:
                return;
              default:
                ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]: in Database::WOTxnCommit() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
                throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
            }
          }else
          {
            throw TITCException<exceptions::ITCGeneral>(exceptions::MDBEnvWrong);
          }
        }else
        {
          throw TITCException<exceptions::ITCGeneral>(exceptions::MDBEInval);
        }
      }

      void WOTxnAbort(MDB_txn *ptr)
      {
        itc::sys::SyncLock synchronize(mMutex);
        if(ptr)
        {
          mdb_txn_abort(ptr);
        }
      }

      /*=====================================================================*/
      ~Environment()
      {
        itc::sys::SyncLock synchronize(mMutex);
        this->close();
      }
     private:

      void close()
      {
        if(mActive)
        {
          while(!mROTxnsPool.empty())
          {
            mdb_txn_abort(mROTxnsPool.front());
            mROTxnsPool.pop();
          }
          mdb_env_close(mEnv);
          mActive = false;
        }
      }
    };
  }
}



#endif	/* LMDBENV_H */

