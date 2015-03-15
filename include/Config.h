/* 
 * File:   Config.h
 * Author: pk
 *
 * Created on 13 Март 2015 г., 23:24
 */

#ifndef CONFIG_H
#  define	CONFIG_H
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

namespace itc
{
  using namespace reflection;
  
  std::string rNumber("[[:digit:]]+|([[:digit:]]*[.][[:digit:]]+)|0x[[:xdigit:]]+");
  std::string rBool("true|false|on|off");
  std::string rString("[[:print:]]");
  std::string rName("[[:alpha:]]+[[:alnum:]_]*");
  
  boost::regex rxNumber(rNumber,boost::regex::extended);
  boost::regex rxBool(rBool,boost::regex::extended);
  boost::regex rxString(rString,boost::regex::extended);
  boost::regex rxName(rName,boost::regex::extended);
  
  
  enum Expect
  {
    MPB, MPE, NAME, COLON, VALUE, QUOTE
  };

  /**
   * [MessagePack-like config parser]
   * 
   * BEGIN_MP = '{'
   * END_MP = '}'
   * 
   * mp = BEGIN_MP kvpair_list END_MP
   * kvpair = NAME ':' value
   * kvpair_list = kvpair_list ',' kvpair
   * value = string || number || mp
   * string = "[[:print:]]"
   * number = "[[:digit:]]+ || ([[:digit:]]+|[[:digit:]]*[.][[:digit:]]+) || 0x[[:xdigit:]]+
   * 
   **/
  
  class Config
  {
   private:
    std::stack<Expect> expected_lexeme;
    std::stack<std::string> lastname;
    std::list<std::string> tokens;
    std::stack<Array&> mSubArrays;
    std::string lastvalue;
    std::string mConfigFile;
    Array mConfigArray;


   public:

    Config(const std::string& fname)
    {
      std::fstream fs(fname,std::ios_base::in);
      std::string input;
      while(fs.good())
      {
        char c;
        fs.get(c);
        input.append(1u,c);
      }
      tokens=utils::tokenizer(input,"","\",{} \t\n");
      parser();
    };
    
   protected:
    void throwOnEmptyLexemesStack()
    {
      if(expected_lexeme.empty())
      {
        itc::getLog()->fatal(__FILE__, __LINE__, "Expected lexemes stack is empty, kick the programmer at the ass (virtualy)");
        throw TITCException<exceptions::IndexOutOfRange>(exceptions::MPConfigSyntax);
      }
    }
    
    void syntaxerror(const std::string& msg)
    {
      itc::getLog()->error(__FILE__, __LINE__, "Syntaxis error on config %s parsing: %s", mConfigFile.c_str(),msg.c_str());
      throw TITCException<exceptions::MPConfigSyntax>(exceptions::ApplicationException);
    }
    
