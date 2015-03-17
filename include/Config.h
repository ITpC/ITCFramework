/**
 * Copyright (c) 2009, Pavel Kraynyukhov.
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
   * [WIP: JSON-like config facility. :WIP]
   * 
   * BEGIN_MP = '{'
   * END_MP = '}'
   * 
   * mp = BEGIN_MP kvpair_list END_MP
   * kvpair = NAME ':' value
   * kvpair_list = kvpair_list ',' kvpair
   * value = string || number || mp
   * string = "[[:print:]]+"
   * number = "[[:digit:]]+ || ([[:digit:]]+|[[:digit:]]*[.][[:digit:]]+) || 0x[[:xdigit:]]+
   * 
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

    Config(const std::string& fname);

   protected:
    /**
     * @TBR on completion.
     **/
    void printArray(reflection::Array& ref, std::string indent="");
    
    /** 
     * WIP !!!!!!!!!!!!
     * 
    */
      
    /* 
    template <VarType RType> struct Type
    {
      auto get(Variable* ptr, const Val2Type<ARRAY>& fictive)
      {
        return ((static_cast<Array*>(ptr))->getValue());
      }
    };
        
    auto operator[](const std::string& name)
    {
      auto it=aaccess(mConfigArray).find(name);
      if(Variable* ptr=it->second.get())
      {
        switch(ptr->getType())
        {
          case ARRAY: return decltype(*((static_cast<Array*>(ptr))->getValue()));
          case NUMBER: return decltype(*((static_cast<Number*>(ptr))->getValue()));
          case BOOL: return decltype(*((static_cast<Bool*>(ptr))->getValue()));
          case STRING: return decltype(*((static_cast<String*>(ptr))->getValue()));
          default:
            throw TITCException<exceptions::Reflection>(exceptions::UndefinedType);
        }
      }
      throw TITCException<exceptions::Reflection>(exceptions::IndexOutOfRange);
    }
     **/
    
    void throwOnEmptyLexemesStack(const std::string& file, const int line)
    {
      if(expected_lexeme.empty())
      {
        itc::getLog()->fatal(file.c_str(), line, "Expected lexemes stack is empty, kick the programmer at the ass (virtualy)");
        itc::getLog()->flush();
        throw TITCException<exceptions::IndexOutOfRange>(exceptions::MPConfigSyntax);
      }
    }

    void syntaxerror(const std::string& file, const int line, const std::string& msg)
    {
      itc::getLog()->error(file.c_str(), line, "Syntaxis error on config %s parsing: %s", mConfigFile.c_str(), msg.c_str());
      itc::getLog()->flush();
      throw TITCException<exceptions::MPConfigSyntax>(exceptions::ApplicationException);
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

    void parser(const std::string&);
   

    void parse()
    {
      expected_lexeme.push(INPUT_END);
      expected_lexeme.push(MPB); // '{' is the first terminal, which we expect.
      aaccess(mConfigArray)["root"] = std::make_shared<Array>();
      mSubArrays.push(
        (static_cast<Array*> (aaccess(mConfigArray)["root"].get()))
        );

      for(tokens_iterator it = tokens.begin(); it != tokens.end(); ++it)
      {
        try
        {
          parser(*it);
        }catch(const std::exception& e)
        {
          if(!lastname.empty())
          {
            std::cerr << "lastname: " << lastname.top() << std::endl;
          }
          std::cerr << "lastvalue: " << lastvalue << std::endl;

          for(tokens_iterator nit = tokens.begin(); nit != it; ++nit)
          {
            std::cerr << *nit;
          }
          std::cerr << "<<<<<<< :" << e.what() << std::endl;
          throw e;
        }
      }

      while(!mSubArrays.empty()) // pop array stack pointers;
      {
        mSubArrays.pop();
      }

      if(expected_lexeme.top() != INPUT_END)
      {
        printArray(mConfigArray);

        std::cerr << "========= printing rest of state stack ===========" << std::endl;
        while(!expected_lexeme.empty())
        {
          std::cerr << expected_lexeme.top() << std::endl;
          expected_lexeme.pop();
        }
        syntaxerror(__FILE__, __LINE__, "State stack is out of order, parsing is finished and end of input is expected, but there is something else in stack.top()");
      }
      expected_lexeme.pop(); // avoid potential memory leak by running config reload over and over :)
    }
  };
}



#endif	/* __JSNON_LIKE_CONFIG_H__ */

