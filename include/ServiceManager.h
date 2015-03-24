/**
 * Copyright (c) 2007-2015, Pavel Kraynyukhov.
 *  
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted under the terms of the General Public License version 2
 * (GPLv2), provided that the above copyright notice and this paragraph and the
 * following two paragraphs and the "LICENSE" file appear in all modified or
 * unmodified copies of the software "AS IS" and without any changes.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 * 
 * 
 * $Id: ServiceManager.h 22 2015-03-11 00:01:03Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 **/


#ifndef SERVICEMANAGER_H
#  define	SERVICEMANAGER_H
#include <abstract/Service.h>
#include <exception>
#include <algorithm>
#include <Singleton.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <map>
#include <set>
#include <list>
#include <TSLog.h>

namespace itc
{
 class ServiceManager
 {
 public:  
  typedef std::shared_ptr<abstract::Service> ServiceSPtr;
  typedef std::map<std::string,ServiceSPtr> ServiceRegistryMap;
  typedef std::pair<std::string,ServiceSPtr> ServicePair;
  typedef std::set<std::string> string_set;
  typedef std::map<std::string,string_set> OrderMap;
  typedef std::pair<std::string,string_set> OrderMapPair;
  typedef string_set::iterator SSIterator;
  typedef OrderMap::iterator OMIterator;
  typedef ServiceRegistryMap::iterator SRMIterator;
  /**
   * @brief This class is a factory of itc::abstract::Service offsprings, dedicated
   * to control lifetime of the Service objects and thier dependencies on each
   * another. This class is a lot like init facilities of the *nix/GNU Linux
   * operating systems, where every service has its lifetime and clear dependency
   * on other services. 
   * 
   * The user is controlling dependencies of the Service objects by adding them
   * with addService<T>() and addServiceAfter<T>(std::const string&) methods.
   * 
   * Start and stop are controlled by the startService(std::const string&), 
   * startAll(), stopService(std::const string&), stopAll() methods.
   * 
   * This class is ensuring that no service has more then one instance and 
   * it is started not more then once. This class assures that the services
   * are stopped and destroyed in proper order when the ServiceManager object 
   * is destroyed. In order to properly implement your own services, inherit from
   * itc::ServiceFacade. 
   * 
   **/
  explicit ServiceManager()
  {
  }

  template <typename T> void addService()
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    mServiceRegistry.insert(ServicePair(Singleton<T>::getInstance()->getName(),Singleton<T>::getInstance()));
    mOrder[Singleton<T>::getInstance()->getName()]=std::set<std::string>();
  }

