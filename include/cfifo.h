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

#ifndef __FCQUEUE_H__
#  define __FCQUEUE_H__
#include <atomic>
#include <vector>
#include <time.h>
#include <system_error>
#include <sys/semaphore.h>

namespace itc
{
  template <typename T, const size_t max_attempts=50> class cfifo
  {
  private:
   
    using semaphore=itc::sys::semaphore<max_attempts>;
    
    struct sitem
    {
      std::atomic<bool>   busy;
      std::atomic_flag    lock;
      semaphore           sem;
      T                   item;
      
      explicit sitem() : busy{false},lock{ATOMIC_FLAG_INIT}{}
      sitem(const sitem&) = delete;
      sitem(sitem&) = delete;
      
      const bool set(const T& _item)
      {
        if(!busy.load())
        {
          if(lock.test_and_set()) return false; // already locked by another thread
          
          bool current_value=false; 
          if(busy.compare_exchange_strong(current_value, true, std::memory_order_seq_cst))
          {
            item=std::move(_item);
            lock.clear();
            sem.post();
            return true;
          }
          lock.clear();
          return false;
        }
        return false;
      }
      
      const bool fetch_and_clear(T& out)
      {
        sem.wait(); // if there are data in item, this thread will wake
                    // best case no waiting here, worst case fallback to POSIX semaphore
        
        if(busy.load()) // ensure the item has data (no other thread has cleared the item)
        {
          if(lock.test_and_set()) return false; // lock is already set by other thread;
          
          bool current_value=true; // must be busy/loaded with data
          if(busy.compare_exchange_strong(current_value, false, std::memory_order_seq_cst))
          { // if busy, then fetch the data, clear the lock
            out=std::move(item);
            lock.clear();
            return true;
          }
          lock.clear();
          return false;
        }
        return false;
      }
    };
    
    std::atomic<size_t> next_push;
    std::atomic<size_t> next_read;
    std::atomic<bool>   valid;
    size_t              limit;
    std::vector<sitem>  queue;
    
  public:
   
   explicit cfifo(const size_t qsz)
   :  next_push{0},next_read{0},valid{true}, limit{qsz}, queue(qsz)
   {
   }
   
   cfifo(cfifo&)=delete;
   cfifo(const cfifo&)=delete;
   
   void send(const T& data)
   {
     send_again:
     if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::send() - accessing concurrent FIFO which is being destroyed");
     
     size_t pos = next_push.load();
     
     while(!next_push.compare_exchange_strong(pos,(pos+1) < limit ? pos+1 : 0))
     {
       pos = next_push.load();
     }
     
     if(!queue[pos].set(data)) // look for a next position if could not set the data
       goto send_again;
   }
   
   auto recv()
   {
     recv_again:
     if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::recv() - accessing concurrent FIFO which is being destroyed");
     
     size_t pos=next_read.load();
     
     while(!next_read.compare_exchange_strong(pos, (pos+1) < limit ? pos+1 : 0))
     {
       pos = next_read.load();
     }
     
     T result;
     if(!queue[pos].fetch_and_clear(result)) // look for a next position if could not fetch the data
       goto recv_again;
     return result;
   }
   
   ~cfifo()
   {
     valid.store(false);
   }
  };
}

#endif /* __FCQUEUE_H__ */

