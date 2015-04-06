/**
 * Copyright (c) 2009-2015, Pavel Kraynyukhov.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * $Id: LMDBROTxn.h 1 2015-03-21 00:51:11Z pk $
 *
 **/


#ifndef LMDBROTXN_H
#  define	LMDBROTXN_H
#  include <memory>
#  include <LMDB.h>
#  include <LMDBEnv.h>
#  include <LMDBException.h>
#  include <TSLog.h>
#  include <stdint.h>
<<<<<<< HEAD
#  include <DBKey.h>
#  include <mutex>
#  include <sys/synclock.h>
#  include <PQueueDataType.h>
#  include <SyncWriter.h>
=======
#  include <DBKeyType.h>
#  include <QueueObject.h>
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378

namespace itc
{
  namespace lmdb
  {

    /**
     * @brief Read-only transaction object. Use the instances of this class on 
     * stack. Never share this object between the threads. Every thread must 
     * allocate those objects in its own space and never share the objects with 
     * others. The transaction begins with instantiation of the object and lasts
     * until the object is destroyed. 
     **/
    class ROTxn
    {
     private:
      MDB_txn* handle;
      std::shared_ptr<::itc::lmdb::Database> mDB;

     public:

      class Cursor
      {
       private:
        MDB_cursor* cursor;
        MDB_dbi dbi;
        MDB_txn *handle;

       public:

        explicit Cursor(const MDB_dbi& pdbi, MDB_txn *phandle)
          :dbi(pdbi), handle(phandle)
        {
          int ret = mdb_cursor_open(handle, dbi, &cursor);
          if(ret)
          {
            if(ret == EINVAL)
            {
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            }
            else
            {
              ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROCursor::ROCursor() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
              throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
            }
          }
        }

        explicit Cursor(const Cursor& ref) = delete;

<<<<<<< HEAD
        const bool getFirst(DBKey& key, PQueueDataType& data)
        {
          return get(key, data, MDB_FIRST);
        }

        const bool getLast(DBKey& key, PQueueDataType& data)
        {
          return get(key, data, MDB_LAST);
        }

        const bool getCurrent(DBKey& key, PQueueDataType& data)
        {
          return get(key, data, MDB_GET_CURRENT);
        }

        const bool getNext(DBKey& key, PQueueDataType& data)
        {
          return get(key, data, MDB_NEXT);
        }

        const bool getPrev(DBKey& key, PQueueDataType& data)
        {
          return get(key, data, MDB_PREV);
        }

