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
 * $Id: Observable.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/



#ifndef _OBSERVABLE_H_
#define _OBSERVABLE_H_

#include <Val2Type.h>
#include <memory>

namespace itc {
    namespace abstract {

        template <
            typename EventType,
            template <class> class TObserver,
            template <class> class TContainer,
            bool UseInsert = true
        > class Observable {
        private:
        protected:
            typedef TObserver<EventType> TPObserver;
            typedef std::shared_ptr<TObserver> TObserverSPtr;
            typedef TContainer<TObserverSPtr> TPOContainer;
            typedef typename TPOContainer::iterator TPOCIterator;

            TPOContainer Observers;
            Bool2Type<UseInsert> mUseInsert;
        public:

            Observable() {
            }

            inline void attach(const TObserverSPtr& anObserver, Bool2Type < true > fictive) {
                Observers.push_back(anObserver);
            }

            inline void attach(const TObserverSPtr& anObserver, Bool2Type < false > fictive) {
                Observers.insert(anObserver);
            }

            inline void attach(const TObserverSPtr& anObserver) {
                this->attach(anObserver, mUseInsert);
            }

            inline void detach(const TObserverSPtr& anObserver) {
                TPOCIterator i = std::find(Observers.begin(), Observers.end(), anObserver);

                if (i != Observers.end())
                    Observers.erase(i);
            }

            inline void detachAll() {
                Observers.clear();
            }

            inline void notifyAll(const EventType& anObject) {
                TPOCIterator ib = Observers.begin();
                TPOCIterator ie = Observers.end();
                for (; ib != ie; i++) notify(*i, anObject);
            }

            virtual void notify(const TObserverSPtr&, const EventType&) = 0;

        protected:

            virtual ~Observable() {
                detachAll();
            }
        };
    }
}
#endif  //_OBSERVABLE_H_
