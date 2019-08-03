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
#include <queue>
#include <map>
#include <type_traits>
#include <system_error>

namespace itc
{
  template <typename T> class cfifo
  {
  private:
   
    struct store_value_type
    {
      std::atomic<bool> can_read;
      T data;
      store_value_type():can_read{false}{}
      store_value_type(const T& _data):can_read{true},data{_data}{}
      store_value_type(T&& _data):can_read{true},data{std::move(_data)}{}
      ~store_value_type()=default;
    };
    
    using store_type=std::pair<std::atomic_flag,store_value_type>;
    using storage_type=std::vector<store_type>;
    
    size_t                        limit;
    std::atomic<size_t>           next_push;
    std::atomic<size_t>           next_read;
    std::atomic<bool>             valid;
    std::atomic<int64_t>          depth;
    storage_type                  queue;
    
    
  public:
   
   explicit cfifo(const size_t qsz)
   :  limit{qsz},next_push{0},next_read{0},valid{false}, depth{0}, queue(qsz)
   {
/*     static_assert(
      std::is_trivially_constructible<T>::value || std::is_trivially_constructible<T,T>::value,
      "itc::cfifo<T>::cfifo(sz), - T must be trivially constructible"
     );*/
     
     // must set all flags to false explicitly, 
     // because no default static initialization is possible
     
     for(size_t i=0;i<limit;++i)
     {
       queue[i].first.clear();
     }
     valid.store(true);
   }
   
   cfifo(cfifo&)=delete;
   cfifo(const cfifo&)=delete;
   
   const bool try_send(const T& data)
   {
     // fix on next possible position
     
     size_t pos = next_push.load();
     // skip while valid, next_push is moved by other threads
     while(assert_valid("try_send")&&(!next_push.compare_exchange_strong(pos,(pos+1) < limit ? pos+1 : 0)))
     {
       // can't fix on position, getting the next one
       pos = next_push.load();
     }
     
     // concurrency:
     // we are safe only until next thread do not try to push at this position
     // after full pos cycle
     
     if(queue[pos].first.test_and_set()) // is tested by another thread
     {
       // we may not be sure if it is a producer or consumer 
       // which we are competing with. So: BAIL OUT!
       return false;
     }
     
     // we are safe to continue, - this is the testing thread
     
     if(queue[pos].second.can_read.load()) // there is the data already!
     {
       // The data are available for consumer to read. So: BAIL OUT!
       queue[pos].first.clear();
       return false;
     }
     
     queue[pos].second.data=data;

     
     // assign flags in reverse order
     queue[pos].second.can_read.store(true); 
     depth.fetch_add(1);
     queue[pos].first.clear();
     
     return true;
   }
   
   const bool try_recv(T& result)
   {
     size_t pos=next_read.load();
     
     while(assert_valid("try_recv")&&(!next_read.compare_exchange_strong(pos, (pos+1) < limit ? pos+1 : 0)))
     {
       pos = next_read.load();
     }
     
     if(queue[pos].first.test_and_set()) // is tested by another thread
     {
       // we may not be sure if it is a producer or consumer 
       // we are competing with. So: BAIL OUT!
       return false;
     }
     
     // we are safe to continue, - this is the testing thread
     
     if(queue[pos].second.can_read.load()) // the data available
     {
       result=std::move(queue[pos].second.data);
       queue[pos].second.can_read.store(false);
       queue[pos].first.clear();
       depth.fetch_sub(1);
       return true;
     }
     
     queue[pos].first.clear();
     return false;
   }
   
   void send(const T& data) // blocks until can store the data, while cfifo is valid
   {
     while(!try_send(data));
   }
   
   auto recv() // blocks until data is available while cfifo is valid
   {
     T result;
     while(!try_recv(result));
     return std::move(result);
   }
   
   const bool assert_valid(const std::string& func)
   {
     if(!valid.load())
     {
       throw std::system_error(EIDRM,std::system_category(),"cfifo<T>::"+func+"(), - queue is being removed");
     }
     return true;
   }
   
   // unreliable but eventually correct
   const bool empty() const
   {
     return depth.load() <= 0;
   }
   
   // unreliable but eventually correct
   const size_t size()
   {
     auto ret=depth.load();
     if(ret>0) return static_cast<size_t>(ret);
     else return 0;
   }
   
   void shutdown()
   {
     valid.store(false);
   }
   
   ~cfifo()
   {
     shutdown();
   };
  };
}

#endif /* __CFIFO_H__ */