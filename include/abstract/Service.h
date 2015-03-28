/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Service.h 1 2015-03-10 15:37:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef SERVICE_H
#  define	SERVICE_H
#include <string>

namespace itc
{
 namespace abstract
 {
  class Service
  {
  public:
    
    virtual void start()=0;
    virtual void stop()=0;
    virtual void restart()=0;
    virtual const std::string& getName()=0;
    
    virtual const bool isdown() const =0;
    virtual const bool isup() const =0;

  protected:
    virtual ~Service()=default;
  };
 }
}

#endif	/* SERVICE_H */

