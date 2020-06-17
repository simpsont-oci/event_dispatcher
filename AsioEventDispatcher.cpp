#include "AsioEventDispatcher.h"

//TODO This is a hack until real timer exists
#include "ProactorSystemTimer.h"

#include "boost/asio.hpp"

AsioEventDispatcher::AsioEventDispatcher() : shutdown_(false), io_service_(), io_service_work_(io_service_), thread_pool_(), proxies_()
{
  const size_t THREAD_POOL_SIZE = 4;
  thread_pool_.reset(new ThreadPool(THREAD_POOL_SIZE, [&](){
    while (!io_service_.stopped())
    {
      io_service_.run_one();
    }
  }));
}

AsioEventDispatcher::~AsioEventDispatcher()
{
  std::unique_lock<std::mutex> lock(mutex_);

  shutdown_ = true;
  io_service_.stop();
}

std::shared_ptr<SystemTimer> AsioEventDispatcher::get_timer() {
  return std::shared_ptr<SystemTimer>(new ProactorSystemTimer(nullptr));
}

AsioEventDispatcher::DispatchStatus AsioEventDispatcher::simple_dispatch(const std::shared_ptr<EventProxy>& proxy)
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!shutdown_) {
    auto it = proxies_.emplace(proxies_.end(), proxy);
    boost::asio::post(io_service_, [it] () {
      (*it)->handle_event();
    });
  }
  return DS_SUCCESS;
}

void AsioEventDispatcher::release(const std::shared_ptr<EventProxy>& proxy)
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

