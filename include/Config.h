/**
 * Copyright (c) 2009-2015, Pavel Kraynyukhov.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted under the terms of the General Public License version 2
 * (GPLv2), provided that the above copyright notice and this paragraph and the
 * following two paragraphs and the "LICENSE" file appear in all modified or
 * unmodified copies of the software "AS IS" and without any changes.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * $Id: Config.h 22 2015-03-13 23:24:33Z pk $
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
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <TSLog.h>
#  include <ITCException.h>
#  include <ConfigReflection.h>
#  include <boost/regex.hpp>
#  include <StringTokenizer.h>
#include <Val2Type.h>

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
    
    const VariableSPtr& operator[](const std::string& name)
    {
      return getRoot()[name];
    }
    
    /**
     * @brief get the variable value by path
     * 
     * @param path is a dot separated sequence of name representing the path
     * to variable. Assume the array { a:{b:{c:1}}}, to get the value of c
     * you have to provide "a.b.c" as a path to this method.
     **/
    template<typename T> T& get(const std::string path)
    {
      utils::StringTokenizer tkz;
      VariableSPtr sptr(mConfigArray["root"]);
      std::string tmp(path); 
      std::list<std::string> pathlist(tkz.scan(tmp,"."));
      std::list<std::string>::iterator it=pathlist.begin();
      size_t depth=pathlist.size();
      std::for_each(
        pathlist.begin(),pathlist.end(),
        [](const std::string& name)
        {
          std::cout <<name << std::endl;
        }
      );
      if(depth>0)
      {
        while(depth>1)
        {
          --depth;
          sptr=cast<Array>(sptr)[*it];
          ++it;
        }
        return cast<T>(cast<Array>(sptr)[*it]);
      }
      abort();
      //throw TITCException<exceptions::Reflection>(exceptions::IndexOutOfRange);
    }
    

   protected:
    
    Array& getRoot()
    {
      return (*static_cast<Array*>(mConfigArray["root"].get()));
    }

    
    /**
     * @TBR on completion.
     **/
    void printArray(reflection::Array& ref, std::string indent="");
     
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

