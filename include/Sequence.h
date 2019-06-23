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
#define SEQUENCE_H_

#include <compat_types.h>
#include <Val2Type.h>
#include <unistd.h>
#include <atomic>
#include <limits>
#include <mutex>
#include <sys/synclock.h>

#include "Singleton.h"

namespace itc
{
  using namespace itc::utils;

  /**
   * @brief This template class provides sequence of a specified integral type.
   * Rotate and Reverse template parameters specify the behavior of a sequence:
   *  Rotate - starts the sequence from beginning after it reaches end of sequence
   *  Reverse - specifies that the sequence starts from max and ends at 0
   *
   * NOTE: specifien a signed integer value will cut the sequence in a half as
   * the range of the sequence is [0..<type>max] if Reverse equals false and
   * [<type>max..0] if Reverse equals true.
   * 
   * @exception std::out_of_range will be thrown if Rotate is false and
   * sequence reached its highest value.
   * 
   **/
  template <typename IntType, bool Rotate = true, bool Reverse = false >
  class Sequence
  {
  private:
    std::atomic<IntType> mSequence;
    Bool2Type<Reverse> mOrder;
    Bool2Type<Rotate> mCyclic;

    const IntType getNext(Bool2Type < false > reverse, Bool2Type <false> cyclic)
    {
      static IntType _max = std::numeric_limits<IntType>::max();

      if (mSequence.load() == _max)
      {
        throw std::out_of_range("Sequence is out of range");
      }else
      {
        return mSequence.fetch_add(1) + 1;
      }
    }

    const IntType getNext(Bool2Type < false > reverse, Bool2Type <true> cyclic)
    {
      static IntType _max = std::numeric_limits<IntType>::max();

      if (mSequence.load() == _max)
      {
        mSequence.store(0);
        return 0;
      }else
      {
        return mSequence.fetch_add(1) + 1 ;
      }
    }

    const IntType getNext(Bool2Type < true > reverse, Bool2Type <true> cyclic)
    {
      static IntType _max = std::numeric_limits<IntType>::max();
      if (mSequence.load() == 0)
      {
        mSequence.store(_max);
        return _max;
      }else
      {
        return mSequence.fetch_sub(1) - 1;
      }
    }

    const IntType getNext(Bool2Type < true > reverse, Bool2Type <false> cyclic)
    {
      static IntType _max = std::numeric_limits<IntType>::max();
      if (mSequence.load() == 0)
      {
        throw std::out_of_range("Sequence is out of range");
      }else
      {
        return mSequence.fetch_sub(1) - 1;
      }
    }


  public:

    explicit Sequence() : mSequence(Reverse ? std::numeric_limits<IntType>::max() : 0) { }

    explicit Sequence(const Sequence& ref) : mSequence{ref.mSequence} { }

    explicit Sequence(Sequence& ref) : mSequence{ref.mSequence} { }

    Sequence& operator=(const Sequence& ref)
    {
      mSequence.store(ref.mSequence.load());
      return (*this);
    }

    const IntType getCurrent()
    {
      return mSequence.load();
    }

    operator uint64_t()
    {
      return getCurrent();
    }
    
    const IntType next()
    {
      return getNext();
    }

    const IntType getNext()
    {
      return getNext(mOrder, mCyclic);
    }
  };
}

#endif /*SEQUENCE_H_*/
