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
	typedef itc::net::Socket<SRV_TCP_ANY_IF,100> ServerSocket;
	typedef itc::net::Socket<CLN_TCP_KA_TND> ClientSocket;
	typedef ClientSocketsFactory<CLN_TCP_KA_TND> TCPSocketsFactory;
	typedef TCPSocketsFactory::SharedClientSocketPtrType SharedCSPtr;
}


#endif	/* TCPSOCKETDEF_H */

