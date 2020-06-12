#pragma once

#include "CopyProxy.h"
#include "SystemTimer.h"

#include <memory>

class EventDispatcher
{
public:
  EventDispatcher() {}
  virtual ~EventDispatcher() {}

  enum DispatchStatus : uint32_t
  {
    DS_UNKNOWN = 0,
    DS_SUCCESS = 1,
    DS_ERROR = 2
  };

  template <typename Handler>
  DispatchStatus dispatch(const Handler& handler)
  {
    //std::cout << "dispatch(const Handler& handler)" << std::endl;
    return simple_dispatch(std::shared_ptr<EventProxy>(new CopyProxy<Handler>(handler)));
  }

  DispatchStatus dispatch(const std::shared_ptr<EventProxy>& proxy);

  virtual std::shared_ptr<SystemTimer> get_timer() = 0;

protected:
  virtual DispatchStatus simple_dispatch(const std::shared_ptr<EventProxy>& proxy) = 0;
};

