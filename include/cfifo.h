/**
 *  Copyright 2018, Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 * 
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 *  $Id: cfifo.h October 31, 2018 9:48 PM $
 * 
 **/

#ifndef __CFIFO_H__
#  define __CFIFO_H__
#include <atomic>
#include <vector>
#include <time.h>
#include <system_error>
#include <sys/semaphore.h>
#include <sys/mutex.h>
#include <mutex>

namespace itc
{
  template <typename T, const size_t max_attempts=50> class cfifo
  {
  private:
   
    using semaphore=itc::sys::semaphore<max_attempts>;
    
    struct sitem
    {
      bool                busy;
      itc::sys::mutex     lock;
      semaphore           sem;
      T                   item;
      
      explicit sitem() : busy{false},lock(){}
      sitem(const sitem&) = delete;
      sitem(sitem&) = delete;
      
      const bool set(const T& _item)
      {
        std::lock_guard<itc::sys::mutex> sync(lock);
        if(!busy)
        {
          
          item=_item;
          busy=true;
          sem.post();
          return true;
        }
        return false; // is not yet fetched
      }
      
      const bool fetch_and_clear(T& out)
      {
        
        sem.wait(); // the thread will wake if producer put the data in the cell;
                  // best case scenarion, - no waiting here because data is already there, worst case - fallback to POSIX sem_wait

        std::lock_guard<itc::sys::mutex> sync(lock);

        if(busy)
        {
          out=item;
          busy=false;
          return true;
        }
        return false;
      }
    };
    
    std::atomic<size_t>           next_push;
    std::atomic<size_t>           next_read;
    std::atomic<bool>             valid;
    size_t                        limit;
    std::vector<sitem>            queue;
    
    void assert_validity(const std::string& method)
    {
      if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::"+method+"() - accessing concurrent FIFO which is being destroyed");
    }
    
  public:
   
   explicit cfifo(const size_t qsz)
   :  next_push{0},next_read{0},valid{true}, limit{qsz}, queue(qsz)
   {
   }
   
   cfifo(cfifo&)=delete;
   cfifo(const cfifo&)=delete;
   
   const bool try_send(const T& data)
   {
     assert_validity("try_send");
     
     size_t pos = next_push.load();
     
     while(!next_push.compare_exchange_strong(pos,(pos+1) < limit ? pos+1 : 0))
     {
       pos = next_push.load();
     }
     
     if(queue[pos].set(data))
       return true;
     return false;
   }
   
   void send(const T& data)
   {
     while(!try_send(data));
   }
   
   const bool try_recv(T& result)
   {
     assert_validity("try_recv");
     
     size_t pos=next_read.load();
     
     while(!next_read.compare_exchange_strong(pos, (pos+1) < limit ? pos+1 : 0))
     {
       pos = next_read.load();
     }
     
     if(!queue[pos].fetch_and_clear(result))
       return false;
     return true;
   }
   
   auto recv()
   {
     T result;
     while(!try_recv(result));
     return std::move(result);
   }
   
   ~cfifo()
   {
     valid.store(false);
   }
  };
}

#endif /* __CFIFO_H__ */

