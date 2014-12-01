/**
 * Copyright (c) 2007, Pavel Kraynyukhov.
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
 * $Id: IMessageListener.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __IMESSAGELISTENER_H__
#    define __IMESSAGELISTENER_H__

#    include <boost/shared_ptr.hpp>
#    include <boost/weak_ptr.hpp>

#    include <sys/Thread.h>
#    include <sys/AtomicBool.h>
#    include <sys/Mutex.h>
#    include <sys/SyncLock.h>
#    include <abstract/Runnable.h>
#    include <Exceptions.h>
#    include <TSLog.h>
#    include <abstract/QueueInterface.h>

namespace itc
{

template <
    typename DataType,
    template <class> class QueueImpl
> class IMessageListener : public itc::abstract::IRunnable
{
public:
    typedef QueueImpl<DataType> TQueueImpl;
    typedef typename std::shared_ptr<TQueueImpl> QueueSharedPtr;
    typedef typename std::weak_ptr<TQueueImpl> QueueWeakPtr;

private:
    sys::AtomicBool doWork;
    QueueWeakPtr mQueue;

public:

    explicit IMessageListener(QueueSharedPtr& pQueue)
    : doWork(false), mQueue(pQueue)
    {
        if (!(mQueue.lock().get()))
            throw NullPointerException(EFAULT);
        doWork = true;
        itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::IMessageListener(QueueSharedPtr& pQueue)", this);
    }

    void shutdown()
    {
        doWork = false;
        itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::shutdown()", this);
    }

    inline QueueWeakPtr getQueueWeakPtr()
    {
        return mQueue;
    }

    void execute()
    {
        itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::execute() has began", this);
        while (doWork)
        {
            DataType tmp;
            if (TQueueImpl * queue_addr = mQueue.lock().get())
            {
                if ((queue_addr->recv(tmp)))
                {
                    onMessage(tmp);
                }
                else
                {
                    doWork = false;
                    itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::execute() - Queue::recv() has fault, calling oQueueDestroy", this);
                    onQueueDestroy();
                }
            }
            else
            {
                doWork = false;
                itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::execute() - QueueWeakPtr does not exists anymore, calling oQueueDestroy", this);
                onQueueDestroy();
            }
        }
        itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "IMessageListener::execute() has been finished", this);
    }

protected:
    virtual void onMessage(DataType& msg) = 0;
    virtual void onQueueDestroy() = 0;

    virtual ~IMessageListener()
    {
        itc::getLog()->debug(__FILE__, __LINE__, "%s at address %x", "~IMessageListener()", this);
    }
};
}

#endif /*__IMESSAGELISTENER_H__*/
