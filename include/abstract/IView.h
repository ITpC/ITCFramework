/**
 * Copyright Pavel Kraynyukhov 2007 - 2021.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: IView.h $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef  __IVIEW_H__
#  define	__IVIEW_H__


namespace itc
{
  namespace abstract
  {
    /**
     * @brief View interface which will work together with an implementation of 
     * abstract::IController
     **/
    template <typename TModel> class IView
    {
     public:
      typedef TModel value_type;
      void update(const TModel& pM)
      {
        this->onUpdate(pM);
      }
      void update(const std::vector<TModel>& pM)
      {
        this->onUpdate(pM);
      }
      
    protected:
      virtual void onUpdate(const TModel&) = 0;
      virtual void onUpdate(const std::vector<TModel>&) = 0;
      
      virtual ~IView() = default;
    };
  }
}


#endif	/* __IVIEW_H__ */
