#include "ReactorEventDispatcher.h"

#include "ReactorSystemTimer.h"

ReactorEventDispatcher::ReactorEventDispatcher() : shutdown_(false), reactor_(new ACE_TP_Reactor()), thread_pool_(), proxies_()
{
  const size_t THREAD_POOL_SIZE = 4;
  thread_pool_.reset(new ThreadPool(THREAD_POOL_SIZE, [&](){
    std::shared_ptr<ACE_TP_Reactor> reactor(reactor_);
    while (!shutdown_) {
      ACE_Time_Value temp(3, 0);
      reactor_->handle_events(temp);
    }
  }));
}

ReactorEventDispatcher::~ReactorEventDispatcher()
{
  {
    std::unique_lock<std::mutex> lock(mutex_);

    shutdown_ = true;

    reactor_->cancel_timer(this);
    reactor_->deactivate(1);
  }

  thread_pool_.reset();
  reactor_->close();
  reactor_.reset();

  proxies_.clear();
}

std::shared_ptr<SystemTimer> ReactorEventDispatcher::get_timer() {
  return std::shared_ptr<SystemTimer>(new ReactorSystemTimer(this));
}

ReactorEventDispatcher::DispatchStatus ReactorEventDispatcher::simple_dispatch(const std::shared_ptr<EventProxy>& proxy)
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!shutdown_) {
    auto it = proxies_.emplace(proxies_.end(), proxy);
    reactor_->schedule_timer(this, &(*it), ACE_Time_Value::zero);
  }
  return DS_SUCCESS;
}

int ReactorEventDispatcher::handle_timeout(const ACE_Time_Value&, const void* arg)
{
  const auto& proxy = *(static_cast<const std::shared_ptr<EventProxy>*>(arg));
  proxy->handle_event();
  release(proxy);
  return 0;
}

void ReactorEventDispatcher::release(const std::shared_ptr<EventProxy>& proxy)
{
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

