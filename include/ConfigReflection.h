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
    /**
     * @brief set of classes to emulate reflection for configuration files.
     * 
     **/
    
    /**
     * @brief base class for reflection.
     **/
    struct Variable
    {
      virtual const std::string& getTypeName() const = 0;
      virtual ~Variable() = default;
    };

    /**
     * @brief The template for types defenitions.
     * 
     * @TODO: 1. add a type_code attribute and define a enum for type-codes.
     * 2. add a method  const TypeCode getType() const.
     **/
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
    
    
    /**
     * @brief an Array class wrapper around the std::map
     **/
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

    /**
     * @brief a Number class.
     **/
    struct Number : public TypedVariable<double>
    {
      explicit Number(const double& val = 0.0f)
      : TypedVariable<value_type>("Number", val)
      {
      }

      explicit Number(const Number& ref)
      : TypedVariable<value_type>("Number", ref.getValue())
      {
      }
    };

    /**
     * @brief a Bool class.
     **/
    struct Bool : public TypedVariable<bool>
    {
      explicit Bool(const bool& val = false)
      : TypedVariable<value_type>("Bool", val)
      {
      }

      explicit Bool(const Bool& ref)
      : TypedVariable<value_type>("Bool", ref.getValue())
      {
      }
    };
  }
}
/**
 * the macro-definitions for decorating the TypedVariable<T>::expose method.
 **/
#  define aaccess(x) (x.expose())
#  define paccess(x) (x->expose())

/**
 *@brief testcase for reflection   
 * 
 * using namespace itc;
 * 
 *void printArray(reflection::Array& ref)
  {
    std::for_each(
      aaccess(ref).begin(),aaccess(ref).end(),
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

