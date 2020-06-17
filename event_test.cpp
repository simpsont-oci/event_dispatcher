#include "SystemTimer.h"
#include "AsioEventDispatcher.h"
#include "ProactorEventDispatcher.h"
#include "ReactorEventDispatcher.h"

#include "ace/Init_ACE.h"

#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

/* Test Handlers */

void test_handler_void() {
  //std::stringstream ss;
  //ss << "test_handler_void" << std::endl;
  //std::cout << ss.str() << std::flush;
}

void test_handler_std_func() {
  //std::stringstream ss;
  //ss << "test_handler_std_func" << std::endl;
  //std::cout << ss.str() << std::flush;
}

void test_handler_std_bind(unsigned int a) {
  //std::stringstream ss;
  //ss << "test_handler_std_bind a=" << a << std::endl;
  //std::cout << ss.str() << std::flush;
}

struct test_handler_fobj {
  void operator()() {
    //std::stringstream ss;
    //ss << "test_handler_fobj" << std::endl;
    //std::cout << ss.str() << std::flush;
  }
  void operator()() const {
    //std::stringstream ss;
    //ss << "test_handler_fobj (const)" << std::endl;
    //std::cout << ss.str() << std::flush;
  }
};

struct test_recursive_handler_fobj {
  test_recursive_handler_fobj() = delete;
  test_recursive_handler_fobj(const test_recursive_handler_fobj&) = default;
  test_recursive_handler_fobj(test_recursive_handler_fobj&&) = delete;
  test_recursive_handler_fobj operator=(const test_recursive_handler_fobj&) = delete;

  test_recursive_handler_fobj(EventDispatcher& dispatcher, const std::string& name, size_t height, size_t leaves = 1u) : dispatcher_(dispatcher), name_(name), height_(height), leaves_(leaves) {
    //std::stringstream ss;
    //ss << "test_recursive_handler_fobj::test_recursive_handler_fobj() : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;

    if (height_ > 0) {
      handler_.reset(new test_recursive_handler_fobj(dispatcher_, name_, height_ - 1, leaves_));
    }
  }
  ~test_recursive_handler_fobj() {
    //std::stringstream ss;
    //ss << "~test_recursive_handler_fobj::test_recursive_handler_fobj() : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;
  }

  void operator()() {
    //std::stringstream ss;
    //ss << "test_recursive_handler_fobj : " << name_ << " : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;

    if (height_ > 0) {
      for (size_t i = 0; i < leaves_; ++i) {
        dispatcher_.dispatch(*handler_);
      }
    }
  }

  void operator()() const {
    //std::stringstream ss;
    //ss << "test_recursive_handler_fobj (const) : " << name_ << " : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;

    if (height_ > 0) {
      for (size_t i = 0; i < leaves_; ++i) {
        dispatcher_.dispatch(*handler_);
      }
    }
  }

  EventDispatcher& dispatcher_;
  std::string name_;
  size_t height_;
  size_t leaves_;
  std::shared_ptr<test_recursive_handler_fobj> handler_;
};

/* run_test */

void run_test(std::shared_ptr<EventDispatcher> dispatcher, uint32_t delay) {

  // void()
  dispatcher->dispatch(test_handler_void);

  // custom function object
  test_handler_fobj fobj;
  dispatcher->dispatch(fobj);

  const test_handler_fobj cfobj;
  dispatcher->dispatch(cfobj);

  dispatcher->dispatch(test_handler_fobj());

  // lambda
  dispatcher->dispatch([] () {
    //std::stringstream ss;
    //ss << "test_handler_lambda" << std::endl;
    //std::cout << ss.str() << std::flush;
  });

  // std::function
  std::function<void()> ffobj(test_handler_std_func);
  dispatcher->dispatch(ffobj);

  dispatcher->dispatch(std::function<void()>(test_handler_std_func));

  // std::bind
  auto bfobj = std::bind(&test_handler_std_bind, 15);
  dispatcher->dispatch(bfobj);

  dispatcher->dispatch(std::bind(&test_handler_std_bind, 7));

  // use EventProxy API
  std::shared_ptr<EventProxy> proxy(new CopyProxy<test_handler_fobj>(fobj));
  dispatcher->dispatch(proxy);

  dispatcher->dispatch(std::shared_ptr<EventProxy>(new CopyProxy<test_handler_fobj>(fobj)));

  // recursive calls
  test_recursive_handler_fobj rfobj(*dispatcher, "depth", 6);
  dispatcher->dispatch(rfobj);

  test_recursive_handler_fobj rfobj2(*dispatcher, "breadth", 3, 2);
  dispatcher->dispatch(rfobj2);

  // custom sleep time
  ACE_OS::sleep(delay);
}

class WaitHandler : public EventProxy {
public:
  WaitHandler() : called_(false) {}

  void handle_event() final {
    std::unique_lock<std::mutex> lock(mutex_);
    called_ = true;  
    cv_.notify_all();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!called_) {
      cv_.wait(lock);
    }
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  bool called_;
};

