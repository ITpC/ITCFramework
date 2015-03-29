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
#  include <LMDBEnv.h>
#  include <LMDB.h>
#  include <LMDBException.h>
#  include <TSLog.h>
#  include <stdint.h>

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
      std::shared_ptr<Database> mDB;

     public:

      class Cursor
      {
       private:
        MDB_cursor* cursor;
        MDB_dbi dbi;
        MDB_txn *handle;

       public:

        explicit Cursor(const MDB_dbi& pdbi, MDB_txn *phandle)
          : dbi(pdbi), handle(phandle)
        {
          int ret = mdb_cursor_open(handle, dbi, &cursor);
          if(ret)
          {
            if(ret == EINVAL)
            {
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
            }else
            {
              itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROCursor::ROCursor() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
              throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
            }
          }
        }

        explicit Cursor(const Cursor& ref) = delete;

        template <typename T> const uint64_t getFirst(T& data)
        {
          return get<T>(data, MDB_FIRST);
        }

        template <typename T> const uint64_t getLast(T& data)
        {
          return get<T>(data, MDB_LAST);
        }

        template <typename T> const uint64_t getCurrent(T& data)
        {
          return get<T>(data, MDB_GET_CURRENT);
        }

        template <typename T> const uint64_t getNext(T& data)
        {
          return get<T>(data, MDB_NEXT);
        }

        template <typename T> const uint64_t getPrev(T& data)
        {
          return get<T>(data, MDB_PREV);
        }

        template <typename T> const uint64_t find(const uint64_t& pkey, T& data)
        {
          MDB_val key , dbdata;
          key.mv_data=(void*)(&pkey);
          key.mv_size=sizeof(uint64_t);
          int ret = mdb_cursor_get(cursor, &key, &dbdata, MDB_SET_KEY);
          switch(ret){
            case 0:
            {
              memcpy(&data, dbdata.mv_data, sizeof(data));
              mdb_cursor_close(cursor);
              return pkey;
            }
            case MDB_NOTFOUND:
              throw TITCException<exceptions::MDBKeyNotFound>(exceptions::MDBGeneral);
            case EINVAL:
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
          }
          itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::Cursor::find() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
          throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
        }

        ~Cursor()
        {
          mdb_cursor_close(cursor);
        }
       private:

        template <typename T> const uint64_t get(T& data, const MDB_cursor_op& op)
        {
          MDB_val key, dbdata;
          int ret = mdb_cursor_get(cursor, &key, &dbdata, op);
          switch(ret){
            case 0:
            {
              memcpy(&data, dbdata.mv_data, sizeof(data));
              uint64_t reskey = 0;
              memcpy(&reskey, key.mv_data, key.mv_size);
              return reskey;
            }
            case MDB_NOTFOUND:
              throw TITCException<exceptions::MDBKeyNotFound>(exceptions::MDBGeneral);
            case EINVAL:
              throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam);
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
      explicit ROTxn(const std::shared_ptr<Database>& ref)
        : mDB(ref)
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
      explicit ROTxn(const std::shared_ptr<Database>& ref, const ROTxn& parent)
        : mDB(ref)
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
       * @param key - the key
       * @param result - storage for the value
       * 
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBClosed)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalParam)
       * @exception TITCException<exceptions::MDBGeneral>(exceptions::InvalidException)
       **/
      template <typename T> bool get(const uint64_t& key, T& result)
      {
        MDB_val data;
        MDB_val dbkey;

        dbkey.mv_size = sizeof(key);
        dbkey.mv_data = (void*) (&key);

        int ret = mdb_get(handle, mDB->dbi, &dbkey, &data);

        switch(ret){
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
            memcpy(((void*) &result), data.mv_data, data.mv_size);
            return true;
          default:
            itc::getLog()->fatal(__FILE__, __LINE__, "[666]:ROTxn::get() something is generally wrong. This message should never appear in the log. Seems that the LMDB API has been changed or extended with new error codes. Please file a bug-report");
            throw TITCException<exceptions::MDBGeneral>(exceptions::InvalidException);
        }
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
