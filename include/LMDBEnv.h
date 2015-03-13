/* 
 * File:   LMDBEnv.h
 * Author: pk
 *
 * Created on 13 Март 2015 г., 14:10
 */

#ifndef LMDBENV_H
#  define	LMDBENV_H
#include <string>
#include <lmdb.h>
#include <memory>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <ITCException.h>

namespace itc
{
 namespace lmdb
 {
  class Environment
  {
  private:
    sys::Mutex mMutex;
    std::string mPath;
    MDB_Env *mEnv;
    MDB_txn *mTxn; 
    bool mActive;
    
    void throwAnException(int ret)
    {
       switch(ret) 
       {
         case MDB_VERSION_MISMATCH: 
           throw TITCException<exceptions::MDBVersionMissmatch>(exceptions::ExternalLibraryException);
           ;
         case  MDB_INVALID:
           throw TITCException<exceptions::MDBInvalid>(exceptions::ExternalLibraryException);
           ;
         case ENOENT: 
           throw TITCException<exceptions::MDBNotFound>(exceptions::ExternalLibraryException);
           ;
         case EACCES:
           throw TITCException<exceptions::MDBEAccess>(exceptions::ExternalLibraryException);
           ;
         case EAGAIN:
           throw TITCException<exceptions::MDBEAgain>(exceptions::ExternalLibraryException);
           ;
         default:
         {
          throw throw TITCException<exceptions::ExternalLibraryException>(errno);
         }
       }
    }
    
  public:
    Environment(const std::string& path)
    : mMutex(), mPath(path),mEnv((MDB_env*)0)
    {
      sys::SyncLock synchronize(mMutex);
      
      if((int ret=mdb_env_create(&mEnv))>0)
      {
        throwAnException(ret);
      }
      
      if((int ret=mdb_env_open(mEnv, mPath.c_str(), 0, 0664)>0))
      {
        mdb_env_close(mEnv);
        throwAnException(ret);
      }
      mdb_txn_begin(mEnv, NULL, 0, &mTxn);
      mActive=true;
    };
    const MDB_env* getEnv()
    {
      sys::SyncLock synchronize(mMutex);
      if(mActive)
      {
       return mEnv;
      }
      throw throw TITCException<exceptions::MDBClosed>(exceptions::ApplicationException);
    }
    const std::string& getPath()
    {
     sys::SyncLock synchronize(mMutex);
      if(mActive)
      {
       return mPath;
      }
      throw throw TITCException<exceptions::MDBClosed>(exceptions::ApplicationException);
    }
    ~Environment()
    {
      sys::SyncLock synchronize(mMutex);
      if(mActive)
      {
        mdb_env_close(mEnv);
        mActive=false;
      }
    }
  };
 }
}



#endif	/* LMDBENV_H */

