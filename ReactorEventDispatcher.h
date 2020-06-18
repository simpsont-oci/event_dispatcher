#pragma once

#include "EventDispatcher.h"
#include "ThreadPool.h"

#include "ace/Event_Handler.h"
#include "ace/TP_Reactor.h"

#include <deque>
#include <mutex>

class ReactorEventDispatcher : public virtual ACE_Event_Handler, public virtual EventDispatcher
{
public:
  ReactorEventDispatcher();
  virtual ~ReactorEventDispatcher();

  std::shared_ptr<ACE_TP_Reactor> tp_reactor() { return reactor_; }

  std::shared_ptr<SystemTimer> get_timer() final;

  using EventDispatcher::DispatchStatus;

protected:

  DispatchStatus simple_dispatch(const std::shared_ptr<EventProxy>& proxy) override;

  int handle_timeout(const ACE_Time_Value&, const void* arg) override;

  void release(const std::shared_ptr<EventProxy>& proxy);

  mutable std::mutex mutex_;
  bool shutdown_;
  std::shared_ptr<ACE_TP_Reactor> reactor_;
  std::shared_ptr<ThreadPool> thread_pool_;
  std::deque<std::shared_ptr<EventProxy>> proxies_;
};

