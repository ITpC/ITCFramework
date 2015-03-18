#include <Config.h>

namespace itc
{
  using namespace reflection;

  void Config::parser(const std::string& token)
  {
    switch(token.c_str()[0])
    {
      case ':':
      {
        throwOnEmptyLexemesStack(__FILE__, __LINE__);
        switch(expected_lexeme.top())
        {
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
            }else
            {
              printArray(mConfigArray);
              syntaxerror(__FILE__, __LINE__, "Inapropriate variable name: " + lastvalue);
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
            std::cerr << "lastname: " << lastname.top() << std::endl;
            std::cerr << "lastvalue: " << lastvalue << std::endl;
            std::cerr << "expected lexeme:" << expected_lexeme.top() << std::endl;
            syntaxerror(__FILE__, __LINE__, "Unexpected state in lexer on COLON terminal symbol");
            break;
        }
      }
        break;
      case '"':
      {
        throwOnEmptyLexemesStack(__FILE__, __LINE__);
        switch(expected_lexeme.top())
        {
          case QUOTE:
            expected_lexeme.pop();
            expected_lexeme.pop(); // pop VALUE;

            paccess(mSubArrays.top())[lastname.top()] = std::make_shared<String>(lastvalue);

            lastvalue.clear();
            lastname.pop();
            break;
          case VALUE:
            expected_lexeme.push(QUOTE);
            lastvalue.clear();
            break;
          default:
            printArray(mConfigArray);
            syntaxerror(__FILE__, __LINE__, "Unexpected state in lexer on '\"' terminal symbol");
        }
      }
        break;
      case '{':
      {
        throwOnEmptyLexemesStack(__FILE__, __LINE__);
        switch(expected_lexeme.top())
        {
          case MPB:
            expected_lexeme.pop();
            expected_lexeme.push(MPE);
            expected_lexeme.push(NAME);
            break;
          case VALUE:
            expected_lexeme.push(MPE);
            expected_lexeme.push(NAME);

            paccess(mSubArrays.top())[lastname.top()] = std::make_shared<Array>();
            mSubArrays.push(
              (static_cast<Array*> (paccess(mSubArrays.top())[lastname.top()].get()))
              );
            break;
          case QUOTE:
            lastvalue.append(token);
            break;

          default:
            printArray(mConfigArray);
            std::cerr << "expected state: " << expected_lexeme.top() << std::endl;
            syntaxerror(__FILE__, __LINE__, "Unexpected state in lexer on '}' terminal symbol");
        }
      }
        break;
      case '}':
      {
        throwOnEmptyLexemesStack(__FILE__, __LINE__);
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
            syntaxerror(__FILE__, __LINE__, "Unexpected state in lexer, previous state does not expect '}' to appear in the stream at this position");
            break;
        }
      }
        break;
      case ',':
      {
        throwOnEmptyLexemesStack(__FILE__, __LINE__);
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
            }else
            {

              printArray(mConfigArray);
              std::cerr << "expected state: " << expected_lexeme.top() << std::endl;
              syntaxerror(__FILE__, __LINE__, "Unexpected state in lexer, previous state does not expect ',' to appear in the stream at this position");
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
              }else
              {
                printArray(mConfigArray);
                syntaxerror(__FILE__, __LINE__, "Inapropriate variable name: " + lastvalue);
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

  void Config::parse()
  {
    if(tokens.empty())
    {
      itc::getLog()->error(__FILE__,__LINE__,"Config::parshe(): No input");
      throw TITCException<exceptions::ConfigSyntax>(exceptions::IndexOutOfRange);
    }
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
}