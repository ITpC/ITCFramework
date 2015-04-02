/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: IMessage.h 1 Апрель 2015 г. 22:31 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __IMESSAGE_H__
#  define	__IMESSAGE_H__

#  include <MessageKeyType.h>


namespace itc
{
  namespace abstract
  {
    class IMessage
    {
     public:
      virtual const Key& getKey() = 0;
      virtual ~IMessage() = default;
    };
  }
}

#endif	/* __IMESSAGE_H__ */

