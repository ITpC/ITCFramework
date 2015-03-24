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
 * $Id: PersistentQueue.h 1 2015-03-21 20:32:15Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __PERSISTENTQUEUE_H__
#  define __PERSISTENTQUEUE_H__

#  include <time.h>
#  include <errno.h>
#  include <string.h>
#  include <sched.h>
#  include <queue>

#  include <abstract/QueueInterface.h>
#  include <sys/Semaphore.h>
#  include <sys/Mutex.h>
#  include <sys/SyncLock.h>
#  include <TSLog.h>
#  include <Val2Type.h>
#  include <abstract/Cleanable.h>
#  include <ITCException.h>
#  include <sys/AtomicDigital.h>
#  include <LMDBWriter.h>
#  include <LMDBWObject.h>
#  include <sys/AtomicBool.h>
#  include <LMDBROTxn.h>
#  include <LMDBWObject.h>
#  include <stdint.h>
#  include <QueueTxn.h>

namespace itc
{

  /**
   * @brief a parsistent queue with LMDB backend. Due to nature of LMDB there
   * is only one writer thread is allowed, so this queue must interract with
   * a writer thread (itc::lmdb::DBWriter). Writing to this is as fast as the 
   * LMDB is allowing, reading is asynchronous. However after you got the 
   * message and processed it in any way you require, you have to call
   * PersistentQueue<T>::commit(key) method, to remove the message from database.
   * consider this call as a transaction commit. If message recieving is not
   * followed by PersistentQueue<T>::commit(key) call at any later time, this 
   * message will persist in a database, and will be loaded again on 
   * PersistentQueue start.
   * 
   **/
  template <
  typename DataType
  > class PersistentQueue : public QueueInterface<DataType>
  {
   public:
    typedef std::shared_ptr<lmdb::ROTxn::Cursor> CursorSPtr;
    typedef std::shared_ptr<itc::lmdb::WObject> WObjectSPtr;
    typedef QueueTxn<DataType> QueueTxnType;
    typedef std::shared_ptr<QueueTxnType> QueueTxnSPtr;
    typedef std::shared_ptr<DataType> Storable;

   private:
    typedef lmdb::SyncDBWriterAdapter SyncDBWAdapter;
    typedef std::shared_ptr<lmdb::SyncDBWriterAdapter> SharedSyncDBWAdapter;
    sys::Mutex mMutex;
    sys::Mutex mCommitProtect;
    sys::Semaphore mMsgTrigger;
    std::queue<Storable> mDataQueue;
    std::queue<uint64_t> mKeysQueue;
    lmdb::DBWriterSPtr mDBWriter;
    SharedSyncDBWAdapter mWAdapter;
    sys::AtomicUInt64 mNewKey;
    sys::AtomicBool mayRun;


   public:

    explicit PersistentQueue(const lmdb::DBWriterSPtr& ref)
      : QueueInterface<DataType>(), mMutex(), mCommitProtect(), mMsgTrigger(),
      mDBWriter(ref), mWAdapter(std::make_shared<SyncDBWAdapter>(ref)), mayRun(true)
    {
      sys::SyncLock dosync(mMutex);
      load();
    }

    bool send(const DataType& pData)
    {
      if(mayRun)
      {
        sys::SyncLock sync(mMutex);

        lmdb::WObjectSPtr tmp(std::make_shared<itc::lmdb::WObject>());
        tmp->data.mv_data = (DataType*) (&pData);
        tmp->data.mv_size = sizeof(pData);
        uint64_t key = ++mNewKey;

        tmp->key.mv_data = &key;
        tmp->key.mv_size = sizeof(uint64_t);

        tmp->theOP = lmdb::ADD;

        mWAdapter->write(tmp);

        mDataQueue.push(std::make_shared<DataType>(pData));
        mKeysQueue.push(mNewKey);
        mMsgTrigger.post();
        return true;
      }
      return false;
    }

    QueueTxnSPtr recv()
    {
      mMsgTrigger.wait();
      if(mayRun)
      {
        sys::SyncLock synchronize(mMutex);
        if(!(mDataQueue.empty() && mKeysQueue.empty()))
        {
          Storable tmpdata(mDataQueue.front());
          uint64_t key=mKeysQueue.front();
          
          mKeysQueue.pop();
          mDataQueue.pop();
          return std::make_shared<QueueTxnType>(mWAdapter,tmpdata,key);
        }
        throw TITCException<exceptions::QueueOutOfSync>(exceptions::ITCGeneral);
      }
      throw TITCException<exceptions::Can_not_lock_mutex>(exceptions::ITCGeneral);
    }

    bool recv(DataType& pData)
    {
      throw TITCException<exceptions::ImplementationForbidden>(exceptions::ITCGeneral);
    }

    uint64_t depth()
    {
      sys::SyncLock synchronize(mMutex);
      return mDataQueue.size();
    }

    void destroy()
    {
      mayRun = false;
      mMsgTrigger.destroy();
    }

    ~PersistentQueue()
    {
      destroy();
    }
   private:

    void load()
    {
      lmdb::ROTxn txn(mDBWriter.get()->getDatabase());
      CursorSPtr cursor(txn.getCursor());
      size_t count = 0;
      try
      {
        DataType edata, cdata;
        cursor->getLast<DataType>(edata);
        size_t current = cursor->getFirst<DataType>(cdata);

        do
        {
          mKeysQueue.push(current);
          mDataQueue.push(std::make_shared<DataType>(cdata));
          mMsgTrigger.post();
          ++count;
        }while((current = cursor->getNext<DataType>(cdata)));
      }catch(TITCException<exceptions::MDBKeyNotFound>& e)
      {
        itc::getLog()->info("PersistentQueue::load(): %ju messages loaded", count);
      }
    }
  };
}

#endif /*THREADSAFELOCALQUEUE_H_*/
