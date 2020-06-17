#pragma once

#include "EventDispatcher.h"
#include "ThreadPool.h"

#include <boost/asio/io_service.hpp>

#include <deque>

class AsioEventDispatcher : public EventDispatcher
{
public:
  AsioEventDispatcher();
  virtual ~AsioEventDispatcher();

  std::shared_ptr<SystemTimer> get_timer() final;

  using EventDispatcher::DispatchStatus;

protected:

  DispatchStatus simple_dispatch(const std::shared_ptr<EventProxy>& proxy) override;

  void release(const std::shared_ptr<EventProxy>& proxy);

  mutable std::mutex mutex_;
  bool shutdown_;
  boost::asio::io_service io_service_;
  boost::asio::io_service::work io_service_work_;
  std::shared_ptr<ThreadPool> thread_pool_;
  std::deque<std::shared_ptr<EventProxy>> proxies_;
};

