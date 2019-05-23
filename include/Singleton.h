/**
 * Copyright Pavel Kraynyukhov 2004 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Singleton.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>

namespace itc {

    /**
     * \brief Thread safe (c++14 - atomic) singleton template (a singleton holder pattern)
     * This singleton may not be inherited or instantiated. You have to call 
     * getInstance() to get a single unique instance of a class T. This template
     * does not prevent you from instantiating the class T somewhere else.
     * 
     **/
    template<class T> class Singleton {
    private:
      explicit Singleton()=delete;
      explicit Singleton(const Singleton&)=delete;
      explicit Singleton(Singleton&)=delete;

    public:
      template <typename... Args> static const std::shared_ptr<T>& getInstance(Args...args)
      {
        static std::shared_ptr<T> instance=std::make_shared<T>(args...);
        return instance;
      }

    protected:
      virtual ~Singleton() = 0;
    };

}

#endif
