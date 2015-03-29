/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: save.cpp 27 Март 2015 г. 13:05 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/
#include <Config.h>
#include <ConfigReflection.h>
#include <algorithm>
#include <functional>
#include <map>
#include <sstream>

void itc::Config::save(std::stringstream& fs, reflection::Array& ref, std::string indent, bool first)
{
  std::for_each(
    aaccess(ref).begin(), aaccess(ref).end(),
    [this, &indent, &first, &fs](const reflection::VariablePairType & var)
    {
      if(var.second.get()->getType() == ARRAY)
      {
        if(!first)
        {
          fs << indent << "," << std::endl;
          first = true;
        }
        fs << indent << var.first << ":" << std::endl;
        fs << indent << "{" << std::endl;
        indent.append("  ");
        save(fs, *(static_cast<reflection::Array*> (var.second.get())), indent, first);
        indent.erase(indent.length() - 2, indent.length());
        fs << indent << "}" << std::endl;
        first = false;
      }else
      {
        if(!first)
        {
          fs << indent << "," << std::endl;
        }else
        {
          first = false;
        }

        fs << indent << var.first << ":";
        switch(var.second.get()->getType())
        {
          case NUMBER:
            fs << (static_cast<Number*> (var.second.get()))->getValue() << std::endl;
            break;
          case STRING:
            fs << "\"" << (static_cast<String*> (var.second.get()))->getValue() << "\"" << std::endl;
            break;
          case BOOL:
            fs << (static_cast<Bool*> (var.second.get()))->getValue() << std::endl;
            break;
          default:
            throw TITCException<exceptions::ConfigSyntax>(exceptions::Reflection);
        }
      }
    }
  );
  indent.erase(indent.length() - 2, indent.length());
}