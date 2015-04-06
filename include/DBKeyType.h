/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: DBKeyType.h 2 Апрель 2015 г. 21:42 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __DBKEYTYPE_H__
#  define	__DBKEYTYPE_H__

#  include <MessageKeyType.h>

namespace itc
{

  struct DBKey
  {
    uint64_t left;
    uint64_t right;

    DBKey() = default;

    DBKey(const uint64_t& l, const uint64_t& r)
      :left(l), right(r)
    {
    }

    DBKey(const Key& ref)
      :left(ref.left), right(ref.right)
    {
    }

    DBKey(const DBKey& key)
      :left(key.left), right(key.right)
    {
    }

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
      left = ref.left;
      right = ref.right;
      return *this;
    }
  };
}


#endif	/* __DBKEYTYPE_H__ */

