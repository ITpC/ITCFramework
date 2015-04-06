/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: DBKey.h 2 Апрель 2015 г. 21:42 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __DBKEYTYPE_H__
#  define	__DBKEYTYPE_H__

#  include <memory>
#  include <atomic>
#  include <mutex>
#  include <functional>
#  include <lmdb.h>
#  include <sys/synclock.h>
#  include <time.h>

namespace itc
{
  class DBKeyAccess;
  class DBKey
  {
   private:
    std::atomic<uint64_t> left;
    std::atomic<uint64_t> right;
    friend DBKeyAccess;
   public:
    
    const bool operator>(const DBKey& ref) const
    {
      if(left > ref.left)
        return true;
      if((left == ref.left) && (right > ref.right))
        return true;
      return false;
    }

    const bool operator<(const DBKey& ref) const
    {
      if(left < ref.left)
        return true;
      if((left == ref.left) && (right < ref.right))
        return true;
      return false;
    }

    const bool operator==(const DBKey& ref) const
    {
      return((left == ref.left)&&(right == ref.right));
    }

    const bool operator!=(const DBKey& ref) const
    {
      return !((*this) == ref);
    }

    const bool operator<=(const DBKey& ref) const
    {
      return((*this) == ref) || ((*this) < ref);
    }

    const bool operator>=(const DBKey& ref) const
    {
      return((*this) == ref) || ((*this) > ref);
    }

    DBKey& operator=(const DBKey& ref)
    {
      left = uint64_t(ref.left);
      right = uint64_t(ref.right);
      return *this;
    }
    
    DBKey() = default;

    DBKey(const uint64_t& l, const uint64_t& r)
      :left(l), right(r)
    {
    }

    DBKey(const DBKey& key)
      :left(uint64_t(key.left)), right(uint64_t(key.right))
    {
    }
  };
  
  class DBKeyAccess
  {
    std::recursive_mutex mMutex;
    DBKey mKey;
    
   public:
    struct timestamp
    {
      uint64_t fsec:32;
      uint64_t null:32;
      uint64_t nsec:30;
      uint64_t zero:34;
    };
    
    struct nph
    {
      uint64_t sec:32;
      uint64_t Id:32;
      uint64_t nsec:30;
      uint64_t qsqn:32;
      uint64_t resr:2;
    };
    
    struct dbk
    {
      uint64_t left;
      uint64_t right;
    };
    
    union keyconv
    {
      timestamp ts;
      timespec  tc;
      nph       ph;
      dbk       ky;
      uint64_t  ar[2];
    };
    
    
    explicit DBKeyAccess(const uint64_t& l, const uint64_t& r) 
    : mMutex(), mKey(set(l,r))
    {
    }
    explicit DBKeyAccess(const DBKey& ref) 
    : mMutex(), mKey(set(ref))
    {
    }
    
    explicit DBKeyAccess(): mMutex() {}
    
    void set(const timespec& ptr)
    {
      RSyncLock sync(mMutex);
      memcpy(&mKey,&ptr,sizeof(DBKey));
    }
    
    void use(std::function<void(const timespec&)> func)
    {
      RSyncLock sync(mMutex);
      timespec ts;
      ts.tv_sec=mKey.left;
      ts.tv_nsec=mKey.right;
      func(ts);
    }

    void use(std::function<void(const DBKeyAccess::nph&)> func)
    {
      RSyncLock sync(mMutex);
      keyconv kc;
      kc.ky.left=mKey.left;
      kc.ky.right=mKey.right;
      func(kc.ph);
    }

    void use(std::function<void(const uint64_t& l, const uint64_t& r)> func)
    {
      RSyncLock sync(mMutex);
      func(mKey.left,mKey.right);
    }

    void use(std::function<void(const MDB_val& ref)> func)
    {
      RSyncLock sync(mMutex);
      uint64_t arr[2];
      arr[0]=mKey.left;
      arr[1]=mKey.right;
      MDB_val key;
      key.mv_size=sizeof(arr);
      key.mv_data=arr;
      func(key);
    }
    void setKey(uint64_t *key)
    {
      RSyncLock sync(mMutex);
      key[0]=mKey.left;
      key[1]=mKey.right;
    }
    const uint32_t getSenderId()
    {
      RSyncLock sync(mMutex);
      keyconv kc;
      kc.ky.left=mKey.left;
      kc.ky.right=mKey.right;
      return kc.ph.Id;
    }

    void getTimeStamp(timespec& ref)
    {
      RSyncLock sync(mMutex);
      keyconv kc;
      kc.ky.left=mKey.left;
      kc.ky.right=mKey.right;
      kc.ts.null=0;
      kc.ts.zero=0;
      ref.tv_sec=kc.tc.tv_sec;
      ref.tv_nsec=kc.tc.tv_nsec;
    }
   private:
    
    DBKey set(const uint64_t& l, const uint64_t& r)
    {
      RSyncLock sync(mMutex);
      DBKey tmp(l,r);
      return tmp;
    }

    DBKey set(const DBKey& ref)
    {
      RSyncLock sync(mMutex);
      DBKey tmp(ref);
      return tmp;
    }
  };
}


#endif	/* __DBKEYTYPE_H__ */
