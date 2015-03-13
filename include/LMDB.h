/* 
 * File:   LMDB.h
 * Author: pk
 *
 * Created on 12 Март 2015 г., 17:07
 */

#ifndef LMDB_H
#  define	LMDB_H
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <ServiceFacade.h>
#include <LMDBEnv.h>
#include <lmdb.h>

namespace itc
{
 namespace lmdb
 {
  class Database : public ServiceFacade
  {
  private:
    sys::Mutex  mL2Mutex;
    Environment mEnvironment;
    std::string mDBName;
    MDB_dbi dbi;
    MDB_txn *txn;

  public:
    Database(const std::string& env_path, const std::string& db_name)
    : ServiceFacade("itc::lmdb::Database["+env_path+"/"+db_name+"]"),
      mL2Mutex(),mEnvironment(env_path),mDBName(db_name)
    {
     
    }
  };
 }
}


#endif	/* LMDB_H */

