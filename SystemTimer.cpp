#include "SystemTimer.h"

SystemTimer::SystemTimer()
{
}

void SystemTimer::async_wait(const std::shared_ptr<EventProxy>& proxy)
{
  //std::cout << "schedule(const Handler& proxy)" << std::endl;
  return simple_async_wait(proxy);
}
