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
#include <Val2Type.h>
#include <sys/Nanosleep.h>
#include <mutex>

namespace itc {

    /**
     * \brief Thread safe singleton template (a singleton holder pattern)
     * This singletone may not be inherited or instantiated. You have to call 
     * getInstance() to get a single unique instance of a class T. This template
     * does not prevent you from instantiating the class T somewhere else.
     * 
     * Note that use of this class can syncronize your threads
     * because of mutex locking.
     * 
     **/
    template<class T> class Singleton {
    private:
      explicit Singleton()=delete;
      explicit Singleton(const Singleton&)=delete;
      explicit Singleton(Singleton&)=delete;

    public:
      static std::shared_ptr<T> getInstance()
      {
        std::lock_guard<std::mutex> sync(mMutex);

        if (mInstance.get()) {
          return mInstance;
        } else {
          std::shared_ptr<T> tmp=std::make_shared<T>();
          mInstance.swap(tmp);
          return mInstance;
        }
      }

      static std::shared_ptr<T> getExisting()
      {
        std::lock_guard<std::mutex> sync(mMutex);

        if (mInstance.get()) {
          return mInstance;
        } else {
          throw std::runtime_error("This singletone is not initialized yet");
        }
      }
      
      template <typename... Args> static std::shared_ptr<T> getInstance(Args...args)
      {
        std::lock_guard<std::mutex> sync(mMutex);

        if (mInstance.get()) {
          return mInstance;
        } else {
          std::shared_ptr<T> tmp=std::make_shared<T>(args...);
          mInstance.swap(tmp);
          return mInstance;
        }
      }

    protected:
      static std::mutex mMutex;
      static std::shared_ptr<T> mInstance;

      virtual ~Singleton() = 0;

      static bool tryDestroyInstance()
      {
        std::lock_guard<std::mutex> sync(mMutex);
        if (mInstance.unique()) {
          std::shared_ptr<T> tmp((T*) 0);
          mInstance.swap(tmp);
          return true;
        }
        return false;
      }

      static void destroyInstance()
      {
        std::lock_guard<std::mutex> sync(mMutex);
        itc::sys::Nap aTimer;
        while ((!mInstance.unique()) && mInstance.use_count()) {
          aTimer.usleep(10);
        }
        std::shared_ptr<T> tmp((T*) 0);
        mInstance.swap(tmp);
      }
    };

    // Initialisation
    template <typename T> std::mutex Singleton<T>::mMutex;
    template <typename T> std::shared_ptr<T> Singleton<T>::mInstance(static_cast<T*>(nullptr));
}

#endif
