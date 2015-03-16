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
#  include <StringTokenizer.h>

namespace itc
{
  using namespace reflection;
  
  std::string rNumber("[-]*[[:digit:]]+|([-]*[[:digit:]]*[.][[:digit:]]+)|0x[[:xdigit:]]+");
  std::string rBool("true|false|on|off");
  std::string rString("[[:print:]]+");
  std::string rName("[[:alpha:]]+[[:alnum:]_]*");
  
  boost::regex rxNumber(rNumber,boost::regex::extended);
  boost::regex rxBool(rBool,boost::regex::extended);
  boost::regex rxString(rString,boost::regex::extended);
  boost::regex rxName(rName,boost::regex::extended);
  
  
  enum Expect
  {
    INPUT_END, MPB, MPE, NAME, COLON, VALUE, QUOTE
  };

  /**
   * [WIP: JSON-like config facility]
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
      tokens=utils::tokenizer(input,"","\",{}: \t\n");
      parser();
      stats();
      printArray(mConfigArray);
    };
    void stats()
    {
      std::cout << mConfigFile << std::endl;
      std::cout << mSubArrays.size() << std::endl;
      std::cout << aaccess(mConfigArray).size() << std::endl;
    }
   protected:
    /**
     * @TBR on completion.
     **/
    void printArray(reflection::Array& ref,std::string indent="")
    {
      std::for_each(
        aaccess(ref).begin(),aaccess(ref).end(),
        [this,&indent](const reflection::VariablePairType& var)
        {
          if(var.second.get()->getTypeName() == "Array")
          {
            std::cout  << indent << "array " << var.first << std::endl;
            std::cout << indent << "{" << std::endl;
            indent.append("  ");
            printArray(*(static_cast<reflection::Array*>(var.second.get())),indent);
            indent.erase(indent.length()-2,indent.length());
            std::cout  << indent << "}" << std::endl;
          }
          else
          {
            std::cout << indent << "var " << var.second.get()->getTypeName() << " " << var.first << "=";
            
            if(var.second.get()->getTypeName()  == "Number" )
            {
              std::cout << (static_cast<Number*>(var.second.get()))->getValue() << std::endl;
            } else if(var.second.get()->getTypeName()  == "String" )
            {
              std::cout << "\"" << (static_cast<String*>(var.second.get()))->getValue() <<"\"" << std::endl;
            } else if(var.second.get()->getTypeName()  == "Bool" )
            {
              std::cout << (static_cast<Bool*>(var.second.get()))->getValue() << std::endl;
            }
          }
        }
      );
    }
    
    void throwOnEmptyLexemesStack(const std::string& file, const int line)
    {
      if(expected_lexeme.empty())
      {
        itc::getLog()->fatal(file.c_str(), line,  "Expected lexemes stack is empty, kick the programmer at the ass (virtualy)");
        itc::getLog()->flush();
        throw TITCException<exceptions::IndexOutOfRange>(exceptions::MPConfigSyntax);
      }
    }
    
    void syntaxerror(const std::string& file, const int line, const std::string& msg)
    {
      itc::getLog()->error(file.c_str(), line, "Syntaxis error on config %s parsing: %s", mConfigFile.c_str(),msg.c_str());
      itc::getLog()->flush();
      throw TITCException<exceptions::MPConfigSyntax>(exceptions::ApplicationException);
    }

    bool saveValue()
    {
      if(!lastvalue.empty())
      {
        if(boost::regex_match(lastvalue, rxNumber))
        {
          paccess(mSubArrays.top())[lastname.top()]=std::make_shared<Number>(std::stod(lastvalue));
        }
        else if(boost::regex_match(lastvalue, rxBool))
        {
          if(lastvalue == "on" or lastvalue == "true")
          {
              paccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(true);
          }
          else if(lastvalue == "off" or lastvalue == "false")
          {
              paccess(mSubArrays.top())[lastname.top()]=std::make_shared<Bool>(false);
          } 
          else
          {
            syntaxerror(__FILE__,__LINE__,"The boolean values are only: on, off, true, false");
          }
        } 
        else if(boost::regex_match(lastvalue, rxString))
        {
          paccess(mSubArrays.top())[lastname.top()]=std::make_shared<String>(lastvalue);
        }
        else
        {
          syntaxerror(__FILE__,__LINE__,"can not determine type of the value");
        }
        return true;
      }
      return false;
    }
    
