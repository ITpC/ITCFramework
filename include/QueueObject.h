/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: DBObject.h 2 Апрель 2015 г. 23:26 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __QUEUEOBJECT_H__
#  define	__QUEUEOBJECT_H__

#  include <stdint.h>
#  include <lmdb.h>
#  include <DBKeyType.h>
#  include <memory>

namespace itc
{
  typedef std::shared_ptr<uint8_t> uint8_t_sarray;

  namespace lmdb
  {

    enum WObjectOP
    {
      ADD, DEL
    };
  }

  struct QueueObject
  {
    DBKey mKey;
    size_t mDataSize;
    uint8_t_sarray mData;
    lmdb::WObjectOP theOP;

    QueueObject() = default;

    QueueObject(const DBKey& key, const size_t size, const uint8_t_sarray& dataref)
      :mKey(key), mDataSize(size), mData(dataref)
    {
    }

    QueueObject(const QueueObject& ref)
      :mKey(ref.mKey), mDataSize(ref.mDataSize), mData(ref.mData)
    {
    }

    const size_t& getSize() const
    {
      return mDataSize;
    }

    const DBKey& getKey() const
    {
      return mKey;
    }

    const uint8_t_sarray& getData() const
    {
      return mData;
    }

    void prep_mdb_key(MDB_val* ptr)
    {
      ptr->mv_size = sizeof(DBKey);
      ptr->mv_data = &mKey;
    }

    void prep_mdb_data(MDB_val* ptr)
    {
      ptr->mv_size = mDataSize;
      ptr->mv_data = mData.get();
    }
    ~QueueObject()=default;
  };

  typedef std::shared_ptr<QueueObject> QueueMessageSPtr;
}

#endif	/* __QUEUEOBJECT_H__ */

