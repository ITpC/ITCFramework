/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ConfigReflection.cpp 27 Март 2015 г. 15:15 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#include <ConfigReflection.h>
namespace itc
{
  namespace reflection
  {
    Variable& Variable::operator[](const std::string& name)
    {
      return cast<Array>(*this)[name];
    }

    const Variable& Variable::operator[](const std::string& name) const
    {
      return cast<const Array>(*this)[name];
    }    
  }
}
