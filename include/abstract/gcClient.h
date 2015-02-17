/* 
 * File:   gcclient.h
 * Author: pk
 *
 * Created on 13 Декабрь 2014 г., 10:42
 */

#ifndef GCCLIENT_H
#define	GCCLIENT_H

#include <abstract/Runnable.h>

namespace itc
{
    namespace abstract
    {
        class gcClient : public itc::abstract::IRunnable
       {
       public:
           virtual bool canRemove()=0;
           virtual ~gcClient()=0;
       };       
    }
}

#endif	/* GCCLIENT_H */