    void parser()
    {
      while(!expected_lexeme.empty())
      {
        expected_lexeme.pop();
      }
      expected_lexeme.push(MPB);
      std::for_each(
        tokens.begin(), tokens.end(),
        [this](const std::string & token){
          switch(token.c_str[0])
          {
            case ':':
            {
              throwOnEmptyLexemesStack();
              switch(expected_lexeme.top()){
                case COLON: // there were some delimiters between name and colon.
                  expected_lexeme.pop();
                  expected_lexeme.push(VALUE);
                  // add the name to stack;
                  lastname.push(lastvalue);
                  lastvalue.clear();
                break;
                case NAME: // there were no delimiters between name and colon.                  
                  expected_lexeme.pop();
                  expected_lexeme.push(VALUE);
                  if(boost::regex_match(lastvalue, result, rxName))
                  {
                    lastname.push(lastvalue);
                  }
                  else
                  {
                    syntaxerror("Inapropriate variable name: "+lastvalue);
                  }
                  lastvalue.clear();
                break;
                case QUOTE:
                  lastvalue.append(token);
                break;
                default:
                  syntaxerror("Unexpected state in lexer on COLON terminal symbol");
                break;
              }
            }
            break;
            case '"':
            {
              throwOnEmptyLexemesStack();
              switch(expected_lexeme.top()){
                case QUOTE:
                  expected_lexeme.pop();
                  expected_lexeme.pop(); // pop VALUE;
                  // add the name to stack;
                  aaccess(mConfigFile)[lastname.top()]=std::make_shared<String>(lastvalue);
                  lastvalue.clear();
                  lastname.pop();
                break;
                case VALUE:
                  expected_lexeme.push(QUOTE);
                  lastvalue.clear();
                break;
              }
            }
            break;
            case '{':
            {
              throwOnEmptyLexemesStack();
              switch(expected_lexeme.top()){
                case MPB:
                  expected_lexeme.pop();
                  expected_lexeme.push(MPE);
                  break;
                case VALUE:
                  expected_lexeme.push(MPE);
                  expected_lexeme.push(NAME);
                  if(mSubArrays.empty())
                  {
                    Array tmp;
                    aaccess(tmp)[lastname.top()]=std::make_shared<Array>();
                    aaccess(mConfigArray)[lastname.top()]=tmp;
                    mSubArrays.push(
                      *(static_cast<Array*>(aaccess(mConfigArray)[lastname.top()].get()))
                    );
                  }
                  else
                  {
                    aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Array>();
                    mSubArrays.push(
                      *(static_cast<Array*>(aaccess(mSubArrays)[lastname.top()].get()))
                    );
                  }
                break;
                case QUOTE:
                  lastvalue.append(token);
                break;
                default:
                  syntaxerror("Unexpected state in lexer on '}' terminal symbol");
              }
            }
            break;
            case '}':
            {
              throwOnEmptyLexemesStack();
              switch(expected_lexeme.top())
              {
                case MPE: // MPE after MPE like {a:1,b:{c:{k:1,s:"sss",bb:on},kk:0.f}}
                  expected_lexeme.pop();
                  if(!mSubArrays.empty())
                    mSubArrays.pop();
                break;
                case VALUE:
                  boost::cmatch result;
                  if(boost::regex_match(lastvalue, result, rxNumber))
                  {
                    aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Number>(std::strod(lastvalue));
                  }
                  else if(boost::regex_match(lastvalue, result, rxBool))
                  {
                    if(lastvalue == "on" or lastvalue == "true")
                    {
                      aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(true);
                    }
                    else
                    {
                      aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(false);
                    }
                  } 
                  else if(boost::regex_match(lastvalue, result, rxString))
                  {
                    aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<String>(lastvalue);
                  }
                  expected_lexeme.pop();
                  if(!mSubArrays.empty())
                    mSubArrays.pop();
                  lastvalue.clear();
                break;
                case QUOTE:
                  lastvalue.append(token);
                break;
                default:
                  syntaxerror("Unexpected state in lexer, previous state does not expect '}' to appear in the stream at this position");
                break;
              }
            }
            break;
            case ',':
            {
              throwOnEmptyLexemesStack();
              switch(expected_lexeme.top())
              {
                case VALUE:
                  boost::cmatch result;
                  if(boost::regex_match(lastvalue, result, rxNumber))
                  {
                    aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Number>(std::strod(lastvalue));
                  }
                  else if(boost::regex_match(lastvalue, result, rxBool))
                  {
                    if(lastvalue == "on" or lastvalue == "true")
                    {
                      aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(true);
                    }
                    else
                    {
                      aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(false);
                    }
                  } 
                  else if(boost::regex_match(lastvalue, result, rxString))
                  {
                    aaccess(mSubArrays.top())[lastname.top()]=std::make_shared<String>(lastvalue);
                  }
                  expected_lexeme.pop();
                  lastvalue.clear();
                  break;
                case QUOTE:
                  lastvalue.append(token);
                break;
                default:
                  syntaxerror("Unexpected state in lexer, previous state does not expect ',' to appear in the stream at this position");
                break;
              }
            }
            break;
            default:
            {
              lastvalue.append(token);
            }
            break;
          }
        }
      );
    }
  };
}



#endif	/* CONFIG_H */

