#include "ProactorEventDispatcher.h"

#include "ProactorSystemTimer.h"

ProactorEventDispatcher::ProactorEventDispatcher() : shutdown_(false), proactor_(new ACE_Proactor()), thread_pool_(), proxies_()
{
  const size_t THREAD_POOL_SIZE = 4;
  proactor_->number_of_threads(THREAD_POOL_SIZE);
  thread_pool_.reset(new ThreadPool(THREAD_POOL_SIZE, [&](){
    std::shared_ptr<ACE_Proactor> proactor(proactor_);
    proactor->proactor_run_event_loop();
  }));
}

ProactorEventDispatcher::~ProactorEventDispatcher()
{
  {
    std::unique_lock<std::mutex> lock(mutex_);

    shutdown_ = true;
    proactor_->cancel_timer(*this);
    proactor_->proactor_end_event_loop();
  }
  thread_pool_.reset();
  proactor_.reset();

  proxies_.clear();
}

std::shared_ptr<SystemTimer> ProactorEventDispatcher::get_timer() {
  return std::shared_ptr<SystemTimer>(new ProactorSystemTimer(this));
}

EventDispatcher::DispatchStatus ProactorEventDispatcher::simple_dispatch(const std::shared_ptr<EventProxy>& proxy) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!shutdown_) {
    auto it = proxies_.emplace(proxies_.end(), proxy);
    proactor_->schedule_timer(*this, &(*it), ACE_Time_Value::zero);
  }
  return DS_SUCCESS;
}

void ProactorEventDispatcher::handle_time_out(const ACE_Time_Value&, const void* arg) {
  const auto& proxy = *(static_cast<const std::shared_ptr<EventProxy>*>(arg));
  proxy->handle_event();
  release(proxy);
}

void ProactorEventDispatcher::release(const std::shared_ptr<EventProxy>& proxy) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!proxies_.empty() && proxies_.front() == proxy) {
    proxies_.pop_front();
    while (!proxies_.empty() && !proxies_.front()) {
      proxies_.pop_front();
    }
  } else {
    auto it = std::find(proxies_.begin(), proxies_.end(), proxy);
    if (it != proxies_.end()) {
      it->reset();
    }
  }
}

