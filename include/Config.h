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
namespace itc
{
  using namespace reflection;
  
  enum Expect
  {
    MPB, MPE, NAME, COLON, VALUE, QUOTE
  };

  /**
   * [MessagePack parser]
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
    std::list<std::string> tokens;
    Array mConfigArray;
    std::stack<Array&> mSubArrays;
    std::string<std::string> lastarrayname;
    std::string lastvalue;
    std::string mConfigFile;
    std::stack<std::string> lastname;

   public:

    Config()
    {
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
                  // add the name to stack;
                  lastname.push(lastvalue);
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
                  access(mConfigFile)[lastname.top()]=std::make_shared<String>(lastvalue);
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
                      access(tmp)[lastname.top()]=std::make_shared<Array>();
                      access(mConfigArray)[lastname.top()]=tmp;
                      mSubArrays.push(
                        *(static_cast<Array*>(access(mConfigArray)[lastname.top()].get()))
                      );
                    }
                    else
                    {
                      access(mSubArrays.top())[lastname.top()]=std::make_shared<Array>();
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
                case MPE: // there were some delimiters between last value and '}'
                  expected_lexeme.pop();
                  find(lastname.top())
                  
                break;
                case VALUE:
                  // TODO add value to variable
                  expected_lexeme.pop();
                  if(!expected_lexeme.empty()) // stack is not empty
                  {
                    if(expected_lexeme.top()==MPE) // end of conf
                    {
                      expected_lexeme.pop();
                      return;
                    }
                    else syntaxerror("Unexpected state in lexer, expecting value for variable, recieved '}'");
                  }
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
                case MPE: // end of numeric value
                  expected_lexeme.push(NAME);
                break;
                case VALUE:
                  // TODO: add value to variable
                  expected_lexeme.pop();
                  break;
                case QUOTE:
                  lastvalue.append(token);
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

