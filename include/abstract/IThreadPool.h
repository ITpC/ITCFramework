/**
 * Copyright Pavel Kraynyukhov 2007 - 2021.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: IThreadPool.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __ITHREADPOOL_H__
#    define __ITHREADPOOL_H__

#include <memory>
namespace itc
{
    namespace abstract
    {
        /**
         * @brief it is a simple interface of the thread pool.
         */
        class IThreadPool
        {
        public:
            typedef std::shared_ptr<abstract::IRunnable> value_type;

            virtual const bool getAutotune() const = 0;
            virtual const size_t getThreadsCount() const = 0;
            virtual const size_t getMaxThreads() const = 0;
            
            virtual void setAutotune(const bool&) = 0;
            virtual void expand(const size_t&) = 0;
            virtual void reduce(const size_t&) = 0;
            virtual void enqueue(const value_type&) = 0;
            
        protected:
            virtual ~IThreadPool()=default;
        };
    }
}
#endif /*ITHREADPOOL_H_*/