    void token_parse(const std::string& token)
    {
      switch(token.c_str()[0])
      {
        case ':':
        {
          throwOnEmptyLexemesStack(__FILE__,__LINE__);
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
              if(boost::regex_match(lastvalue, rxName))
              {
                lastname.push(lastvalue);
              }
              else
              {
                printArray(mConfigArray);
                syntaxerror(__FILE__,__LINE__,"Inapropriate variable name: "+lastvalue);
              }
              lastvalue.clear();
            break;
            case VALUE:
            break;
            case QUOTE:
              lastvalue.append(token);
            break;
            default:
              printArray(mConfigArray);
              std::cout << "lastname: " << lastname.top() << std::endl;
              std::cout << "lastvalue: " << lastvalue << std::endl;
              std::cout << "expected lexeme:" << expected_lexeme.top() << std::endl;
              syntaxerror(__FILE__,__LINE__,"Unexpected state in lexer on COLON terminal symbol");
            break;
          }
        }
        break;
        case '"':
        {
          throwOnEmptyLexemesStack(__FILE__,__LINE__);
          switch(expected_lexeme.top()){
            case QUOTE:
              expected_lexeme.pop();
              expected_lexeme.pop(); // pop VALUE;
              
              paccess(mSubArrays.top())[lastname.top()]=std::make_shared<String>(lastvalue);
              
              lastvalue.clear();
              lastname.pop();
            break;
            case VALUE:
              expected_lexeme.push(QUOTE);
              lastvalue.clear();
            break;
            default:
              printArray(mConfigArray);
              syntaxerror(__FILE__,__LINE__,"Unexpected state in lexer on '\"' terminal symbol");
          }
        }
        break;
        case '{':
        {
          throwOnEmptyLexemesStack(__FILE__,__LINE__);
          switch(expected_lexeme.top()){
            case MPB:
              expected_lexeme.pop();
              expected_lexeme.push(MPE);
              expected_lexeme.push(NAME);
              break;
            case VALUE:
              expected_lexeme.push(MPE);
              expected_lexeme.push(NAME);

              paccess(mSubArrays.top())[lastname.top()]=std::make_shared<Array>();
              mSubArrays.push(
                (static_cast<Array*>(paccess(mSubArrays.top())[lastname.top()].get()))
              );
            break;
            case QUOTE:
              lastvalue.append(token);
            break;
            
            default:
              printArray(mConfigArray);
              std::cout << "expected state: " << expected_lexeme.top() << std::endl;
              syntaxerror(__FILE__,__LINE__,"Unexpected state in lexer on '}' terminal symbol");
          }
        }
        break;
        case '}':
        {
          throwOnEmptyLexemesStack(__FILE__,__LINE__);
          switch(expected_lexeme.top())
          {
            case MPE: // MPE after MPE like {a:1,b:{c:{k:1,s:"sss",bb:on},kk:0.f}}
              expected_lexeme.pop();
              if(!mSubArrays.empty())
                mSubArrays.pop();
            break;
            case VALUE:
              saveValue();
              expected_lexeme.pop(); // VALUE POP
              expected_lexeme.pop(); // MPE POP
              if(!mSubArrays.empty()) mSubArrays.pop();
              lastvalue.clear();
              if(!lastname.empty()) lastname.pop();
            break;
            case QUOTE:
              lastvalue.append(token);
            break;
            default:
              printArray(mConfigArray);
              syntaxerror(__FILE__,__LINE__,"Unexpected state in lexer, previous state does not expect '}' to appear in the stream at this position");
            break;
          }
        }
        break;
        case ',':
        {
          throwOnEmptyLexemesStack(__FILE__,__LINE__);
          switch(expected_lexeme.top())
          {
            case VALUE:
              saveValue();
              expected_lexeme.pop();
              expected_lexeme.push(NAME);
              lastvalue.clear();
              break;
            case QUOTE:
              lastvalue.append(token);
            break;
            case MPE:
              expected_lexeme.push(NAME);
            break;
            default:
              if(expected_lexeme.empty())
              {
                expected_lexeme.push(NAME);
              }
              else
              {
                
                printArray(mConfigArray);
                std::cout << "expected state: " << expected_lexeme.top() << std::endl;
                syntaxerror(__FILE__,__LINE__,"Unexpected state in lexer, previous state does not expect ',' to appear in the stream at this position");
              }
            break;
          }
        }
        break;
        case '\t':
        case ' ':
        case '\n':
          switch(expected_lexeme.top())
          {
            case NAME:
              if(!lastvalue.empty())
              {
                expected_lexeme.pop();
                expected_lexeme.push(VALUE);
                if(boost::regex_match(lastvalue, rxName))
                {
                  lastname.push(lastvalue);
                }
                else
                {
                  printArray(mConfigArray);
                  syntaxerror(__FILE__,__LINE__,"Inapropriate variable name: "+lastvalue);
                }
                lastvalue.clear();
              }
            break;              
            case VALUE:
                if(saveValue())
                {
                  expected_lexeme.pop();
                  lastvalue.clear();
                }
              break;
            case QUOTE:
              lastvalue.append(token);
            break;
            default: // skip until nex lexeme
              break;
          }
        break;
        default:
        {
          lastvalue.append(token);
        }
        break;
      }
    }

    
    void parser()
    {
      expected_lexeme.push(INPUT_END);
      expected_lexeme.push(MPB); // '{' is the first terminal, which we expect.
      aaccess(mConfigArray)["root"]=std::make_shared<Array>();
      mSubArrays.push(
        (static_cast<Array*>(aaccess(mConfigArray)["root"].get()))
      );
      
      for(tokens_iterator it=tokens.begin();it!=tokens.end();++it)
      {
        try {
          token_parse(*it);
        } catch(const std::exception& e)
        {
          if(!lastname.empty())
          {
            std::cout << "lastname: " << lastname.top() << std::endl;
          }
          std::cout << "lastvalue: " << lastvalue << std::endl;
          
          for(tokens_iterator nit=tokens.begin();nit!=it;++nit)
          {
            std::cout << *nit;
          }
          std::cout << "<<<<<<< :" << e.what() << std::endl;
          throw e;
        }
      }
      
      while(!mSubArrays.empty()) // pop array stack pointers;
      {
        mSubArrays.pop();
      }

      if(expected_lexeme.top()!=INPUT_END)
      {
        printArray(mConfigArray);
        
        std::cout << "========= printing rest of state stack ===========" << std::endl;
        while(!expected_lexeme.empty())
        {
          std::cout << expected_lexeme.top() << std::endl;
          expected_lexeme.pop();
        }
        syntaxerror(__FILE__,__LINE__,"State stack is out of order, parsing is finished and end of input is expected, but there is something else in stack.top()");
      }
      expected_lexeme.pop(); // avoid potential memory leak by running config reload over and over :)
    }
  };
}



#endif	/* CONFIG_H */

