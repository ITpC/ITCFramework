/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: LMDBEnv.h 1 13 Март 2015 г., 14:10 pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef LMDBENV_H
#  define	LMDBENV_H
#  include <ITCException.h>
#  include <TSLog.h>
#  include <stdint.h>
#  include <lmdb.h>
#  include <memory>
#  include <mutex>
#  include <atomic>
#  include <queue>
#  include <stdint.h>

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
      std::mutex mMutex;
      std::string mPath;
      MDB_env *mEnv;
      MDB_dbi mDbs;
      std::atomic<bool> mActive;
      MDBEnvStats mStats;
      MDBEnvInfo mInfo;
      std::queue<MDB_txn*> mROTxnsPool;
      std::atomic<uint64_t> mMDB_MAP_SIZE;

     public:

      /**
       * @brief constructor for LMDB Environment 
       * 
       **/
      explicit Environment(const std::string& path, const int& dbs, const uint64_t& mdbmapsz=41943040)
        : mMutex(), mPath(path), mEnv((MDB_env*) 0), mDbs(dbs),
        mActive(false), mStats(std::make_shared<MDB_stat>()),mMDB_MAP_SIZE(mdbmapsz)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
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
          throw TITCException<exceptions::MDBGeneral>(ret);
        }
        // mdb_env_set_mapsize
        ret = mdb_env_set_mapsize(mEnv,size_t(mMDB_MAP_SIZE));
        if(ret)
        {
          itc::getLog()->error(__FILE__, __LINE__, "LMDB: Can not enlarge the database %s", mPath.c_str());
          mdb_env_close(mEnv);
          throw TITCException<exceptions::MDBGeneral>(ret);
        }
        // Open an environment handle
        ret = mdb_env_open(mEnv, mPath.c_str(), MDB_NOTLS, 0664);
        if(ret)
        {
          itc::getLog()->error(__FILE__, __LINE__, "LMDB: Can not open environment %s", mPath.c_str());
          mdb_env_close(mEnv);
          throw TITCException<exceptions::MDBGeneral>(ret);
        }
        mActive = true;
      };

      MDB_env* getEnv()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        if(mActive)
        {
          return mEnv;
        }
        throw TITCException<exceptions::MDBClosed>(exceptions::ApplicationException);
      }

      const MDBEnvStats& getStats()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_stat(mEnv, mStats.get());
        return mStats;
      }

      const MDBEnvInfo& getInfo()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_info(mEnv, mInfo.get());
        return mInfo;
      }

      void Sync()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_sync(mEnv, 0);
      }

      void SyncForce()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_sync(mEnv, 1);
      }

      unsigned int getFlags()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        unsigned int flags;
        mdb_env_get_flags(mEnv, &flags);
        return flags;
      }

      void setFlags(const unsigned int flags)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_set_flags(mEnv, flags, 1);
      }

      void unsetFlags(const unsigned int flags)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        mdb_env_set_flags(mEnv, flags, 0);
      }

      /**
       * @brief adjusts the mapsize of the environment, if it was changed by
       * external programm. TODO: make an utility to change the mapsize.
       **/
      void adjustMapSize()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
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
      MDB_txn* beginROTxn(MDB_txn *parent = nullptr)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        if(isActive())
        {
          MDB_txn *tmp;
          if(mROTxnsPool.empty())
          {
            int ret = mdb_txn_begin(mEnv, parent, MDB_RDONLY, &tmp);
            if(ret) throw TITCException<exceptions::MDBGeneral>(ret);
          }else
          {
            tmp = mROTxnsPool.front();
            mROTxnsPool.pop();
            int ret = mdb_txn_renew(tmp);
            if(ret) throw TITCException<exceptions::MDBGeneral>(ret);
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
        std::lock_guard<std::mutex> dosync(mMutex);
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
      MDB_txn* beginWOTxn(MDB_txn *parent = nullptr)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        itc::getLog()->trace(__FILE__, __LINE__, "[trace] -> in Database::beginWOTxn(), [thread]: %jx",pthread_self());
        MDB_txn *tmp;
        int ret = mdb_txn_begin(mEnv, parent, 0, &tmp);
        if(ret) 
        {
          if(ret == MDB_MAP_RESIZED)
          {
            int ret1=mdb_env_set_mapsize(mEnv,0);
            if(ret1) throw TITCException<exceptions::MDBGeneral>(ret1);
          }
          else throw TITCException<exceptions::MDBGeneral>(ret);
        }
        
        if(tmp == nullptr)
        {
          ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]: in Database::beginWOTxn() something is generally wrong. The transaction handle is null.");
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
        itc::getLog()->trace(__FILE__, __LINE__, "[trace] <- out Database::beginWOTxn(), [thread]: %jx",pthread_self());
        return tmp;
      }
      
      void WOTxnCommit(MDB_txn *ptr)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] in -> WOTxnCommit()",pthread_self());
        if(ptr)
        {
          MDB_env *env = mdb_txn_env(ptr);
          if(mEnv == env)
          {
            int ret = mdb_txn_commit(ptr);
            if(ret)
            {
                throw TITCException<exceptions::MDBGeneral>(ret);
            }
            else
            {
              ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] out <- WOTxnCommit()",pthread_self());
              return;
            }
          }else
          {
            ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]: in Database::WOTxnCommit() something is generally wrong. Transaction requested from wrong environment.");
            throw TITCException<exceptions::ITCGeneral>(exceptions::MDBEnvWrong);
          }
        }else
        {
          ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]: in Database::WOTxnCommit() something is generally wrong. The transaction handle is null.");
          throw TITCException<exceptions::ITCGeneral>(exceptions::MDBEInval);
        }
        ::itc::getLog()->trace(__FILE__, __LINE__, "[%jx] <- out WOTxnCommit(),",pthread_self());
      }

      void WOTxnAbort(MDB_txn *ptr)
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        itc::getLog()->trace(__FILE__, __LINE__, "[trace] -> in Database::WOTxnAbort()");
        if(ptr)
        {
          mdb_txn_abort(ptr);
        }
        itc::getLog()->trace(__FILE__, __LINE__, "[trace] <-out Database::WOTxnAbort()");
      }

      /*=====================================================================*/
      ~Environment()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
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

