/* 
 * File:   TCPSocketDef.h
 * Author: pk
 *
 * Created on 4 Март 2015 г., 2:14
 */

#ifndef TCPSOCKETDEF_H
#define	TCPSOCKETDEF_H

#include <net/NSocket.h>
#include <ClientSocketsFactory.h>

namespace itc
{
	typedef net::Socket<SRV_TCP_ANY_IF,100> ServerSocket;
	typedef net::Socket<CLN_TCP_KA_TD> ClientSocket;
	typedef ClientSocketsFactory<CLN_TCP_KA_TD> TCPSocketsFactory;
	typedef TCPSocketsFactory::SharedClientSocketPtrType CSocketSPtr;
  typedef TCPSocketsFactory::ClientSocketType  TCPClientSocketType;
}


#endif	/* TCPSOCKETDEF_H */

