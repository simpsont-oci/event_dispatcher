#pragma once

#include "AsioEventDispatcher.h"
#include "SystemTimer.h"

#include <boost/asio.hpp>

#include <condition_variable>
#include <mutex>

class AsioSystemTimer : public virtual SystemTimer
{
public:

  explicit AsioSystemTimer(AsioEventDispatcher& ped);
  virtual ~AsioSystemTimer();

  void expires_after(const Duration& duration) final;
  void expires_at(const TimePoint& timepoint) final;

  TimePoint expiry() const final;

  void wait() final;

  ConVarStatus wait_for(const Duration& duration) final;
  ConVarStatus wait_until(const TimePoint& timepoint) final;

  void simple_async_wait(const std::shared_ptr<EventProxy>& proxy) final;

protected:

  std::mutex mutex_;
  std::condition_variable cv_;
  boost::asio::system_timer timer_;
};

