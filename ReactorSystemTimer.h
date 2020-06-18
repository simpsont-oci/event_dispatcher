#pragma once

#include "ReactorEventDispatcher.h"

class ReactorSystemTimer : public virtual ACE_Event_Handler, public virtual SystemTimer
{
public:
  const long INVALID_TIMER = 1;

  explicit ReactorSystemTimer(ReactorEventDispatcher* red);
  virtual ~ReactorSystemTimer();

  void expires_after(const Duration& duration) final;
  void expires_at(const TimePoint& timepoint) final;

  TimePoint expiry() const final;

  void wait() final;

  ConVarStatus wait_for(const Duration& duration) final;
  ConVarStatus wait_until(const TimePoint& timepoint) final;

  void simple_async_wait(const std::shared_ptr<EventProxy>& proxy) final;

protected:

  void check_and_create_timer_i();

  void simple_async_wait_i(const std::shared_ptr<EventProxy>& proxy);

  int handle_timeout(const ACE_Time_Value&, const void* arg) override;

  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  bool shutdown_;
  long timer_id_;
  TimePoint expiry_;
  ReactorEventDispatcher* red_;
  std::vector<std::shared_ptr<EventProxy>> proxies_;
};

