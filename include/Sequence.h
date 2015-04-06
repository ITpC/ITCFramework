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
 * $Id: Sequence.h 22 2010-11-23 12:53:33Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 **/

#ifndef SEQUENCE_H_
#  define SEQUENCE_H_

#  include <compat_types.h>
#  include <Val2Type.h>
#  include <unistd.h>
#  include <atomic>
#  include <limits>

namespace itc
{
  namespace sys
  {
    using namespace itc::utils;

    /**
     * This class provides 64-bit wide unsigned sequence. The purpose of this class
     * is to prowide MT-Safe and consistent sequence of numbers. There are two
     * policies: Rotate and Reverse, those are compile time attributes defining the
     * sequence behavior. If Rotate policy is true, then the sequence will start over
     * after reaching the uint64_t maximum value (-1). The Reverse policy dictates that
     * the sequence starts from max uint64_t and decrementing until reaching 0 or rotates
     * after 0 if the Rotate policy is in use.
     * 
     * @exception std::out_of_range will be thrown if Rotate is false and
     * sequence reached its highest value.
     * 
     **/
    template <bool Rotate = true, bool Reverse = false >
    class Sequence
    {
     private:
      std::atomic<uint64_t> mSequence;

      uint64_t getNext(Bool2Type < false > reverse)
      {

        if(mSequence == std::numeric_limits<uint64_t>::max())
        {
          if(Rotate)
          {
            mSequence = 0;
            return mSequence;
          }
          else
          {
            throw std::out_of_range("Sequence is out of range");
          }
        }
        else
        {
          return ++mSequence;
        }
      }

      uint64_t getNext(Bool2Type < true > reverse)
      {

        if(mSequence == 0)
        {
          if(Rotate)
          {
            mSequence = std::numeric_limits<uint64_t>::max();
            return mSequence;
          }
          else
          {
            throw std::out_of_range("Sequence is out of range");
          }
        }
        else
        {
          return --mSequence;
        }
      }

     public:

      explicit Sequence():mSequence(Reverse ? std::numeric_limits<uint64_t>::max() : 0)
      {
      }

      explicit Sequence(const Sequence& ref):mSequence(ref.mSequence)
      {
      }

      const Sequence& operator=(const Sequence& ref)
      {
        mSequence = ref.mSequence;
        return(*this);
      }

      const uint64_t getCurrent() const
      {
        return mSequence;
      }

      const uint64_t getNext() const
      {
        return getNext(Reverse);
      }
    };
  }
}

#endif /*SEQUENCE_H_*/
