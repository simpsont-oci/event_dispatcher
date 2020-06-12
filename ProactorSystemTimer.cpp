#include "ProactorSystemTimer.h"

//TODO REMOVE
#include <iostream>
#include <ctime>
#include <iomanip>

ProactorSystemTimer::ProactorSystemTimer(ProactorEventDispatcher* ped) : shutdown_(false), timer_id_(INVALID_TIMER), ped_(ped) {}

ProactorSystemTimer::~ProactorSystemTimer() {
  std::unique_lock<std::mutex> lock(mutex_);

  shutdown_ = true;
  if (timer_id_ != INVALID_TIMER) {
    ped_->proactor()->cancel_timer(timer_id_);
    timer_id_ = INVALID_TIMER;
  }
}

void ProactorSystemTimer::expires_after(const SystemTimer::Duration& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  expiry_ = std::chrono::system_clock::now() + duration;
}

void ProactorSystemTimer::expires_at(const SystemTimer::TimePoint& timepoint) {
  std::unique_lock<std::mutex> lock(mutex_);
  expiry_ = timepoint;
}

SystemTimer::TimePoint ProactorSystemTimer::expiry() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return expiry_;
}

void ProactorSystemTimer::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  cv_.wait(lock);
}

SystemTimer::ConVarStatus ProactorSystemTimer::wait_for(const SystemTimer::Duration& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  return cv_.wait_for(lock, duration);
}

SystemTimer::ConVarStatus ProactorSystemTimer::wait_until(const SystemTimer::TimePoint& timepoint) {
  std::unique_lock<std::mutex> lock(mutex_);
  check_and_create_timer_i();
  return cv_.wait_until(lock, timepoint);
}

void ProactorSystemTimer::simple_async_wait(const std::shared_ptr<EventProxy>& proxy) {
  std::unique_lock<std::mutex> lock(mutex_);
  simple_async_wait_i(proxy);
}

void ProactorSystemTimer::check_and_create_timer_i() {
  if (timer_id_ == INVALID_TIMER) {
    TimePoint now = std::chrono::system_clock::now();
    //std::time_t expiry_time_t = std::chrono::system_clock::to_time_t(expiry_);
    //std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    //std::cout << "expiry_ = " << std::put_time(std::localtime(&expiry_time_t), "%F %T") << std::endl;
    //std::cout << "now() = " << std::put_time(std::localtime(&now_time_t), "%F %T") << std::endl;
    auto sec_delta = std::chrono::duration_cast<std::chrono::seconds>(expiry_ - now).count();
    auto usec_delta = std::chrono::duration_cast<std::chrono::microseconds>((expiry_ - now) - std::chrono::seconds(sec_delta)).count();
    //std::cout << "delta -> time_t = " << std::chrono::system_clock::to_time_t(delta) << std::endl;
    ped_->proactor()->schedule_timer(*this, 0, ACE_Time_Value(sec_delta, usec_delta));
  }
}

void ProactorSystemTimer::simple_async_wait_i(const std::shared_ptr<EventProxy>& proxy) {
  if (!shutdown_) {
    proxies_.emplace_back(proxy);
    check_and_create_timer_i();
  }
}

void ProactorSystemTimer::handle_time_out(const ACE_Time_Value&, const void* arg) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto it = proxies_.begin(); it != proxies_.end(); ++it) {
    ped_->dispatch(*it);
  }
  proxies_.clear();
  timer_id_ = INVALID_TIMER;
  cv_.notify_all();
}

