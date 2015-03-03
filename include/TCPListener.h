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
 * $Id: TCPListener.h 1 2015-02-28 13:30:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/
#ifndef __TCPLISTENER_H__
#define	__TCPLISTENER_H__
#include <net/NSocket.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <memory>
#include <ClientSocketsFactory.h>

namespace itc
{
	typedef itc::net::Socket<SRV_TCP_ANY_IF,100> ServerSocket;
	typedef itc::net::Socket<CLN_TCP_KA_TND> ClientSocket;
	typedef std::shared_ptr<ClientSocket> ClientSocketPtr;
	typedef ClientSocketsFactory<CLN_TCP_KA_TND> TCPSocketsFactory;
	typedef TCPSocketsFactory::SharedClientSocketPtrType SharedCSPtr;

	class TCPListener: public itc::abstract::IRunnable
	{
	private:
	    ServerSocket mServerSocket;
	    itc::sys::Mutex mMutex;

	public:
	    TCPListener(const std::string& address,const int port)
	    : mServerSocket(address,port)
	    {   
	    }
	    void execute()
	    {
		while (1)
		{
		    itc::sys::SyncLock sync(mMutex);

		    SharedCSPtr newClient(itc::Singleton<TCPSocketsFactory>::getInstance<size_t,size_t>(100,1000)->getBlindSocket());

		    if(int ret=mServerSocket.accept(newClient))
		    {
			throw TITCException<exceptions::InvalidSocketException>(ret);
		    }
		    else
		    {
			 WorkerPtrType aWorker(new worker(newClient));  
			 SharedWorkerPtrType aWorkerThreadPtr(new WorkerThreadType(aWorker));
			 workers.push_back(aWorkerThreadPtr);
		    }
		}
	    }
	    void onCancel()
	    {
		itc::sys::SyncLock sync(mMutex);
		mServerSocket.close();
	    }
	    void shutdown()
	    {
		itc::sys::SyncLock sync(mMutex);
		mServerSocket.close();
	    }
	};
}


#endif	/* __TCPLISTENER_H__ */

