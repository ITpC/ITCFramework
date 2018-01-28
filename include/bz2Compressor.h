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
 * $Id: bz2Compressor.h 22 2010-11-23 12:53:33Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 **/

#ifndef __BZ2COMPRESSOR_H__
#    define	__BZ2COMPRESSOR_H__
#    include <abstract/IMessageListener.h>
#    include <ThreadSafeLocalQueue.h>
#    include <CompressBuffer.h>
#    include <ITCException.h>
#    include <TSLog.h>
#    include <InterfaceCheck.h>
#    include <errno.h>
#    include <bzlib.h>
#    include <memory>
#    include <sys/synclock.h>
#    include <sys/types.h>
#    include <CompCommon.h>
#    include <math.h>

namespace itc
{

class bz2Compressor : public IMLType
{
public:

    explicit bz2Compressor(QueueSharedPtrType& pInQueue, QueueSharedPtrType& pOutQueue)
    : IMLType(pInQueue), mOutQueue(pOutQueue)
    {
        SyncLock synchronize(mMutex);
        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::bz2Compressor()");
        if (mOutQueue.get() == NULL)
        {
            throw NullPointerException(EFAULT);
        }
        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::bz2Compressor() end of constructor");
    }

    ~bz2Compressor()
    {
        SyncLock synchronize(mMutex);
    }

    void onCancel()
    {
        SyncLock synchronize(mMutex);
        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::onCancel()");
    }

protected:

    void onMessage(CompressBuffer<char>& msg)
    {
        register int ret;
        SyncLock synchronize(mMutex);
        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::OnMessage()");
        float bsf = 1.01;
        do
        {
            ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::OnMessage() - incomming message %d - %u", msg.mBuffer.get(), msg.mUsed);
            CompressBuffer<char> out(lroundf(msg.mUsed * bsf) + 600);
            out.mUCSize = msg.mUsed;
            out.mID=msg.mID;
            ret = BZ2_bzBuffToBuffCompress(out.mBuffer.get(), &(out.mUsed), msg.mBuffer.get(), msg.mUsed, 9, 0, 30);
            ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::OnMessage() - compressed message size %u", out.mUsed);
            switch (ret)
            {
                case BZ_CONFIG_ERROR:
                    throw BZ2ConfigErrorException(ENODATA);
                case BZ_PARAM_ERROR:
                    throw BZ2ParamErrorException(ENODATA);
                case BZ_MEM_ERROR:
                    throw BZ2MemErrorException(ENOMEM);
                case BZ_OK:
                    out.mID=msg.mID;
                    mOutQueue.get()->send(out);
                    ::itc::getLog()->debug(__FILE__, __LINE__ - 1, "bz2Compressor::OnMessage(), compressed data have been send to out-queue");
                    break;
                case BZ_OUTBUFF_FULL:
                    ::itc::getLog()->debug(__FILE__, __LINE__ - 13, "bz2Compressor::OnMessage(), compress retry");
                    break;
            }
            bsf += 0.01;
        }
        while (ret == BZ_OUTBUFF_FULL);

        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::OnMessage(), end");
    }

    void onQueueDestroy()
    {
        SyncLock synchronize(mMutex);
        ::itc::getLog()->debug(__FILE__, __LINE__, "bz2Compressor::onQueueDestroy()");
    }

private:
    sys::Mutex mMutex;
    QueueSharedPtrType mOutQueue;
};
}

#endif /* __BZ2COMPRESSOR_H__ */
