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
 * $Id: bz2Decompressor.h 22 2010-11-23 12:53:33Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 **/

#ifndef __BZ2DECOMPRESSOR_H__
#    define	__BZ2DECOMPRESSOR_H__
#    include <abstract/IMessageListener.h>
#    include <ThreadSafeLocalQueue.h>
#    include <CompressBuffer.h>
#    include <ITCException.h>
#    include <TSLog.h>
#    include <InterfaceCheck.h>
#    include <errno.h>
#    include <bzlib.h>
#    include <boost/shared_ptr.hpp>
#    include <sys/SyncLock.h>
#    include <sys/types.h>
#    include <CompCommon.h>
#    include <CompressBuffer.h>

namespace itc
{

class bz2Decompressor : public IMLType
{
public:

    explicit bz2Decompressor(QueueSharedPtrType& pInQueue, QueueSharedPtrType& pOutQueue)
    : IMLType(pInQueue), mOutQueue(pOutQueue)
    {
        sys::SyncLock synchronize(mMutex);
        if (mOutQueue.get() == NULL)
        {
            throw NullPointerException(EFAULT);
        }
    }

    ~bz2Decompressor()
    {
        sys::SyncLock synchronize(mMutex);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::~bz2Decompressor()");
    }

    void onCancel()
    {
        sys::SyncLock synchronize(mMutex);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::onCancel()");
    }

protected:

    void onMessage(CompressBuffer<char>& msg)
    {
        register int ret;
        sys::SyncLock synchronize(mMutex);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::onMessage()");
        CompressBuffer<char> out(msg.mUCSize);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::onMessage(), prepared out buffer: %d, size: %u , UCSize: %u", out.mBuffer.get(), out.mUsed,out.mUCSize);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::onMessage(), prepared out buffer: %d, size: %u , UCSize: %u", msg.mBuffer.get(), msg.mUsed,msg.mUCSize);
        ret = BZ2_bzBuffToBuffDecompress(out.mBuffer.get(), &(out.mUsed), msg.mBuffer.get(), msg.mUsed, 0, 0);

        switch (ret)
        {
            case BZ_CONFIG_ERROR:
                throw BZ2ConfigErrorException(EINVAL);
                break;
            case BZ_PARAM_ERROR:
                throw BZ2ParamErrorException(E2BIG);
                break;
            case BZ_MEM_ERROR:
                throw BZ2MemErrorException(ENOMEM);
                break;
            case BZ_OUTBUFF_FULL:
                throw BZ2MemErrorException(ENOBUFS);
                break;
            case BZ_DATA_ERROR:
            case BZ_DATA_ERROR_MAGIC:
            case BZ_UNEXPECTED_EOF:
                throw BZ2BadDataException(EPROTO);
                break;
        }
        if (ret == BZ_OK)
            out.mID=msg.mID;
            this->mOutQueue.get()->send(out);
    }

    void onQueueDestroy()
    {
        sys::SyncLock synchronize(mMutex);
        getLog()->debug(__FILE__, __LINE__, "bz2Decompressor::onQueueDestroy()");
    }


private:
    sys::Mutex mMutex;
    QueueSharedPtrType mOutQueue;
};
}
#endif	/* __BZ2DECOMPRESSOR_H__ */

