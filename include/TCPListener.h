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
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <cstdint>

#include <net/NSocket.h>
#include <sys/synclock.h>
#include <TCPSocketDef.h>
#include <abstract/IController.h>
#include <abstract/Runnable.h>

namespace itc
{

  class TCPListener: public ::itc::abstract::IRunnable, public ::itc::abstract::IController<CSocketSPtr>
  {
  private:
    std::mutex        mMutex;
    std::string       mAddress;
    int               mPort;
    ServerSocket      mServerSocket;
    ViewTypeSPtr      mSocketsHandler;
    std::atomic<bool> mNotEnd;

	public:
   typedef ModelType value_type;
   
    explicit TCPListener(const std::string& address,const int port,const ViewTypeSPtr& sh)
    : mMutex(), mAddress(address), mPort(port), mServerSocket(mAddress,mPort),
      mSocketsHandler(sh),mNotEnd(true)
    {   
    }
    
    void execute()
    {
      while (mNotEnd)
      {
        SyncLock sync(mMutex);

        value_type newClient(
          itc::Singleton<TCPSocketsFactory>::getInstance<
            size_t,size_t
          >(100,1000)->getBlindSocket()
        );

        if(int ret=mServerSocket.accept(newClient) == -1)
        {
          throw TITCException<exceptions::InvalidSocketException>(ret);
        }
        else
        {
          std::string peeraddr;
          newClient.get()->getpeeraddr(peeraddr);
          itc::getLog()->info("Inbound connection from %s", peeraddr.c_str());
          notify(newClient,mSocketsHandler);
        }
      }
    }
    void onCancel()
    {
      SyncLock sync(mMutex);
      mNotEnd=false;
      mServerSocket.close();
    }
    void shutdown()
    {
      SyncLock sync(mMutex);
      mNotEnd=false;
      mServerSocket.close();
    }
  };
}


#endif	/* __TCPLISTENER_H__ */

