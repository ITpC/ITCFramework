/* 
 * File:   LMDBException.h
 * Author: pk
 *
 * Created on 18 Март 2015 г., 13:44
 */

#ifndef LMDBEXCEPTION_H
#  define	LMDBEXCEPTION_H

#  include <lmdb.h>
#  include <errno.h>
#  include <ITCException.h>

namespace itc
{

  class LMDBExceptionParser
  {
   public:
    LMDBExceptionParser(int ret)
    {
      switch(ret){
        case MDB_VERSION_MISMATCH:
          throw TITCException<exceptions::MDBVersionMissmatch>(exceptions::MDBGeneral);
          ;
        case MDB_INVALID:
          throw TITCException<exceptions::MDBInvalid>(exceptions::MDBGeneral);
          ;
        case ENOENT:
          throw TITCException<exceptions::MDBNotFound>(exceptions::MDBGeneral);
          ;
        case EACCES:
          throw TITCException<exceptions::MDBEAccess>(exceptions::MDBGeneral);
          ;
        case EAGAIN:
          throw TITCException<exceptions::MDBEAgain>(exceptions::MDBGeneral);
          ;
        case MDB_PANIC:
          throw TITCException<exceptions::MDBPanic>(errno);
          ;
        case MDB_MAP_RESIZED:
          throw TITCException<exceptions::MDBMapResized>(exceptions::MDBGeneral);
          ;
        case MDB_READERS_FULL:
          throw TITCException<exceptions::MDBReadersFull>(exceptions::MDBGeneral);
          ;
        case ENOMEM:
          throw TITCException<exceptions::MDBGeneral>(ENOMEM);
          ;
        case MDB_NOTFOUND:
          throw TITCException<exceptions::MDBNotFound>(exceptions::MDBGeneral);
          ;
        case MDB_DBS_FULL:
          throw TITCException<exceptions::MDBTooMany>(exceptions::MDBGeneral);
          ;
        case EINVAL:
          throw TITCException<exceptions::MDBEInval>(exceptions::MDBGeneral);
        default:
        {
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
      }
    }
  };
}

#endif	/* LMDBEXCEPTION_H */

