#include "AsioSystemTimer.h"

AsioSystemTimer::AsioSystemTimer(AsioEventDispatcher& aed) : timer_(aed.get_io_service()) {
}

AsioSystemTimer::~AsioSystemTimer() {
}

void AsioSystemTimer::expires_after(const SystemTimer::Duration& duration) {
  timer_.expires_after(duration);
}

void AsioSystemTimer::expires_at(const SystemTimer::TimePoint& timepoint) {
  timer_.expires_at(timepoint);
}

SystemTimer::TimePoint AsioSystemTimer::expiry() const {
  return timer_.expiry();
}

void AsioSystemTimer::wait() {
  timer_.wait();
}

SystemTimer::ConVarStatus AsioSystemTimer::wait_for(const SystemTimer::Duration& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, duration);
}

SystemTimer::ConVarStatus AsioSystemTimer::wait_until(const SystemTimer::TimePoint& timepoint) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_until(lock, timepoint);
}

void AsioSystemTimer::simple_async_wait(const std::shared_ptr<EventProxy>& proxy) {
  timer_.async_wait([&, proxy](const boost::system::error_code& error){
    if (error != boost::asio::error::operation_aborted) {
      cv_.notify_all();
      proxy->handle_event();
    }
  });
}

