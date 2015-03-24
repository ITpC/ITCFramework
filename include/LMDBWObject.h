/* 
 * File:   LMDBWObject.h
 * Author: pk
 *
 * Created on 22 Март 2015 г., 14:01
 */

#ifndef LMDBWOBJECT_H
#  define	LMDBWOBJECT_H
#include <abstract/Runnable.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <memory>
#include <queue>
#include <algorithm>
#include <functional>
#include <LMDBEnv.h>
#include <LMDB.h>

namespace itc
{
  namespace lmdb
  {
    enum WObjectOP{ADD,DEL};
    
    struct WObject
    {
      MDB_val key;
      MDB_val data;
      WObjectOP theOP;
      
      explicit WObject(){}
      explicit WObject(const WObject&)=delete;
      explicit WObject(WObject&)=delete;
      ~WObject()=default;
    };
  }
}

#endif	/* LMDBWOBJECT_H */

