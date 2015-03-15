/* 
 * File:   ConfigReflection.h
 * Author: pk
 *
 * Created on 15 Март 2015 г., 20:51
 */

#ifndef CONFIGREFLECTION_H
#  define	CONFIGREFLECTION_H
#  include<stdint.h>
#  include <string>
#  include <algorithm>
#  include <functional>
#  include <memory>
#  include <list>
#  include <stack>
#  include <map>
#  include <fstream>  
#  include <iostream>

namespace itc
{
  namespace reflection
  {
    struct Variable
    {
      virtual const std::string& getTypeName() const = 0;
      virtual ~Variable() = default;
    };

    template <typename T> class TypedVariable : public Variable
    {
     public:
      typedef T value_type;

      explicit TypedVariable(const std::string& tname)
        : type_name(tname)
      {
      }

      explicit TypedVariable(const std::string& tname, const value_type& ref)
        : type_name(tname), value(ref)
      {
      }

      explicit TypedVariable(const TypedVariable<value_type>& ref)
        : value(ref.value)
      {
      }

      const TypedVariable<value_type>& operator=(TypedVariable<value_type>& ref)
      {
        value = ref.value;
      }

      value_type& expose()
      {
        return value;
      }

      const value_type& getValue() const
      {
        return value;
      }

      const std::string& getTypeName() const
      {
        return this->type_name;
      }
     private:
      std::string type_name;
      value_type value;
    };

    typedef std::shared_ptr<Variable> VariableSPtr;
    typedef std::map<std::string, VariableSPtr> VariablesMap;
    typedef std::pair<std::string, VariableSPtr> VariablePairType;

    struct Array : public TypedVariable<VariablesMap>
    {
      typedef value_type::iterator iterator;
      typedef value_type::const_iterator const_iterator;

      explicit Array()
        : TypedVariable<value_type>("Array")
      {
      }

      explicit Array(const Array& ref)
        : TypedVariable<value_type>("Array", ref.getValue())
      {
      }
    };

    struct String : public TypedVariable<std::string>
    {
      explicit String(const std::string& val = "")
      : TypedVariable<value_type>("String", val)
      {
      }

      explicit String(const String& ref)
      : TypedVariable<value_type>("String", ref.getValue())
      {
      }
    };

    struct Float : public TypedVariable<float>
    {
      explicit Float(const float& val = 0.0f)
      : TypedVariable<value_type>("Float", val)
      {
      }

      explicit Float(const Float& ref)
      : TypedVariable<value_type>("Float", ref.getValue())
      {
      }
    };
    
    struct Integer : public TypedVariable<int64_t>
    {
      explicit Integer(const float& val = 0LL)
      : TypedVariable<value_type>("Integer", val)
      {
      }

      explicit Float(const Float& ref)
      : TypedVariable<value_type>("Integer", ref.getValue())
      {
      }
    };
  }
}
#  define access(x) (x.expose())

/**
 *@brief testcase for reflection   
 * 
 * using namespace itc;
 * 
 *void printArray(reflection::Array& ref)
  {
    std::for_each(
      access(ref).begin(),access(ref).end(),
      [](const reflection::VariablePairType& var)
      {
        if(var.second.get()->getTypeName() == "Array")
        {
          std::cout << "============ printing for subarray  " << var.first << " begin ===================" << std::endl;
          printArray(*(static_cast<reflection::Array*>(var.second.get())));
          std::cout << "============ subarray printing  ends here ====================" << std::endl;
        }
        else
        {
            std::cout << "variable type: " << var.second.get()->getTypeName() << ", with name: " << var.first << std::endl;
        }
      }
    );
  
 * int main()
  {
    reflection::String tmpstr("1234567");
    reflection::String tmpstr2("1234569");
    reflection::String tmpstr3("FFF4569");


    reflection::Array anArray;
    access(anArray)["tmpstr"]=std::make_shared<reflection::String>(tmpstr);
    access(anArray)["tmpstr2"]=std::make_shared<reflection::String>(tmpstr2);
    reflection::Array anArray1;
    access(anArray1)["subarray"]=std::make_shared<reflection::Array>(anArray);
    access(anArray1)["tmpstr3"]=std::make_shared<reflection::String>(tmpstr3);

    printArray(anArray1);
  }
 *
 **/

#endif	/* CONFIGREFLECTION_H */

