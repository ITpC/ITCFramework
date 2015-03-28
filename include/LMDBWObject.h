/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: LMDBWObject.h 1 2015-03-22 14:01:11Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef LMDBWOBJECT_H
#  define	LMDBWOBJECT_H
#  include <lmdb.h>

namespace itc
{
  namespace lmdb
  {

    enum WObjectOP
    {
      ADD, DEL
    };

    struct WObject
    {
      MDB_val key;
      MDB_val data;
      WObjectOP theOP;

      explicit WObject()
      {
      }
      explicit WObject(const WObject&) = delete;
      explicit WObject(WObject&) = delete;
      ~WObject() = default;
    };
  }
}

#endif	/* LMDBWOBJECT_H */

