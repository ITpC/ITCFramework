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
      
      const bool try_set(const T& _item)
      {
        if(lock.test_and_set(std::memory_order_seq_cst)) return false;
        
        bool current_value=false;
        
        while(busy.compare_exchange_strong(current_value, true, std::memory_order_seq_cst))
        {
          current_value=false;
        };
        
        item=std::move(_item);
        lock.clear(std::memory_order_seq_cst);
        sem.post();
      }
      
      void set(const T& _item)
      {
        while(lock.test_and_set(std::memory_order_seq_cst));
        
        bool current_value=false;
        
        while(busy.compare_exchange_strong(current_value, true, std::memory_order_seq_cst))
        {
          current_value=false;
        };
        item=std::move(_item);
        lock.clear(std::memory_order_seq_cst);
        sem.post();
      }
      
      void fetch_and_clear(T& out)
      {
        sem.wait();
        
        while(lock.test_and_set(std::memory_order_seq_cst));
        
        bool current_value=true;
        
        while(!busy.compare_exchange_strong(current_value, false, std::memory_order_seq_cst))
        {
          current_value=true;
        };
        
        out=std::move(item);
        lock.clear(std::memory_order_seq_cst);
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
   
   const bool try_send(const T& data)
   {
     if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::try_send() - accessing concurrent FIFO which is being destroyed");
     
     size_t pos = next_push.load();
     
     while(!next_push.compare_exchange_strong(pos,(pos+1) < limit ? pos+1 : 0))
     {
       pos = next_push.load();
     }
     
     if(queue[pos].try_set(data))
       return true;
     else
       return false;
   }
   
   void send(const T& data)
   {
     if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::send() - accessing concurrent FIFO which is being destroyed");
     
     size_t pos = next_push.load();
     
     while(!next_push.compare_exchange_strong(pos,(pos+1) < limit ? pos+1 : 0))
     {
       pos = next_push.load();
     }
     
     queue[pos].set(data);
   }
   
   auto recv()
   {
     if(!valid.load())
       throw std::system_error(EOWNERDEAD,std::system_category(),"cfifo::recv() - accessing concurrent FIFO which is being destroyed");
     
     size_t pos=next_read.load();
     
     while(!next_read.compare_exchange_strong(pos, (pos+1) < limit ? pos+1 : 0))
     {
       pos = next_read.load();
     }
     
     T result;
     queue[pos].fetch_and_clear(result);
     return result;
   }
   
   ~cfifo()
   {
     valid.store(false);
   }
  };
}

#endif /* __FCQUEUE_H__ */

