#pragma once

#include "EventDispatcher.h"

#include "ace/Proactor.h"

#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class ProactorEventDispatcher : public virtual ACE_Handler, public virtual EventDispatcher
{
public:
  ProactorEventDispatcher();
  virtual ~ProactorEventDispatcher();

  std::shared_ptr<ACE_Proactor> proactor() { return proactor_; }

  std::shared_ptr<SystemTimer> get_timer() final;

  using EventDispatcher::DispatchStatus;

protected:

  DispatchStatus simple_dispatch(const std::shared_ptr<EventProxy>& proxy) override;

  void handle_time_out(const ACE_Time_Value&, const void* arg) override;

  void release(const std::shared_ptr<EventProxy>& proxy);

  mutable std::mutex mutex_;
  bool shutdown_;
  std::shared_ptr<ACE_Proactor> proactor_;
  std::vector<std::shared_ptr<std::thread> > thread_pool_;
  std::deque<std::shared_ptr<EventProxy>> proxies_;
};

