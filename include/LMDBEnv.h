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

#  include <string>
#  include <lmdb.h>
#  include <memory>

namespace itc
{
  namespace lmdb
  {

    /**
     * @brief wrapper around Symas LMDB. Environment setup.
     **/
    class Environment
    {
     private:
      itc::sys::Mutex mMutex;
      std::string mPath;
      MDB_env *mEnv;
      MDB_dbi mDbs;
      bool mActive;


     public:

      /**
       * @brief constructor for LMDB Environment 
       * 
       **/
      explicit Environment(const std::string& path, const int& dbs)
        : mMutex(), mPath(path), mEnv((MDB_env*) 0), mDbs(dbs),
        mActive(false)
      {
        itc::sys::SyncLock synchronize(mMutex);
        // Create an LMDB environment handle.
        int ret = mdb_env_create(&mEnv);
        if(ret)
        {
          itc::getLog()->error(__FILE__,__LINE__,"LMDB: Can not create database environment in %s",mPath.c_str());
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
        ret = mdb_env_set_maxdbs(mEnv, mDbs);
        if(ret) 
        {
          itc::getLog()->error(__FILE__,__LINE__,"LMDB: Can not set max_dbs for %s",mPath.c_str());
          LMDBExceptionParser onMaxDBsSet(ret);
        }
        // Open an environment handle
        ret = mdb_env_open(mEnv, mPath.c_str(), 0, 0664);
        if(ret)
        {
          itc::getLog()->error(__FILE__,__LINE__,"LMDB: Can not open environment %s",mPath.c_str());
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

      const bool isActive() const
      {
        return mActive;
      }

      const std::string& getPath() const
      {
        return mPath;
      }

      ~Environment()
      {
        itc::sys::SyncLock synchronize(mMutex);
        if(mActive)
        {
          mdb_env_close(mEnv);
          mActive = false;
        }
      }
    };
  }
}



#endif	/* LMDBENV_H */

