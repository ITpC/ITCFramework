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
 * $Id: PQMessageListener.h 1 2015-03-20 10:53:11Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __PQMESSAGELISTENER_H__
#  define __PQMESSAGELISTENER_H__

#  include <memory>

#  include <sys/Thread.h>
#  include <sys/AtomicBool.h>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <abstract/Runnable.h>
#  include <Exceptions.h>
#  include <TSLog.h>
#  include <abstract/QueueInterface.h>
#  include <QueueTxn.h>

namespace itc
{

  template <
  typename DataType,
  template <class> class QueueImpl
  > class PQMessageListener : public itc::abstract::IRunnable
  {
   public:
    typedef QueueImpl<DataType> TQueueImpl;
    typedef typename std::shared_ptr<TQueueImpl> QueueSharedPtr;
    typedef typename std::weak_ptr<TQueueImpl> QueueWeakPtr;
    typedef QueueTxn<DataType>  QueueTxnType;
    typedef std::shared_ptr<QueueTxnType> QueueTxnSPtr;


   private:
    sys::AtomicBool mayRun;
    QueueWeakPtr mQueue;

   public:

    explicit PQMessageListener(QueueSharedPtr& pQueue)
      : mayRun(false), mQueue(pQueue)
    {
      if(!(mQueue.lock().get()))
        throw NullPointerException(EFAULT);
      mayRun = true;
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::PQMessageListener(QueueSharedPtr& pQueue)", this);
    }

    void shutdown()
    {
      mayRun = false;
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::shutdown()", this);
    }

    inline QueueWeakPtr getQueueWeakPtr()
    {
      return mQueue;
    }

    void execute()
    {
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() has been started", this);
      while(mayRun)
      {
        if(TQueueImpl * queue_addr = mQueue.lock().get())
        {
          try {
            onMessage(queue_addr->recv());
          }catch(std::exception& e)
          {
            mayRun = false;
            itc::getLog()->error(__FILE__, __LINE__, "Listener at address %x, has cought an exception on queue recieve and following message processing: %s", this, e.what());
          }
        }else
        {
          mayRun = false;
          itc::getLog()->error(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() - QueueWeakPtr does not exists anymore, calling oQueueDestroy", this);
        }
      }
      itc::getLog()->error(__FILE__, __LINE__, "%s at address %x", "PQMessageListener::execute() has been finished", this);
    }

   protected:
    virtual void onMessage(const QueueTxnSPtr&) = 0;
    virtual void onQueueDestroy() = 0;

    virtual ~PQMessageListener()
    {
      shutdown();
      itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "~PQMessageListener()", this);
    }
  };
}

#endif /*__PQMESSAGELISTENER_H__*/
