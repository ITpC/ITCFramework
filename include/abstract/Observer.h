/**
 * Copyright Pavel Kraynyukhov 2007 - 2021.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: Observer.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __OBSERVER_H__
#define __OBSERVER_H__

namespace itc {
    namespace abstract {

        template <typename EventType, template <class> class TObservable> class Observer {
        public:

            inline void update(TObservable<EventType> *pObservable, const EventType& event) {
                onUpdate(pObservable, event);
            }

            virtual void onUpdate(TObservable<EventType> *pObservable, const EventType& event) = 0;

        protected:

            virtual ~Observer()=default;
        };
    }
}
#endif  //_OBSERVER_H_
