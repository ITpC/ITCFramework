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
 * $Id: ServiceFacade.h 22 2015-03-13 10:53:00Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 **/

#ifndef SERVICEFACADE_H
#  define	SERVICEFACADE_H

#include <abstract/Service.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <TSLog.h>

namespace itc
{
 /**
  * @brif The itc::ServiceFacade have to be inherited by classes those are 
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
    sys::Mutex  mMutex;
    bool        mUp;
    std::string mName;
 public:
    explicit ServiceFacade(const std::string& name)
    : mMutex(),mUp(false), mName(name)
    {
      sys::SyncLock synchronize(mMutex);
      itc::getLog()->info("Service %s is created",mName.c_str());
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
    
    const std::string& getName() const
    {
      return mName;
    }
    
    void stop()
    {
      sys::SyncLock synchronize(mMutex);
      onStop();
      this->mUp=false;
    }

    void start()
    {
      sys::SyncLock synchronize(mMutex);
      onStart();
      this->mUp=true;
    }

    virtual void onStart()=0;
    virtual void onStop()=0;
    virtual void onDestroy()=0;    

 
    ~ServiceFacade()
    {
     sys::SyncLock synchronize(mMutex);
     itc::getLog()->info("Service %s is destroyed",mName.c_str());
    }
 };
}

#endif	/* SERVICEFACADE_H */

