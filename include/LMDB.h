/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: LMDB.h 1 12 Март 2015 г., 17:07 pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef LMDB_H
#  define	LMDB_H
#  include <TSLog.h>
#  include <string>
#  include <lmdb.h>
#  include <LMDBEnv.h>
#  include <queue>
#  include <stdint.h>
#  include <mutex>
#  include <atomic>

namespace itc
{
  namespace lmdb
  {
    class WOTxn;
    class ROTxn;

    class Database
    {
     public:
       typedef std::shared_ptr<Environment> LMDBEnvSPtr;
     private:
      std::mutex mMutex;
      std::recursive_mutex mMasterLock;
      std::string mDBEnvPath;
      int mDBMode;
      std::atomic<bool> mIsOpen;
      
      std::shared_ptr<Environment> mEnvironment;
      
      std::string mDBName;
      
      MDB_dbi dbi;
      MDB_txn *txn;
      
      friend ROTxn;
      friend WOTxn;

     public:

      /**
       * @brief wrapper for Symas LMDB as a backend for 
       * persistent queues[WIP]
       *
       * @param path - LMDB database environment path
       * @param dbname - name of the database (optional)
       * @param mode - database open mode and settings (optional, default: create
       * database if it is not exists, the keys are of uint64_t type
       **/
      explicit Database(
        const std::string& path,
        const std::string& dbname = "",
        const uint32_t dbsize=41943040,
        const int mode = MDB_CREATE 
      ):  mMutex(), mMasterLock(), mDBEnvPath(path),
          mDBMode(mode), mIsOpen(false)
      {
        std::lock_guard<std::mutex> dosync(mMutex);

        if(dbname.empty())
        {
          mEnvironment = std::make_shared<Environment>(mDBEnvPath, 0,dbsize);
        }
        else
        {
          mDBName = mDBEnvPath + "/" + dbname;
          mEnvironment = std::make_shared<Environment>(mDBEnvPath, 1,dbsize);
        }
      }

      Database(const Database&) = delete;
      Database(Database&) = delete;

      /**
       * @brief create the environment and open the database
       **/
      void open()
      {
        std::lock_guard<std::mutex> dosync(mMutex);

        if(mIsOpen)
        {
          itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> Database::open(), database is already open");
          return;
        }
        
        itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> Database::open() in env: %s",mDBEnvPath.c_str());
        
        int ret = mdb_txn_begin(mEnvironment.get()->getEnv(), NULL, 0, &txn);
        if(ret) throw TITCException<exceptions::MDBGeneral>(ret);
        if(mDBName.empty())
        {
          ret = mdb_dbi_open(txn, NULL, mDBMode, &dbi);
        }
        else
        {
          itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> open database %s in env: %s",mDBName.c_str(),mDBEnvPath.c_str());
          ret = mdb_dbi_open(txn, mDBName.c_str(), mDBMode, &dbi);
        }
        if(ret) throw TITCException<exceptions::MDBGeneral>(ret);
        ret=mdb_txn_commit(txn);
        if(ret)
        {
          throw TITCException<exceptions::MDBGeneral>(ret);
        }
        mIsOpen = true;
      }

      const std::shared_ptr<Environment>& getDBEnv()
      {
        return mEnvironment;
      }

      const std::string& getName() const
      {
        return mDBName;
      }


      void lock()
      {
        mMasterLock.lock();
      }

      void unlock()
      {
        mMasterLock.unlock();
      }
      
      void shutdown()
      {
        mIsOpen = false;
      }

      const bool isopen()
      {
        return mIsOpen;
      }
      
      ~Database()
      {
        this->shutdown();
      }
    };
  }
}


#endif	/* LMDB_H */