void run_speed_test(std::shared_ptr<EventDispatcher> dispatcher) {
  const size_t test_size = 1000000;
  std::vector<std::shared_ptr<EventProxy>> handlers;

  for (size_t i = 0; i < test_size; ++i) {
    handlers.emplace_back(new WaitHandler());
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (auto it = handlers.cbegin(); it != handlers.cend(); ++it) {
    dispatcher->dispatch(*it);
  }

  for (auto it = handlers.cbegin(); it != handlers.cend(); ++it) {
    auto wh = std::dynamic_pointer_cast<WaitHandler>(*it);
    if (wh) {
      wh->wait();
    } else {
      std::cout << "ERROR" << std::endl;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();

  handlers.clear(); 

  double seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  seconds /= 1e3;

  std::cout << "run_speed_test finished in " << seconds << " seconds." << std::endl;
}

void run_dispatcher_test_suite(std::shared_ptr<EventDispatcher> dispatcher) {
  run_test(dispatcher, 3);
  run_speed_test(dispatcher);
  run_test(dispatcher, 0); // Test clean shutdown
}

class timer_func_obj : public EventProxy, public std::enable_shared_from_this<timer_func_obj> {
public:
  timer_func_obj() = delete;
  timer_func_obj(const timer_func_obj&) = delete;
  timer_func_obj& operator=(const timer_func_obj&) = delete;

  void stop() {
    stopping_ = true;
  }

  timer_func_obj(std::shared_ptr<SystemTimer>& timer, const std::string& name) : stopping_(false), timer_(timer), name_(name) {}

  void handle_event() final {
    {
      std::stringstream ss;
      ss << "Timer Handler " << name_ << std::endl;
      std::cout << ss.str() << std::flush;
    }
    if (!stopping_) {
      auto timer = timer_.lock();
      if (timer) {
        timer->expires_after(std::chrono::seconds(2));
        timer->async_wait(std::dynamic_pointer_cast<EventProxy>(shared_from_this()));
      }
    }
  }

private:
  bool stopping_;
  std::weak_ptr<SystemTimer> timer_;
  std::string name_;
};

void run_timer_test_suite(std::shared_ptr<EventDispatcher> dispatcher) {

  auto timer = dispatcher->get_timer();

  timer->expires_after(std::chrono::seconds(3));
  std::cout << "timer wait for 1" << std::endl;
  auto cv_status = timer->wait_for(std::chrono::seconds(1));
  std::cout << "timer wait for 1" << std::endl;
  cv_status = timer->wait_for(std::chrono::seconds(1));
  std::cout << "timer final wait" << std::endl;
  timer->wait();
  std::cout << "timer wait complete" << std::endl;

  timer->expires_after(std::chrono::seconds(3));

  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      std::cout << "Timer Handler #1\n" << std::flush;
    });
  });

  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      std::cout << "Timer Handler #2\n" << std::flush;
    });
  });

  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      std::cout << "Timer Handler #3\n" << std::flush;
    });
  });

  ACE_OS::sleep(4);

  // repeating timer
  auto timer2 = dispatcher->get_timer();

  timer->expires_after(std::chrono::seconds(1));
  timer2->expires_after(std::chrono::seconds(2));

  std::shared_ptr<timer_func_obj> f1(new timer_func_obj(timer, "Alpha"));
  std::shared_ptr<timer_func_obj> f2(new timer_func_obj(timer2, "Beta"));

  timer->async_wait(std::dynamic_pointer_cast<EventProxy>(f1));
  timer2->async_wait(std::dynamic_pointer_cast<EventProxy>(f2));

  ACE_OS::sleep(7);

  f1->stop();
  f2->stop();
}

void run_asio_tests() {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_dispatcher_test_suite(dispatcher);
}

void run_proactor_tests() {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_dispatcher_test_suite(dispatcher);

  //dispatcher.reset();
  //ACE_OS::sleep(1);
  //dispatcher = std::make_shared<ProactorEventDispatcher>();
  //run_timer_test_suite(dispatcher);
}

void run_reactor_tests() {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_dispatcher_test_suite(dispatcher);
}

/* main */

int main(int, char**) {

  ACE::init();

  // to turn off ace proactor chatter
  ACE_Log_Category::ace_lib().priority_mask(0);

  std::cout << "Begin AsioEventDispatcher Test" << std::endl;
  run_asio_tests();
  std::cout << "End AsioEventDispatcher Test" << std::endl;

  std::cout << "- - - - - - - - - - - - - - - -" << std::endl;

  std::cout << "Begin ProactorEventDispatcher Test" << std::endl;
  run_proactor_tests();
  std::cout << "End ProactorEventDispatcher Test" << std::endl;

  std::cout << "- - - - - - - - - - - - - - - -" << std::endl;

  std::cout << "Begin ReactorEventDispatcher Test" << std::endl;
  run_reactor_tests();
  std::cout << "End ReactorEventDispatcher Test" << std::endl;

  ACE::fini();

  return 0;
}
