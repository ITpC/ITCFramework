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
 * $Id: ThreadSafeLocalQueue.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __THREADSAFELOCALQUEUE_H__
#define __THREADSAFELOCALQUEUE_H__

#include <time.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <queue>

#include <abstract/QueueInterface.h>
#include <sys/Semaphore.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <TSLog.h>
#include <Val2Type.h>
#include <abstract/Cleanable.h>
#include <ITCException.h>
#include <sys/AtomicDigital.h>


namespace itc {

    template <
    typename DataType
    > class ThreadSafeLocalQueue : public QueueInterface<DataType> {
    private:
        sys::Mutex mMutex;
        sys::Semaphore mMsgTrigger;
        std::queue<DataType> mQueue;

    public:

        explicit ThreadSafeLocalQueue()
        : QueueInterface<DataType>(),
        mMutex(),
        mMsgTrigger() {
        }

        inline bool send(const DataType& pData) {
            sys::SyncLock sync(mMutex);
            mQueue.push(pData);
            mMsgTrigger.post();
            return true;
        }

        inline bool recv(DataType& pData) {
            mMsgTrigger.wait();
            sys::SyncLock synchronize(mMutex);
            if (!mQueue.empty()) {
                pData = mQueue.front();
                mQueue.pop();
                return true;
            }
            return false;
        }

        inline size_t depth() {
            sys::SyncLock synchronize(mMutex);
            return mQueue.size();
        }

        inline void destroy() {
            mMsgTrigger.destroy();
        }

        ~ThreadSafeLocalQueue() {
            destroy();
        }
    };
}

#endif /*THREADSAFELOCALQUEUE_H_*/
