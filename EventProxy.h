#pragma once

class EventProxy
{
public:
  EventProxy() = default;
  EventProxy(const EventProxy&) = default;
  EventProxy(EventProxy&&) noexcept = default;
  EventProxy& operator=(const EventProxy&) = default;
  EventProxy& operator=(EventProxy&&) = default;
  virtual ~EventProxy() = default;

  virtual void handle_event() = 0;
};