  template <typename T> void addServiceAfter(const std::string& name)
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    SRMIterator srmit=mServiceRegistry.find(name);
    if(srmit==mServiceRegistry.end())
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "There is no such service [%s] registered, ignoring dependent services",
        name.c_str()
      );
      return;      
    }
    mServiceRegistry.insert(
      ServicePair(
        Singleton<T>::getInstance()->getName(),Singleton<T>::getInstance()
      )
    );
    if(OMCrossDep(name,Singleton<T>::getInstance()->getName()))
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Service %s is already depend on %s, cross dependency is ignored",
        name.c_str(),Singleton<T>::getInstance()->getName().c_str()
      );
      return;
    }
    OMIterator it=mOrder.find(name);
    if(it!=mOrder.end())
    {
      it->second.insert(Singleton<T>::getInstance()->getName());
    }
    else
    {
      string_set tmp;
      tmp.insert(Singleton<T>::getInstance()->getName());
      mOrder.insert(OrderMapPair(name,tmp));
    }
  }
  
  template <typename T> const std::shared_ptr<T>& getService(const std::string& name)
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    SRMIterator srmit=mServiceRegistry.find(name);
    if(srmit!=mServiceRegistry.end())
    {
      if(srmit->second.get())
      {
        return srmit->second;
      }
    }
    throw TITCException<exceptions::NullPointerException>(exceptions::ITCGeneral);
  }
  
  bool OMCrossDep(const std::string& svc1, const std::string& svc2)
  {
    OMIterator omit=mOrder.find(svc1);
    if(omit!=mOrder.end())
    {
      SSIterator ssiter=omit->second.find(svc2);
      if(ssiter!=omit->second.end())
      {
        return true;
      }
    }
    omit=mOrder.find(svc2);
    if(omit!=mOrder.end())
    {
      SSIterator ssiter=omit->second.find(svc1);
      if(ssiter!=omit->second.end())
      {
        return true;
      }
    }
    return false;
  }
  
  void startAll()
  {
   sys::RecursiveSyncLock synchronize(mMutex);
   std::for_each(
      mServiceRegistry.begin(),mServiceRegistry.end(),
      [this](const ServicePair& ref)
      {
       startService(ref.first);
      }
   );
  }
  
  std::list<std::string> finddeps(const std::string& name)
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    std::list<std::string> result;
    
    std::for_each(
      mOrder.begin(),
      mOrder.end(),
      [&result,&name](const OrderMapPair& ref)
      {
        SSIterator ssiter=ref.second.find(name);
        if(ssiter!=ref.second.end())
        {
          result.push_back(ref.first);
        }
      }
    );
    return result;
  }
  
  void startService(const std::string& ref)
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    OMIterator it=mOrder.find(ref);
    if(it!=mOrder.end()) // we found the service
    {
      std::list<std::string> needed(finddeps(it->first));
      std::for_each(
        needed.begin(),needed.end(),
        [this](const std::string& name)
        {
         startService(name);
        }
      );
      startServicePrivate(it->first);
    }
    else
    {
      std::for_each(
        mOrder.begin(),
        mOrder.end(),
        [&ref,this](const OrderMapPair& rref)
        {
          SSIterator ssiter=rref.second.find(ref);
          if(ssiter!=rref.second.end())
          {
            startService(rref.first);
            startServicePrivate(*ssiter);
          }
        }
      );
    }
  }
  
  void stopService(const std::string& name)
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    OMIterator omit=mOrder.find(name);
    if(omit!=mOrder.end()) //recursively stopping dependent services
    {
      std::for_each(
        omit->second.begin(),omit->second.end(),
        [this](const std::string& subservice)
        {
          stopServicePrivate(subservice);
        }
      );
      // now stopping the main service
      stopServicePrivate(name);
    }
    else stopServicePrivate(name);
  }
  
  void stopAll()
  {
    sys::RecursiveSyncLock synchronize(mMutex);
    std::for_each(
       mServiceRegistry.begin(),mServiceRegistry.end(),
       [this](const ServicePair& ref)
       {
        stopService(ref.first);
       }
    );
  }
  
  void showservices()
  {
   sys::RecursiveSyncLock synchronize(mMutex);
   itc::getLog()->info("The ollowing services are registered:");
    std::for_each(
       mServiceRegistry.begin(),mServiceRegistry.end(),
       [this](const ServicePair& ref)
       {
        itc::getLog()->info("-> %s with addr at %jx",ref.first.c_str(),ref.second.get());
       }
    );
  }
  void showdeps()
  {
   sys::RecursiveSyncLock synchronize(mMutex);
   std::for_each
   (
    mOrder.begin(),mOrder.end(), 
    [](const OrderMapPair& ref)
    {
     itc::getLog()->info("Service: %s , is required to be started befor following servives:",ref.first.c_str());
     std::for_each
     (
        ref.second.begin(),
        ref.second.end(),
        [](const std::string& rref)
        {
         itc::getLog()->info("--> %s",rref.c_str());
        }
     );
    }
   );
  }
  
  ~ServiceManager()
  {
    destroyAll();
  }
 private:
    sys::RecursiveMutex  mMutex;
    ServiceRegistryMap mServiceRegistry;
    OrderMap mOrder;

    std::list<std::string> getdeplist(const std::string& name)
    {
      OMIterator it=mOrder.find(name);
      std::list<std::string> result;
      if(it!=mOrder.end()) // is a first level service
      {
        std::for_each(
          it->second.begin(),
          it->second.end(),
          [this,&result](const std::string& nm)
          {
            result.push_back(nm);
            getdeplist(nm);
          }
        );
      }
      return result;
    }
    
    void stopServicePrivate(const std::string& name)
    {
      SRMIterator srmit=mServiceRegistry.find(name);
      if(srmit!=mServiceRegistry.end())
      {
        abstract::Service *ptr=srmit->second.get();
        if((ptr)&&(ptr->isup()))
        {
          itc::getLog()->info("[ServiceManager] shutdown the service %s", ptr->getName().c_str());
          ptr->stop();
          itc::getLog()->info("[ServiceManager] the service %s is down", ptr->getName().c_str());
        }
      }
    }
    void startServicePrivate(const std::string& name)
    {
      SRMIterator srmit=mServiceRegistry.find(name);
      if(srmit!=mServiceRegistry.end())
      {
        abstract::Service *ptr=srmit->second.get();
        if((ptr)&&(ptr->isdown()))
        {
          itc::getLog()->info("[ServiceManager] starting the service %s", ptr->getName().c_str());
          ptr->start();
          itc::getLog()->info("[ServiceManager] the service %s is up", ptr->getName().c_str());
        }
      }
    }
    void destroyService(const std::string& name)
    {
      sys::RecursiveSyncLock synchronize(mMutex);
      
      SRMIterator srmit=mServiceRegistry.find(name);
      if(srmit!=mServiceRegistry.end())
      {
        std::list<std::string> deplist(getdeplist(name));
        std::for_each(
          deplist.begin(),
          deplist.end(),
          [this](const std::string& nm)
          {
           destroyService(nm);
          }
        );
        stopService(name);
        mServiceRegistry.erase(srmit);
      }
    }

    void destroyAll()
    {
      sys::RecursiveSyncLock synchronize(mMutex);
      SRMIterator srmit=mServiceRegistry.begin();
      while(srmit!=mServiceRegistry.end())
      {
        destroyService(srmit->first);
        srmit=mServiceRegistry.begin();
      }
    }
 };
}


#endif	/* SERVICEMANAGER_H */