        const bool find(const DBKey& pkey, PQueueDataType& data)
        {
          int ret = 0;
          SyncWriterLock swl;
          
          swl.use(pkey,
            [&ret, &data, this](const uint64_t& l, const uint64_t& r)
            {
              uint64_t u64a[2];
              MDB_val dbdata;
              MDB_val key;
              u64a[0]=l;
              u64a[1]=r;
              key.mv_data=u64a;
              key.mv_size=sizeof(u64a);
              ret = mdb_cursor_get(cursor, &key, &dbdata, MDB_SET_KEY);
              PQueueDataType tmp(dbdata.mv_size, (uint8_t*)dbdata.mv_data);
              data=tmp;
=======
        QueueMessageSPtr getFirst()
        {
          return get(MDB_FIRST);
        }

        QueueMessageSPtr getLast()
        {
          return get(MDB_LAST);
        }

        QueueMessageSPtr getCurrent()
        {
          return get(MDB_GET_CURRENT);
        }

        QueueMessageSPtr getNext()
        {
          return get(MDB_NEXT);
        }

        QueueMessageSPtr getPrev()
        {
          return get(MDB_PREV);
        }

        QueueMessageSPtr find(const DBKey& pkey)
        {
          MDB_val key, dbdata;
          key.mv_data = (void*)(&pkey);
          key.mv_size = sizeof(DBKey);
          int ret = mdb_cursor_get(cursor, &key, &dbdata, MDB_SET_KEY);
          switch(ret)
          {
            case 0:
            {
              uint8_t_sarray tmparray(new uint8_t[dbdata.mv_size]);
              memcpy(tmparray.get(), dbdata.mv_data, dbdata.mv_size);
              mdb_cursor_close(cursor);
              return QueueMessageSPtr(std::make_shared<QueueObject>(pkey, dbdata.mv_size, tmparray));
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
            }
          );
          
          
          if(ret == 0)
          {  
            return true;
          }
          switch(ret)
          {
            case MDB_NOTFOUND:
              throw TITCException<exceptions::MDBKeyNotFound>(exceptions::MDBGeneral);
            case EINVAL:
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          }
          ::itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::Cursor::find(), - something is generally wrong. This message should never appear in the log.");
          ::itc::getLog()->fatal(__FILE__, __LINE__, "Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
          throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
        }

        ~Cursor()
        {
          mdb_cursor_close(cursor);
        }
       private:

<<<<<<< HEAD
        bool get(DBKey& _key, PQueueDataType& _data, const MDB_cursor_op& op)
=======
        const QueueMessageSPtr get(const MDB_cursor_op& op)
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
        {
          SyncWriterLock swl;
          MDB_val key, dbdata;
          int ret = mdb_cursor_get(cursor, &key, &dbdata, op);
<<<<<<< HEAD
          if(ret == 0)
          {
            DBKey newkey(((uint64_t*)(key.mv_data))[0],((uint64_t*)(key.mv_data))[1]);
            _key=newkey;
            PQueueDataType tmp(dbdata.mv_size, (const uint8_t*)dbdata.mv_data);
            swl.assign(tmp, _data);
            return true;            
          }
          else
          {
            switch(ret)
            {
              case MDB_NOTFOUND:
                throw TITCException<exceptions::MDBKeyNotFound>(exceptions::MDBGeneral);
              case EINVAL:
                throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
=======
          switch(ret)
          {
            case 0:
            {
              uint8_t_sarray tmparray(new uint8_t[dbdata.mv_size]);
              memcpy(tmparray.get(), dbdata.mv_data, dbdata.mv_size);
              DBKey dbkey;
              memcpy(&dbkey, key.mv_data, key.mv_size);

              return QueueMessageSPtr(std::make_shared<QueueObject>(dbkey, dbdata.mv_size, tmparray));
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
            }
          }
          itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::Cursor::get() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
          throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
        }
      };

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       **/
<<<<<<< HEAD
      explicit ROTxn(const std::shared_ptr<Database>& ref)
=======
      explicit ROTxn(const std::shared_ptr<::itc::lmdb::Database>& ref)
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
        :mDB(ref)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginROTxn();
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief constructor
       * 
       * @param ref - reference to std::shared_ptr of the itc::lmdb::Database object
       * @param parent - parent transaction
       **/
<<<<<<< HEAD
      explicit ROTxn(const std::shared_ptr<Database>& ref, const ROTxn& parent)
=======
      explicit ROTxn(const std::shared_ptr<::itc::lmdb::Database>& ref, const ROTxn& parent)
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
        :mDB(ref)
      {
        try
        {
          handle = (mDB.get()->mEnvironment.get())->beginROTxn(parent.handle);
        }catch(std::exception& e)
        {
          itc::getLog()->error(__FILE__, __LINE__, "Transaction may not begin, reason: %s\n", e.what());
          throw TITCException<exceptions::ITCGeneral>(exceptions::Can_not_begin_txn);
        }
      }

      /**
       * @brief forbid the copy constructor
       * 
       * @param ref - treference
       **/
      ROTxn(const ROTxn& ref) = delete;

      /**
       * @brief get the value from the database related to provided key
       * 
       * @param key - the DBKey
       * @return std::shared_ptr<QueueObject>
       * 
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBClosed)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::InvalidException)
       **/
<<<<<<< HEAD
      const bool get(const DBKey& key, PQueueDataType& result)
      {
        Cursor cursor(mDB.get()->dbi, handle);
        bool ret=false;
        try
        {
          ret=cursor.find(key,result);
        }catch(const TITCException<exceptions::MDBKeyNotFound>& e)
        {
          return false;
=======
      QueueMessageSPtr get(const DBKey& key)
      {
        MDB_val data;
        MDB_val dbkey;

        dbkey.mv_size = sizeof(DBKey);
        dbkey.mv_data = (void*)(&key);

        int ret = mdb_get(handle, mDB->dbi, &dbkey, &data);

        switch(ret)
        {
          case MDB_NOTFOUND:
          {
            return false;
          }
          case EINVAL:
          {
            itc::getLog()->error(__FILE__, __LINE__, "ROTxn::get() either the key is invalid or something is wrong with source code, that is handling this method");
            throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          }
          case 0:
          {
            uint8_t_sarray tmparray(new uint8_t[data.mv_size]);
            memcpy(tmparray.get(), data.mv_data, data.mv_size);
            return QueueMessageSPtr(std::make_shared<QueueObject>(key, data.mv_size, tmparray));
          }
          default:
            itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::get() something is generally wrong. This message should never appear in the log.");
            itc::getLog()->fatal(__FILE__, __LINE__, "Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
            throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
>>>>>>> 0a9ee3776957ef7945cd9935805ab6efe3144378
        }
        return ret;
      }

      std::shared_ptr<Cursor> getCursor()
      {
        return std::make_shared<Cursor>(mDB.get()->dbi, handle);
      }

      /**
       * @brief commit transaction/reset
       * 
       **/
      ~ROTxn()
      {
        (mDB.get()->mEnvironment.get())->ROTxnCommit(handle);
      }
    };
  }
}

#endif	/* LMDBROTXN_H */

