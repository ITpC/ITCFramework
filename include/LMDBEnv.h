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
       * @exception TITCException<exceptions::ExternalLibraryException>(errno)
       * @exception TITCException<exceptions::MDBEAgain>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBEAccess>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBNotFound>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBInvalid>(exceptions::ExternalLibraryException)
       * @exception TITCException<exceptions::MDBVersionMissmatch>(exceptions::ExternalLibraryException)
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
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
        ret = mdb_env_set_maxdbs(mEnv, mDbs);
        if(ret) LMDBExceptionParser onMaxDBsSet(ret);
        // Open an environment handle
        ret = mdb_env_open(mEnv, mPath.c_str(), 0, 0664);
        if(ret)
        {
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

