/**
 * Copyright (c) 2003-2011, Pavel Kraynyukhov.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * $Id: CompressBuffer.h 22 2010-11-23 12:53:33Z pk $
 * $Author: pk$
 * File:   CompressBuffer.h
 *
 **/

#ifndef __COMPRESSBUFFER_H__
#    define	__COMPRESSBUFFER_H__
#    include <SharedBuffer.h>

namespace itc
{

template <typename T> struct CompressBuffer
{
    u_int64_t mID;
    u_int32_t mUsed;
    u_int32_t mUCSize;
    SharedBuffer<T> mBuffer;

    explicit CompressBuffer(const size_t pSize = 0) : mID(0), mUsed(pSize), mUCSize(pSize), mBuffer(pSize)
    {
    }
    
    CompressBuffer(const SharedBuffer<T>& ref) : mID(0), mUsed(ref.size()), mUCSize(ref.size()), mBuffer(ref)
    {
    }

    CompressBuffer(const CompressBuffer<T>& ref) : mID(ref.mID), mUsed(ref.mUsed), mUCSize(ref.mUCSize), mBuffer(ref.mBuffer)
    {
    }
    
    inline const size_t size()
    {
        return mBuffer.size();
    }
    
    inline const CompressBuffer & operator=(const CompressBuffer & ref)
    {
        mID = ref.mID;
        mUsed = ref.mUsed;
        mUCSize = ref.mUCSize;
        mBuffer = ref.mBuffer;
        return (*this);
    }
};

}
#endif	/* __COMPRESSBUFFER_H__ */

