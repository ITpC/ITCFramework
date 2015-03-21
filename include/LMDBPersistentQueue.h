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
 * $Id: LocalPersistentQueue.h 22 2015-03-21 16:39:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/


#ifndef LMDBPERSISTENTQUEUE_H
#  define	LMDBPERSISTENTQUEUE_H


#  include <time.h>
#  include <errno.h>
#  include <string.h>
#  include <sched.h>
#  include <queue>
#  include <memory>

#  include <abstract/QueueInterface.h>
#  include <sys/Semaphore.h>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <TSLog.h>
#  include <Val2Type.h>
#  include <abstract/Cleanable.h>
#  include <ITCException.h>
#  include <sys/AtomicDigital.h>
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBROTxn.h>
#  include <LMDBWOTxn.h>

namespace itc
{

  typedef std::shared_ptr<itc::lmdb::Database> LMDBSptr;
  typedef itc::lmdb::ROTxn ROTransaction;
  typedef itc::lmdb::WOTxn WOTransaction;
  typedef itc::lmdb::ROTxn::Cursor Cursor;

  template <typename DataType> class LMDBPersistentQueue : public QueueInterface<DataType>
  {
   private:
    sys::Mutex mMutex;
    sys::Mutex mFrontPopProtect;
    sys::Semaphore mMsgTrigger;
    std::queue<size_t> mQueueKeys;
    std::queue<size_t> mQueueData;

    LMDBSptr mDBInstance;

   public:

    class QERmTxn
    {
     private:
      size_t mKey;
      WOTransaction mWOTxn;
      bool is_not_aborted;
     public:

      DataType anObject;

      explicit QERmTxn(const LMDBSptr& instance, const size_t& key, const DataType& ref)
        : mKey(key), mWOTxn(instance), is_not_aborted(true), anObject(ref)
      {
      }
      
      void abort()
      {
        is_not_aborted=false;
        mWOTxn.
      }
      
      ~QERmTxn()
      {
        if(is_not_aborted)
        {
          mWOTxn.del(mKey);
        }
      }
    };

    explicit LMDBPersistentQueue(const LMDBSptr& instance)
      : QueueInterface<DataType>(),
      mMutex(), mMsgTrigger(),
      mDBInstance(instance)
    {
      sys::SyncLock sync(mMutex);
      ROTransaction readTxn(mDBInstance);
      Cursor cursor(readTxn.getCursor());
      DataType bdata;
      DataType edata;
      try
      {
        size_t end = cursor.getFirst<DataType>(edata);
        size_t begin = cursor.getFirst<DataType>(bdata);
        size_t aniterator = begin;
        while(aniterator != end)
        {
          mQueueKeys.push(aniterator);
          mQueueData.push(bdata);
          mMsgTrigger.post();
          aniterator = cursor.getNext(bdata);
        }
        mQueueKeys.push(aniterator);
        mQueueData.push(bdata);
        mMsgTrigger.post();
      }catch(TITCException<exceptions::MDBKeyNotFound>& e)
      {
      }
    }

    bool send(const DataType& pData)
    {
      sys::SyncLock sync(mMutex);
      size_t key = std::hash<DataType>(pData);
      auto dbinsert = [&pData, &key, this](){
        itc::lmdb::WOTxn writeTxn(mDBInstance);
        writeTxn.put<DataType>(key, pData);
      }
      dbinsert();
      mQueueKeys.push(key);
      mQueueData.push(pData);
      mMsgTrigger.post();
      return true;
    }

    std::shared_ptr<QERmTxn>& pop()
    {
      mMsgTrigger.wait();
      sys::SyncLock synchronize(mMutex);
      if(!mQueueKeys.empty())
      {
        std::shared_ptr<QERmTxn> tmpobj = std::make_shared<QERmTxn>(
          mDBInstance, mQueueKeys.front(), mQueueData.front()
        );
        return tmpobj;
      }
    }

    bool recv(DataType& pData)
    {
      // may not be used with IMessageListener
      throw TITCException<exceptions::ImplementationForbidden>(exceptions::ITCGeneral);
    }

    size_t depth()
    {
      sys::SyncLock synchronize(mMutex);
      return mQueueKeys.size();
    }

    void destroy()
    {
      mMsgTrigger.destroy();
    }

    ~LMDBPersistentQueue()
    {
      destroy();
    }
  };
}


#endif	/* LMDBPERSISTENTQUEUE_H */

