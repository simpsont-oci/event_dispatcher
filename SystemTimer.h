#pragma once

#include "CopyProxy.h"

#include <chrono>
#include <condition_variable>
#include <memory>

class SystemTimer {
public:
  using Duration = std::chrono::system_clock::duration;
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  using ConVarStatus = std::cv_status;

  SystemTimer();
  virtual ~SystemTimer() {}

  virtual void expires_after(const Duration& duration) = 0;
  virtual void expires_at(const TimePoint& timepoint) = 0;
  virtual TimePoint expiry() const = 0;

  virtual void wait() = 0;
  virtual ConVarStatus wait_for(const Duration& duration) = 0;
  virtual ConVarStatus wait_until(const TimePoint& timepoint) = 0;

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    //std::cout << "schedule(const Handler& handler)" << std::endl;
    return simple_async_wait(std::shared_ptr<EventProxy>(new CopyProxy<Handler>(handler)));
  }

  void async_wait(const std::shared_ptr<EventProxy>& proxy);

protected:
  virtual void simple_async_wait(const std::shared_ptr<EventProxy>& proxy) = 0;
};

