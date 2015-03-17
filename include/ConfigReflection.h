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
#  include <ITCException.h>
#  include <type_traits>

namespace itc
{
  /**
   * @brief set of classes to emulate reflection for configuration files.
   * 
   **/
  namespace reflection
  {

    enum VarType
    {
      ARRAY, NUMBER, STRING, BOOL
    };

    /**
     * @brief Variable is interface for reflected types.
     **/
    struct Variable
    {
      virtual const std::string getTypeName() const = 0;
      virtual const VarType& getType() const = 0;
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

      explicit TypedVariable(const VarType& tc)
        : type_code(tc)
      {
      }

      explicit TypedVariable(const VarType& tc, const value_type& ref)
        : type_code(tc), value(ref)
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

      const std::string getTypeName() const
      {
        switch(type_code){
          case ARRAY: return "Array";
          case NUMBER: return "Number";
          case STRING: return "String";
          case BOOL: return "Bool";
          default:
            throw TITCException<exceptions::Reflection>(exceptions::UndefinedType);
        }
      }

      const VarType& getType() const
      {
        return type_code;
      }

     private:
      VarType type_code;
      value_type value;
    };

    typedef std::shared_ptr<Variable> VariableSPtr;
    typedef std::map<std::string, VariableSPtr> VariablesMap;
    typedef std::pair<std::string, VariableSPtr> VariablePairType;

    /**
     * @brief a String class
     **/
    struct String : public TypedVariable<std::string>
    {

      explicit String(const std::string& val = "")
        : TypedVariable<value_type>(STRING, val)
      {
      }

      explicit String(const String& ref)
        : TypedVariable<value_type>(STRING, ref.getValue())
      {
      }

      const std::string& to_stdstring() const
      {
        return getValue();
      }

      const bool operator==(const String& ref) const
      {
        return getValue() == ref.getValue();
      }

      const bool operator!=(const String& ref) const
      {
        return getValue() != ref.getValue();
      }

      const bool operator>(const String& ref) const
      {
        return getValue() > ref.getValue();
      }

      const bool operator<(const String& ref) const
      {
        return getValue() < ref.getValue();
      }

    };

    /**
     * @brief a Number class.
     **/
    struct Number : public TypedVariable<double>
    {

      explicit Number(const double& val = 0.0f)
        : TypedVariable<value_type>(NUMBER, val)
      {
      }

      explicit Number(const Number& ref)
        : TypedVariable<value_type>(NUMBER, ref.getValue())
      {
      }

      const bool operator==(const Number& ref) const
      {
        return getValue() == ref.getValue();
      }

      const bool operator!=(const Number& ref) const
      {
        return getValue() != ref.getValue();
      }

      const bool operator>(const Number& ref) const
      {
        return getValue() > ref.getValue();
      }

      const bool operator<(const Number& ref) const
      {
        return getValue() < ref.getValue();
      }

      const int toInt() const
      {
        return getValue();
      }

      const long toLong() const
      {
        return getValue();
      }

      const long long toLongLong() const
      {
        return getValue();
      }

      const float toFloat() const
      {
        return getValue();
      }

      const double& toDouble() const
      {
        return getValue();
      }

      const double operator+(const Number& ref) const
      {
        return toDouble() + ref.toDouble();
      }

      const double operator-(const Number& ref) const
      {
        return getValue() - ref.getValue();
      }

      const double operator/(const Number& ref) const
      {
        return getValue() / ref.getValue();
      }

      const double operator*(const Number& ref) const
      {
        return getValue() * ref.getValue();
      }

      const long long operator%(const Number& ref) const
      {
        return toLongLong() % ref.toLongLong();
      }
    };

    /**
     * @brief a Bool class.
     **/
    struct Bool : public TypedVariable<bool>
    {

      explicit Bool(const bool& val = false)
        : TypedVariable<value_type>(BOOL, val)
      {
      }

      explicit Bool(const Bool& ref)
        : TypedVariable<value_type>(BOOL, ref.getValue())
      {
      }

      const bool operator==(const Bool& ref) const
      {
        return getValue() == ref.getValue();
      }

      const bool operator!=(const Bool& ref) const
      {
        return getValue() != ref.getValue();
      }
    };

    /**
     * @brief an Array class wrapper around the std::map
     **/
    struct Array : public TypedVariable<VariablesMap>
    {
      typedef value_type::iterator iterator;
      typedef value_type::const_iterator const_iterator;

      explicit Array()
        : TypedVariable<value_type>(ARRAY)
      {
      }

      explicit Array(const Array& ref)
        : TypedVariable<value_type>(ARRAY, ref.getValue())
      {
      }

      VariableSPtr& operator[](const std::string& name)
      {
        iterator it = expose().find(name);

        if(it != expose().end())
        {
          return it->second;
        }else
        {
          throw TITCException<exceptions::Reflection>(exceptions::IndexOutOfRange);
        }
      }

      /**
       * as far as the dectype(auto) is not acceptable in C++11 and not working
       * correctly in C++1y (gcc 4.8.4) we are forced to use something else :/
       **/
      template <typename T> T& access(const std::string& name)
      {
        bool is_a_proper_class = std::is_base_of<Variable, T>::value;
        
        if(!is_a_proper_class)
        {
          throw TITCException<exceptions::Reflection>(exceptions::InvalidTypecast);
        }

        iterator it = expose().find(name);
        T tmp;

        if(it != expose().end())
        {
          if(Variable * ptr = it->second.get())
          {
            if(ptr->getType() == tmp.getType())
            {
              return *(static_cast<T*> (it->second.get()));
            }else
            {
              throw TITCException<exceptions::Reflection>(exceptions::InvalidTypecast);
            }
          }else
          {
            throw TITCException<exceptions::Reflection>(exceptions::IndexOutOfRange);
          }
        }else
        {
          throw TITCException<exceptions::Reflection>(exceptions::IndexOutOfRange);
        }
      }
    };
    
    template <typename T> T& cast(const VariableSPtr& var)
    {
      bool is_a_proper_class = std::is_base_of<Variable, T>::value;
      T tmp;
      
      if(!is_a_proper_class)
      {
        throw TITCException<exceptions::Reflection>(exceptions::InvalidTypecast);
      }
      
      if(Variable* ptr=var.get())
      {
        if(ptr->getType()==tmp.getType())
        {
          return *((static_cast<T*> (ptr)));
        }
        else
        {
          throw TITCException<exceptions::Reflection>(exceptions::InvalidTypecast);
        }
      }
      throw TITCException<exceptions::Reflection>(exceptions::NullPointerException);
    }
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

