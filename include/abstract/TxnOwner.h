/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: TxnOwner.h 27 Март 2015 г. 23:29 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __TXNOWNER_H__
#  define	__TXNOWNER_H__
#  include <stdint.h>
#  include <memory>
#  include <abstract/IQTxn.h>

namespace itc
{
  namespace abstract
  {
    typedef std::shared_ptr<abstract::IQTxn> IQTxnSPtr;
    class TxnOwner
    {
     public:
      explicit TxnOwner() = default;
      virtual void TxnAborted(const uint64_t&, const IQTxnSPtr& ref) = 0;
      ~TxnOwner() = default;
    };
  }
  typedef std::shared_ptr<abstract::TxnOwner> TxnOwnerSPtr;
}


#endif	/* __TXNOWNER_H__ */

