/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: abstract/IController.h 1 2007-12-10 15:37:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef _ICONTROLLER_H
#define	_ICONTROLLER_H
#include <abstract/IView.h>
#include <memory>
#include <iostream>
#include <TSLog.h>

namespace itc
{
  namespace abstract
  {
    /**
     * @ Inherit this type to implement a specific controller
     **/
    template <typename TModel> class IController
    {
    public:
      typedef IView<TModel> ViewType;
      typedef std::weak_ptr<ViewType> ViewTypeSPtr;
      typedef TModel  ModelType;

      const bool notify(const TModel& pModel, ViewTypeSPtr& pView)
      {
        if(auto aViewPtr=pView.lock())
        {
          aViewPtr->update(pModel);
          return true;
        }
        else
        {
          itc::getLog()->error(__FILE__,__LINE__,"itc::abstract::IController::notify() a view is NULL");
          return false;
        }
      }
      const bool notify(const std::vector<TModel>& pModel, ViewTypeSPtr& pView)
      {
        if(auto aViewPtr=pView.lock())
        {
          aViewPtr->onBatchUpdate(pModel);
          return true;
        }
        else
        {
          itc::getLog()->error(__FILE__,__LINE__,"itc::abstract::IController::notify() a view is NULL");
          return false;
        }
      }
      protected:
      virtual ~IController()=default;
    };
  }
}
#endif	/* _ICONTROLLER_H */

