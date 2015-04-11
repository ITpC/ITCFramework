/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ServiceFacade.h 22 2015-03-13 10:53:00Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef SERVICEFACADE_H
#  define	SERVICEFACADE_H

#  include <abstract/Service.h>
#  include <TSLog.h>
#  include <mutex>
#  include <atomic>

namespace itc
{

  /**
   * @brief The itc::ServiceFacade have to be inherited by classes those are 
   * intend to act as a services managed by an itc::ServiceManager.
   * The implementation of service should initialize a ServiceFacade ctor
   * with std::string value which is a unique name of your service.
   * The implementation of service should implement 3 methods as well:
   * onStart, onStop and onDestroy. First two will be called by ServiceFacade
   * on start and on stop of the service. The method onDestroy is to be used
   * in the implementation destructor.
   * 
   **/
  class ServiceFacade : public itc::abstract::Service
  {
   private:
    std::mutex mMutex;
    std::atomic<bool> mUp;
    std::string mName;
   public:

    explicit ServiceFacade(const std::string& name)
      : mMutex(), mUp(false), mName(name)
    {
      std::lock_guard<std::mutex> sync(mMutex);
      ::itc::getLog()->info("Service %s is created", mName.c_str());
    }

    const bool isup() const
    {
      return mUp;
    }

    void restart()
    {
      stop();
      start();
    }

    const bool isdown() const
    {
      return !mUp;
    }

    const std::string& getName()
    {
      std::lock_guard<std::mutex> sync(mMutex);
      return mName;
    }

    void stop()
    {
      std::lock_guard<std::mutex> sync(mMutex);
      if(mUp)
      {
        try
        {
          onStop();
          this->mUp = false;
        }catch(const std::exception& e)
        {
          ::itc::getLog()->error(__FILE__, __LINE__, "Cought an exception on service %s start: %s", mName.c_str(), e.what());
        }
      }
    }

    void start()
    {
      std::lock_guard<std::mutex> sync(mMutex);
      if(!mUp)
      {
        try
        {
          onStart();
          this->mUp = true;
        }catch(const std::exception& e)
        {
          this->mUp = false;
          ::itc::getLog()->error(__FILE__, __LINE__, "Cought an exception on service %s start: %s", mName.c_str(), e.what());
          throw;
        }
      }
    }

    virtual void onStart() = 0;
    virtual void onStop() = 0;
    virtual void onDestroy() = 0;

    ~ServiceFacade()
    {
      std::lock_guard<std::mutex> sync(mMutex);
      ::itc::getLog()->info("Service %s is destroyed", mName.c_str());
    }
  };
}

#endif	/* SERVICEFACADE_H */

