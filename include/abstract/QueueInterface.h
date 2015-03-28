/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: QueueInterface.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __QUEUE_INTERFACE_H__
#  define __QUEUE_INTERFACE_H__
#  include <stdint.h>

namespace itc
{

  enum QueueBlockPolicy
  {
    ASYNC, SYNC
  };

  template <typename DataType> class QueueInterface
  {
   public:

    QueueInterface()
    {
    }
    virtual bool send(const DataType&) = 0;
    virtual bool recv(DataType&) = 0;
    virtual size_t depth() = 0;
    virtual void destroy() = 0;

   protected:

    virtual ~QueueInterface() = default;
  };
}
#endif /*__QUEUE_INTERFACE_H__*/
