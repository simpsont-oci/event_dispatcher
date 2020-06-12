#include "EventDispatcher.h"

EventDispatcher::DispatchStatus EventDispatcher::dispatch(const std::shared_ptr<EventProxy>& proxy) {
  //std::cout << "dispatch(const Handler& proxy)" << std::endl;
  return simple_dispatch(proxy);
}

