#include "ReactorSystemTimer.h"

ReactorSystemTimer::ReactorSystemTimer(ReactorEventDispatcher* ped) : shutdown_(false), timer_id_(INVALID_TIMER), red_(ped) {}

ReactorSystemTimer::~ReactorSystemTimer() {
  std::unique_lock<std::mutex> lock(mutex_);

  shutdown_ = true;
  if (timer_id_ != INVALID_TIMER) {
    red_->tp_reactor()->cancel_timer(timer_id_);
    timer_id_ = INVALID_TIMER;
  }
}

void ReactorSystemTimer::expires_after(const SystemTimer::Duration& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  expiry_ = std::chrono::system_clock::now() + duration;
}

void ReactorSystemTimer::expires_at(const SystemTimer::TimePoint& timepoint) {
  std::unique_lock<std::mutex> lock(mutex_);
  expiry_ = timepoint;
}

SystemTimer::TimePoint ReactorSystemTimer::expiry() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return expiry_;
}

void ReactorSystemTimer::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  cv_.wait(lock);
}

SystemTimer::ConVarStatus ReactorSystemTimer::wait_for(const SystemTimer::Duration& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  return cv_.wait_for(lock, duration);
}

SystemTimer::ConVarStatus ReactorSystemTimer::wait_until(const SystemTimer::TimePoint& timepoint) {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  return cv_.wait_until(lock, timepoint);
}

void ReactorSystemTimer::simple_async_wait(const std::shared_ptr<EventProxy>& proxy) {
  std::unique_lock<std::mutex> lock(mutex_);
  simple_async_wait_i(proxy);
}

void ReactorSystemTimer::check_and_create_timer_i() {
  if (timer_id_ == INVALID_TIMER) {
    TimePoint now = std::chrono::system_clock::now();
    auto sec_delta = std::chrono::duration_cast<std::chrono::seconds>(expiry_ - now).count();
    auto usec_delta = std::chrono::duration_cast<std::chrono::microseconds>((expiry_ - now) - std::chrono::seconds(sec_delta)).count();
    red_->tp_reactor()->schedule_timer(this, 0, ACE_Time_Value(sec_delta, usec_delta));
  }
}

void ReactorSystemTimer::simple_async_wait_i(const std::shared_ptr<EventProxy>& proxy) {
  if (!shutdown_) {
    proxies_.emplace_back(proxy);
    check_and_create_timer_i();
  }
}

int ReactorSystemTimer::handle_timeout(const ACE_Time_Value&, const void* arg) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto it = proxies_.begin(); it != proxies_.end(); ++it) {
    red_->dispatch(*it);
  }
  proxies_.clear();
  timer_id_ = INVALID_TIMER;
  cv_.notify_all();
  return 0;
}

