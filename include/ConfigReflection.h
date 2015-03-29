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
      Variable& operator[](const std::string&);
      const Variable& operator[](const std::string&) const;
      virtual ~Variable() = default;
    };

    typedef std::shared_ptr<Variable> VariableSPtr;
    typedef std::map<std::string, VariableSPtr> VariablesMap;
    typedef std::pair<std::string, VariableSPtr> VariablePairType;

    /**
     * @brief The template for types defenitions.
     * 
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

      const value_type& expose() const
      {
        return value;
      }

      const value_type& getValue() const
      {
        return value;
      }

      TypedVariable<T>& operator=(const TypedVariable<T>& ref)
      {
        value = ref.value;
        return static_cast<TypedVariable<T>&> (*this);
      }

      Variable& operator[](const std::string & var)
      {
        if(type_code == ARRAY)
        {
          return *(((static_cast<VariablesMap> (value))[var]).get());
        }
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }

      const Variable& operator[](const std::string & var) const
      {
        if(type_code == ARRAY)
        {
          return *(((static_cast<VariablesMap> (value))[var]).get());
        }
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }

      const std::string getTypeName() const
      {
        switch(type_code){
          case ARRAY: return "Array";
          case NUMBER: return "Number";
          case STRING: return "String";
          case BOOL: return "Bool";
          default:
            throw TITCException<exceptions::UndefinedType>(exceptions::Reflection);
        }
      }

      const VarType & getType() const
      {
        return type_code;
      }

     private:
      VarType type_code;
      value_type value;
    };

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

      explicit String(String& ref)
        : TypedVariable<value_type>(STRING, ref.getValue())
      {
      }

      const std::string& str() const
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

      operator std::string() const
      {
        return expose();
      }
      
      String& operator=(const char* ref)
      {
        expose()=std::string(ref);
        return (*this);
      }

      String& operator=(const std::string& ref)
      {
        expose()=ref;
        return (*this);
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
      
      Array& operator=(const Array& ref)
      {
        (static_cast<Array::value_type>(expose()))=ref.getValue();
        return (*this);
      }
      
      Variable& operator[](const std::string& name)
      {
        return *(expose()[name].get());
      }

      const Variable& operator[](const std::string& name) const
      {
        return *((static_cast<VariablesMap> (expose()))[name].get());
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
          throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
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
              throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
            }
          }else
          {
            throw TITCException<exceptions::IndexOutOfRange>(exceptions::Reflection);
          }
        }else
        {
          throw TITCException<exceptions::IndexOutOfRange>(exceptions::Reflection);
        }
      }
    };

    template <typename T> T& cast(Variable& var)
    {
      bool is_a_proper_class = std::is_base_of<Variable, T>::value;
      T tmp;

      if(!is_a_proper_class)
      {
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }

      if(var.getType() == tmp.getType())
      {
        return *((static_cast<T*> (&var)));
      }else
      {
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }
    }

    template <typename T> const T& cast(const Variable& var)
    {
      bool is_a_proper_class = std::is_base_of<Variable, T>::value;
      T tmp;

      if(!is_a_proper_class)
      {
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }

      if(var.getType() == tmp.getType())
      {
        return *((static_cast<const T*> (&var)));
      }else
      {
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }
    }

    template <typename T> T& cast(VariableSPtr& var)
    {
      bool is_a_proper_class = std::is_base_of<Variable, T>::value;
      T tmp;

      if(!is_a_proper_class)
      {
        throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
      }

      if(Variable * ptr = var.get())
      {
        if(ptr->getType() == tmp.getType())
        {
          return *((static_cast<T*> (ptr)));
        }else
        {
          throw TITCException<exceptions::InvalidTypecast>(exceptions::Reflection);
        }
      }
      throw TITCException<exceptions::NullPointerException>(exceptions::Reflection);
    }
  }
}
/**
 * the macro-definitions for decorating the TypedVariable<T>::expose method.
 **/
#  define aaccess(x) (x.expose())
#  define paccess(x) (x->expose())


#endif	/* CONFIGREFLECTION_H */

