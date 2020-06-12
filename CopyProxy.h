#pragma once

#include "EventProxy.h"

#include <type_traits>
#include <utility>

template <typename Handler>
class CopyProxy : public EventProxy
{
public:
  CopyProxy() = delete;

  explicit CopyProxy(const Handler& handler);
  explicit CopyProxy(Handler&& handler);
  CopyProxy(const CopyProxy& val);
  CopyProxy(CopyProxy&& val) noexcept;
  ~CopyProxy() override;

  CopyProxy& operator=(const CopyProxy&) = default;
  CopyProxy& operator=(CopyProxy&&) noexcept = default;

  void handle_event() override;

private:
  const typename std::decay<Handler>::type handler_;
};

template <typename Handler>
CopyProxy<Handler>::CopyProxy(const Handler& handler) : handler_(handler) {
  /*std::cout << "CopyProxy<Handler>::CopyProxy(const Handler&)" << std::endl;*/
}

template <typename Handler>
CopyProxy<Handler>::CopyProxy(Handler&& handler) : handler_(std::move(handler)) {
  /*std::cout << "CopyProxy<Handler>::CopyProxy(Handler&&)" << std::endl;*/
}

template <typename Handler>
CopyProxy<Handler>::CopyProxy(const CopyProxy<Handler>& val) : handler_(val.handler_) {
  /*std::cout << "CopyProxy<Handler>::CopyProxy(const CopyProxy&)" << std::endl;*/
}

template <typename Handler>
CopyProxy<Handler>::CopyProxy(CopyProxy<Handler>&& val) noexcept : handler_(val.handler_) {
  /*std::cout << "CopyProxy<Handler>::CopyProxy(CopyProxy&&)" << std::endl;*/
}

template <typename Handler>
CopyProxy<Handler>::~CopyProxy() {
  /*std::cout << "CopyProxy<Handler>::~CopyProxy()" << std::endl;*/
}

template <typename Handler>
void CopyProxy<Handler>::handle_event() {
  handler_();
}


