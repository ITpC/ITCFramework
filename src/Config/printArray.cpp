#include <Config.h>

namespace itc
{
  using namespace reflection;

  void Config::printArray(reflection::Array& ref, std::string indent)
  {
    std::for_each(
      aaccess(ref).begin(), aaccess(ref).end(),
      [this, &indent](const reflection::VariablePairType & var)
      {
        if(var.second.get()->getType() == ARRAY)
        {
          std::cout << indent << "array " << var.first << std::endl;
          std::cout << indent << "{" << std::endl;
          indent.append("  ");
          printArray(*(static_cast<reflection::Array*> (var.second.get())), indent);
          indent.erase(indent.length() - 2, indent.length());
          std::cout << indent << "}" << std::endl;
        }else
        {
          std::cout << indent << var.second.get()->getTypeName() << " " << var.first << "=";

          if(var.second.get()->getType() == NUMBER)
          {
            std::cout << (static_cast<Number*> (var.second.get()))->getValue() << std::endl;
          }else if(var.second.get()->getType() == STRING)
          {
            std::cout << "\"" << (static_cast<String*> (var.second.get()))->getValue() << "\"" << std::endl;
          }else if(var.second.get()->getType() == BOOL)
          {
            std::cout << (static_cast<Bool*> (var.second.get()))->getValue() << std::endl;
          }
        }
      }
    );
  }
}
