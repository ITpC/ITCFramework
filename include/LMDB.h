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
#  include <Config.h>

namespace itc
{
  using namespace reflection;
  namespace lmdb
  {
    using namespace reflection;
    class Database : public ServiceFacade
    {
     private:
      sys::Mutex mL2Mutex;
      std::shared_ptr<Environment> mEnvironment;
      std::string mDBName;
      MDB_dbi dbi;
      MDB_txn *txn;

     public:

      /**
       * @brief Symas LMDB wrapper around DB functionality [WIP, nothing good yet here]
       * 
       **/
      explicit Database()
        : ServiceFacade("itc::lmdb::Database"),
        mL2Mutex()
      {
        sys::SyncLock synchronize(mL2Mutex);
        char* lmdb_base=getenv("LMDB_BASE");
        if(lmdb_base == NULL)
        {
          itc::getLog()->error(__FILE__,__LINE__,"LMDB_BASE is undefined");
          throw TITCException<exceptions::MDBNotFound>(exceptions::FileNotFound);
        }
          
        std::string lmdb_instance_home(lmdb_base);
        itc::Config aLMDBConfig(lmdb_instance_home + "/etc/lmdb.conf");
        String datastore(aLMDBConfig.get<String>("datastore"));
        String database(aLMDBConfig.get<String>("database"));
        Number dbsnr(aLMDBConfig.get<Number>("dbsnr"));
        mDBName = datastore.to_stdstring()+"/"+database.to_stdstring();
        itc::getLog()->debug(__FILE__,__LINE__,"Trying to open a database in %s",mDBName.c_str());
        mEnvironment = std::make_shared<Environment>(datastore.to_stdstring(), dbsnr.toInt());
      }

      /**
       * @brief implementation of onStart of ServiceFacade. This method opens the LMDB database
       * or throws an exception.
       **/
      void onStart()
      {
        sys::SyncLock synchronize(mL2Mutex);
      
        int ret = mdb_txn_begin(mEnvironment.get()->getEnv(), NULL, 0, &txn);
        if(ret) LMDBExceptionParser onTxnOpen(ret);

        ret = mdb_dbi_open(txn, mDBName.c_str(), MDB_CREATE | MDB_REVERSEKEY, &dbi);
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

