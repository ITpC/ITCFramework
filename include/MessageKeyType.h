/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: MessageKeyType.h 1 Апрель 2015 г. 22:51 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __MESSAGEKEYTYPE_H__
#  define	__MESSAGEKEYTYPE_H__

namespace itc
{

  struct Key
  {
    uint64_t left;
    uint64_t right;

    Key(){}
    Key(const uint64_t& l, const uint64_t& r)
      :left(l), right(r)
    {
    }

    Key(const Key& key)
      :left(key.left), right(key.right)
    {
    }

    const bool operator>(const Key& ref) const
    {
      if(left>ref.left)
        return true;
      if((left == ref.left) && (right > ref.right) )
        return true;
      return false;
    }
    const bool operator<(const Key& ref) const
    {
      if(left<ref.left)
        return true;
      if((left==ref.left) && (right<ref.right))
        return true;
      return false;
    }
    const bool operator==(const Key& ref) const
    {
      return ((left == ref.left)&&(right == ref.right));
    }
    
    const bool operator!=(const Key& ref) const
    {
      return !((*this) == ref);
    }
    const bool operator<=(const Key& ref) const
    {
      return ((*this) == ref ) || ((*this) < ref);
    }

    const bool operator>=(const Key& ref) const
    {
      return ((*this) == ref ) || ((*this) > ref);
    }
  };

}

#endif	/* __MESSAGEKEYTYPE_H__ */

