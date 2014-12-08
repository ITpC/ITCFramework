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
 * $Id: ClientSocketsFactory.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __CLIENT_SOCKETS_FACTORY_H__
#define __CLIENT_SOCKETS_FACTORY_H__

#include <memory>

#include <InterfaceCheck.h>
#include <net/NSocket.h>

#include <queue>

#define CLSocketMinDefinition (size_t)(CLIENT_SOCKET)

template <size_t SOpts> class ClientSocketsFactory {
public:
    typedef ::itc::net::Socket<SOpts> ClientSocketType;
    typedef ::std::shared_ptr<ClientSocketType> SharedClientSocketPtrType;

    explicit ClientSocketsFactory(size_t maxPrebuild, size_t minQL)
    : mMaxQueueLength(maxPrebuild), mMinQueueLength(minQL)
    {
        static_assert(SOpts < SERVER_SOCKET,"Must be a client socket type");
        static_assert(SOpts > CLN_TCP_KA_TND,"Is not a client socket type");

        for (
            size_t i = 0; i < mMaxQueueLength;
            mPreBuildSockets.push(
                    new ClientSocketType()
            ), i++
        );
    }

    inline SharedClientSocketPtrType getBlindSocket() {
        if (size_t depth = mPreBuildSockets.size() > mMinQueueLength) {
            ClientSocketType* ptr = mPreBuildSockets.front();
            mPreBuildSockets.pop();
            return SharedClientSocketPtrType(ptr);
        } else {
            for (
                size_t i = depth; i < mMaxQueueLength;
                    mPreBuildSockets.push(
                    new ClientSocketType()
                ), i++
            );
            ClientSocketType* ptr = mPreBuildSockets.front();
            mPreBuildSockets.pop();
            SharedClientSocketPtrType ret(ptr);
            return ret;
        }
    }

private:
    std::queue<ClientSocketType*> mPreBuildSockets;
    size_t mMaxQueueLength;
    size_t mMinQueueLength;
};

#endif /*__CLIENT_SOCKETS_FACTORY_H__*/
