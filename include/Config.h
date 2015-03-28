/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Config.h 22 2015-03-13 23:24:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __JSNON_LIKE_CONFIG_H__
#  define	__JSNON_LIKE_CONFIG_H__
#  include <string>
#  include <algorithm>
#  include <functional>
#  include <memory>
#  include <list>
#  include <stack>
#  include <map>
#  include <fstream>
#  include <TSLog.h>
#  include <ITCException.h>
#  include <ConfigReflection.h>
#  include <boost/regex.hpp>
#  include <StringTokenizer.h>
#  include <Val2Type.h>
#  include <sstream>

namespace itc
{
  using namespace reflection;

  enum Expect
  {
    INPUT_END, MPB, MPE, NAME, COLON, VALUE, QUOTE
  };

  static std::string rNumber("[-]*[[:digit:]]+|([-]*[[:digit:]]*[.][[:digit:]]+)|0x[[:xdigit:]]+");
  static std::string rBool("true|false|on|off");
  static std::string rString("[[:print:]]+");
  static std::string rName("[[:alpha:]]+[[:alnum:]_]*");

  static boost::regex rxNumber(rNumber, boost::regex::extended);
  static boost::regex rxBool(rBool, boost::regex::extended);
  static boost::regex rxString(rString, boost::regex::extended);
  static boost::regex rxName(rName, boost::regex::extended);

  /**
   * @brief This is a configuration holding class, for JSON-like arrays.
   * 
   * <PRE>
   * The syntax of the configuration file:
   * 
   * array: '{' key_value_list '}'
   *    ;
   * key_value_list: key_value_list, key_value_pair
   *    ;
   * key_value_pair: name ':' value
   *    ;
   * value: array | string | NUMBER | BOOL 
   *    ;
   * string: '"' string '"' | [[:print:]]+ 
   *    ;
   * BOOL: "true" | "false" | "on" | "off"
   *    ;
   * NUMBER: [-]*[[:digit:]]+|([-]*[[:digit:]]*[.][[:digit:]]+)|0x[[:xdigit:]]+
   *    ;
   * </PRE>
   **/
  class Config
  {
   private:
    std::stack<Expect> expected_lexeme;
    std::stack<std::string> lastname;
    std::list<std::string> tokens;
    std::stack<Array*> mSubArrays;
    std::string lastvalue;
    std::string mConfigFile;
    Array mConfigArray;

    typedef std::list<std::string>::iterator tokens_iterator;

   public:
    /**
     * @brief constructor
     * 
     * @param fname a filename of config file to read the array from.
     **/
    Config(const std::string& fname);

    const Variable& operator[](const std::string& name)
    {
      return getRoot()[name];
    }

    void save(const std::string& newname)
    {
      std::fstream fs(newname,std::ios_base::out|std::ios_base::trunc);
      fs << save();
      fs.close();      
    }
    
    const std::string save()
    {
      std::stringstream fs;
      fs << "{" << std::endl;
      save(fs, getRoot(), "  ");
      fs << "}" << std::endl;
      return fs.str();
    }
    
   protected:

    Array& getRoot()
    {
      return *static_cast<Array*>(&(mConfigArray["root"]));
    }

    void printArray(reflection::Array&, std::string indent="  ");
    void save(std::stringstream&, reflection::Array&, std::string indent = "", bool first = true);

    /**
     * @brief JSON-like parser
     * 
     * @param stream - an input sequence of symbols to parse to.
     **/
    void parser(const std::string& stream);

    /**
     * @brief JSON-like parser-wrapper with stack and syntax breaks output.
     **/
    void parse();

    /**
     * @brief throw an exception on parser stack violations (bugs in parser)
     **/
    void throwOnEmptyLexemesStack(const std::string& file, const int line)
    {
      if(expected_lexeme.empty())
      {
        itc::getLog()->fatal(file.c_str(), line, "Expected lexemes stack is empty, kick the programmer at the ass (virtualy)");
        itc::getLog()->flush();
        throw TITCException<exceptions::IndexOutOfRange>(exceptions::ConfigSyntax);
      }
    }

    /**
     * @brief throw an exception on syntax errors.
     **/
    void syntaxerror(const std::string& file, const int line, const std::string& msg)
    {
      itc::getLog()->error(file.c_str(), line, "Syntaxis error on config %s parsing: %s", mConfigFile.c_str(), msg.c_str());
      itc::getLog()->flush();
      throw TITCException<exceptions::ConfigSyntax>(exceptions::ApplicationException);
    }

    bool saveValue()
    {
      if(!lastvalue.empty())
      {
        if(boost::regex_match(lastvalue, rxNumber))
        {
          paccess(mSubArrays.top())[lastname.top()] = std::make_shared<Number>(std::stod(lastvalue));
        }else if(boost::regex_match(lastvalue, rxBool))
        {
          if(lastvalue == "on" or lastvalue == "true")
          {
            paccess(mSubArrays.top())[lastname.top()] = std::make_shared<Bool>(true);
          }else if(lastvalue == "off" or lastvalue == "false")
          {
            paccess(mSubArrays.top())[lastname.top()] = std::make_shared<Bool>(false);
          }else
          {
            syntaxerror(__FILE__, __LINE__, "The boolean values are only: on, off, true, false");
          }
        }else if(boost::regex_match(lastvalue, rxString))
        {
          paccess(mSubArrays.top())[lastname.top()] = std::make_shared<String>(lastvalue);
        }else
        {
          syntaxerror(__FILE__, __LINE__, "can not determine type of the value");
        }
        return true;
      }
      return false;
    }
  };
}



#endif	/* __JSNON_LIKE_CONFIG_H__ */

