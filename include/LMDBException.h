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
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBVersionMissmatch);
          ;
        case MDB_INVALID:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBInvalid);
          ;
        case ENOENT:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBNotFound);
          ;
        case EACCES:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEAccess);
          ;
        case EAGAIN:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEAgain);
          ;
        case MDB_PANIC:
          throw TITCException<exceptions::MDBPanic>(errno);
          ;
        case MDB_MAP_RESIZED:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBMapResized);
          ;
        case MDB_READERS_FULL:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBReadersFull);
          ;
        case ENOMEM:
          throw TITCException<exceptions::MDBGeneral>(ENOMEM);
          ;
        case MDB_NOTFOUND:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBNotFound);
          ;
        case MDB_DBS_FULL:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBTooMany);
          ;
        case EINVAL:
          throw TITCException<exceptions::MDBGeneral>(exceptions::MDBEInval);
        default:
        {
          throw TITCException<exceptions::ExternalLibraryException>(errno);
        }
      }
    }
  };
}

#endif	/* LMDBEXCEPTION_H */

