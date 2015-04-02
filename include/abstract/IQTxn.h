/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: IQTxn.h 27 Март 2015 г. 23:53 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __IQTXN_H__
#  define	__IQTXN_H__

#include <memory>
#include <stdint.h>

namespace itc
{
  namespace abstract
  {
    class IQTxn: public std::enable_shared_from_this<IQTxn>
    {
     public:
      explicit IQTxn()=default;
      virtual void abort()=0;
      virtual const bool commit()=0;
     protected:
      ~IQTxn()=default;
    };
  }
}


#endif	/* __IQTXN_H__ */

