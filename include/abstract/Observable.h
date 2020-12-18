/**
 * Copyright Pavel Kraynyukhov 2007 - 2021.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Observable.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
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
