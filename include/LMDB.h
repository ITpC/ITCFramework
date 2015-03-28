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
#  include <LMDBException.h>
#  include <queue>
#  include <stdint.h>
#  include <mutex>

namespace itc
{
  namespace lmdb
  {
    class WOTxn;
    class ROTxn;

    class Database
    {
     private:
      std::mutex mMutex;
      std::shared_ptr<Environment> mEnvironment;
      std::string mDBEnvPath;
      std::string mDBName;
      int mDBMode;
      MDB_dbi dbi;
      MDB_txn *txn;
      bool isopen;
      friend ROTxn;
      friend WOTxn;

     public:

      /**
       * @brief Symas LMDB wrapper around DB functionality as a backend for 
       * persistent queues[WIP, nothing good yet here]
       *
       * @param path - LMDB database environment path
       * @param dbname - name of the database (optional)
       * @param mode - database open mode and settings (optional, default: create
       * database if it is not exists, the keys are of uint64_t type
       **/
      explicit Database(
        const std::string& path,
        const std::string& dbname = "",
        const int mode = MDB_CREATE | MDB_INTEGERKEY
        )
        : mMutex(), mDBEnvPath(path),
        mDBMode(mode), isopen(false)
      {
        std::lock_guard<std::mutex> dosync(mMutex);

        if(dbname.empty())
        {
          mEnvironment = std::make_shared<Environment>(mDBEnvPath, 0);
        }else
        {
          mDBName = path + "/" + dbname;
          mEnvironment = std::make_shared<Environment>(mDBEnvPath, 1);
        }
      }

      Database(const Database&) = delete;

      /**
       * @brief create the environment and open the database
       **/
      void open()
      {
        itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> Database::open()");
        std::lock_guard<std::mutex> dosync(mMutex);

        if(isopen)
        {
          itc::getLog()->debug(__FILE__, __LINE__, "[trace] -> Database::open(), database is already open");
          return;
        }

        int ret = mdb_txn_begin(mEnvironment.get()->getEnv(), NULL, 0, &txn);
        if(ret) LMDBExceptionParser onTxnOpen(ret);
        if(mDBName.empty())
        {
          ret = mdb_dbi_open(txn, NULL, mDBMode, &dbi);
        }else
        {
          ret = mdb_dbi_open(txn, mDBName.c_str(), mDBMode, &dbi);
        }
        if(ret) LMDBExceptionParser onDbiOpen(ret);
        mdb_txn_commit(txn);
        isopen = true;
      }

      const std::shared_ptr<Environment>& getDBEnv()
      {
        return mEnvironment;
      }

      const std::string& getName() const
      {
        return mDBName;
      }

      void shutdown()
      {
        std::lock_guard<std::mutex> dosync(mMutex);
        isopen = false;
      }

      ~Database()
      {
        this->shutdown();
      }
    };
  }
}


#endif	/* LMDB_H */

