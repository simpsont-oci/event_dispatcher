#include "AsioEventDispatcher.h"
#include "ProactorEventDispatcher.h"
#include "ReactorEventDispatcher.h"

#include "ace/Init_ACE.h"

#include "gtest/gtest.h"

/* Call State */

class call_state {
public:
  call_state() : mutex_(), cv_(), count_(0) {
  }

  void reset() {
    std::unique_lock<std::mutex> lock(mutex_);
    count_ = 0;
    cv_.notify_all();
  }

  void call() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_all();
  }

  void call() const {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_all();
  }

  size_t count() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return count_;
  }


  template <typename Rep, typename Period>
  bool wait_expected(const std::chrono::duration<Rep, Period>& rel_time, size_t expected) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, rel_time, [&]() -> bool { return count_ == expected; });
  }

private:

  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  mutable size_t count_;
};

typedef std::shared_ptr<call_state> call_state_ptr;

/* Test Handlers */

call_state test_handler_void_call_state;
void test_handler_void() {
  test_handler_void_call_state.call();
  //std::stringstream ss;
  //ss << "test_handler_void" << std::endl;
  //std::cout << ss.str() << std::flush;
}

call_state test_handler_std_func_call_state;
void test_handler_std_func() {
  test_handler_std_func_call_state.call();
  //std::stringstream ss;
  //ss << "test_handler_std_func" << std::endl;
  //std::cout << ss.str() << std::flush;
}

call_state test_handler_std_bind_call_state;
void test_handler_std_bind(unsigned int a) {
  test_handler_std_bind_call_state.call();
  //std::stringstream ss;
  //ss << "test_handler_std_bind a=" << a << std::endl;
  //std::cout << ss.str() << std::flush;
}

struct test_handler_fobj {
  test_handler_fobj() : call_state_(new call_state()), const_call_state_(new call_state()) {}

  void operator()() {
    call_state_->call();
    //std::stringstream ss;
    //ss << "test_handler_fobj" << std::endl;
    //std::cout << ss.str() << std::flush;
  }
  void operator()() const {
    const_call_state_->call();
    //std::stringstream ss;
    //ss << "test_handler_fobj (const)" << std::endl;
    //std::cout << ss.str() << std::flush;
  }

  call_state_ptr call_state_;
  call_state_ptr const_call_state_;
};

struct test_recursive_handler_fobj {
  test_recursive_handler_fobj() = delete;
  test_recursive_handler_fobj(const test_recursive_handler_fobj&) = default;
  test_recursive_handler_fobj(test_recursive_handler_fobj&&) = delete;
  test_recursive_handler_fobj operator=(const test_recursive_handler_fobj&) = delete;

  test_recursive_handler_fobj(EventDispatcher& dispatcher, const std::string& name,
    size_t height, size_t leaves = 1u,
    const call_state_ptr& cs = call_state_ptr(),
    const call_state_ptr& ccs = call_state_ptr())
  : dispatcher_(dispatcher)
  , name_(name)
  , height_(height)
  , leaves_(leaves)
  , call_state_(cs)
  , const_call_state_(ccs)
  {
    //std::stringstream ss;
    //ss << "test_recursive_handler_fobj::test_recursive_handler_fobj() : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;

    if (!call_state_) {
      call_state_.reset(new call_state());
    }

    if (!const_call_state_) {
      const_call_state_.reset(new call_state());
    }

    if (height_ > 1) {
      handler_.reset(new test_recursive_handler_fobj(dispatcher_, name_, height_ - 1, leaves_, call_state_, const_call_state_));
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
    call_state_->call();
    if (height_ > 1) {
      for (size_t i = 0; i < leaves_; ++i) {
        dispatcher_.dispatch(*handler_);
      }
    }
  }

  void operator()() const {
    //std::stringstream ss;
    //ss << "test_recursive_handler_fobj (const) : " << name_ << " : " << height_ << " x " << leaves_ << std::endl;
    //std::cout << ss.str() << std::flush;

    const_call_state_->call();
    if (height_ > 1) {
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
  call_state_ptr call_state_;
  call_state_ptr const_call_state_;
};

void run_void_handler_test(std::shared_ptr<EventDispatcher> dispatcher, uint32_t delay) {
  test_handler_void_call_state.reset();
  dispatcher->dispatch(test_handler_void);

  if (delay != 0) {
    auto dsec = std::chrono::seconds(delay);
    ASSERT_TRUE(test_handler_void_call_state.wait_expected(dsec, 1)) << "actual count = " << test_handler_void_call_state.count();
  }
}

void run_fobj_handler_test(std::shared_ptr<EventDispatcher> dispatcher, uint32_t delay) {
  test_handler_fobj fobj;
  dispatcher->dispatch(fobj);

  const test_handler_fobj cfobj;
  dispatcher->dispatch(cfobj);

  dispatcher->dispatch(test_handler_fobj());

  if (delay != 0) {
    auto dsec = std::chrono::seconds(delay);
    ASSERT_TRUE(fobj.const_call_state_->wait_expected(dsec, 1)) << "actual count = " << fobj.call_state_->count();
    ASSERT_TRUE(cfobj.const_call_state_->wait_expected(dsec, 1)) << "actual count = " << cfobj.const_call_state_->count();
  }
}

/* run_combo_test */

void run_combo_test(std::shared_ptr<EventDispatcher> dispatcher, uint32_t delay) {

  test_handler_void_call_state.reset();
  test_handler_std_func_call_state.reset();
  test_handler_std_bind_call_state.reset();

  // void()
  dispatcher->dispatch(test_handler_void);

  // custom function object
  test_handler_fobj fobj;
  dispatcher->dispatch(fobj);

  const test_handler_fobj cfobj;
  dispatcher->dispatch(cfobj);

  dispatcher->dispatch(test_handler_fobj());

  // lambda
  std::shared_ptr<call_state> lambda_call_state(new call_state());
  dispatcher->dispatch([lambda_call_state] () {
    lambda_call_state->call();
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
  test_handler_fobj cp_fobj;
  std::shared_ptr<EventProxy> proxy(new CopyProxy<test_handler_fobj>(cp_fobj));
  dispatcher->dispatch(proxy);

  dispatcher->dispatch(std::shared_ptr<EventProxy>(new CopyProxy<test_handler_fobj>(cp_fobj)));

  // recursive calls
  test_recursive_handler_fobj rfobj(*dispatcher, "depth", 6);
  dispatcher->dispatch(rfobj);

  test_recursive_handler_fobj rfobj2(*dispatcher, "breadth", 3, 2);
  dispatcher->dispatch(rfobj2);

  if (delay != 0) {
    auto dsec = std::chrono::seconds(delay);
    ASSERT_TRUE(test_handler_void_call_state.wait_expected(dsec, 1)) << "actual count = " << test_handler_void_call_state.count();
    ASSERT_TRUE(fobj.const_call_state_->wait_expected(dsec, 1)) << "actual count = " << fobj.call_state_->count();
    ASSERT_TRUE(cfobj.const_call_state_->wait_expected(dsec, 1)) << "actual count = " << cfobj.const_call_state_->count();
    ASSERT_TRUE(lambda_call_state->wait_expected(dsec, 1)) << "actual count = " << lambda_call_state->count();
    ASSERT_TRUE(test_handler_std_func_call_state.wait_expected(dsec, 2)) << "actual count = " << test_handler_std_func_call_state.count();
    ASSERT_TRUE(test_handler_std_bind_call_state.wait_expected(dsec, 2)) << "actual count = " << test_handler_std_bind_call_state.count();
    ASSERT_TRUE(cp_fobj.const_call_state_->wait_expected(dsec, 2)) << "actual count = " << cp_fobj.const_call_state_->count();
    ASSERT_TRUE(rfobj.const_call_state_->wait_expected(dsec, 6)) << "actual count = " << rfobj.const_call_state_->count(); // 6
    ASSERT_TRUE(rfobj2.const_call_state_->wait_expected(dsec, 7)) << "actual count = " << rfobj2.const_call_state_->count(); // 4 + 2 + 1
  }
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

class timer_func_obj : public EventProxy, public call_state, public std::enable_shared_from_this<timer_func_obj> {
public:
  timer_func_obj() = delete;
  timer_func_obj(const timer_func_obj&) = delete;
  timer_func_obj& operator=(const timer_func_obj&) = delete;

  void stop() {
    stopping_ = true;
  }

  timer_func_obj(std::shared_ptr<SystemTimer>& timer, const std::string& name, const SystemTimer::Duration& period) : stopping_(false), timer_(timer), name_(name), period_(period) {}

  void handle_event() final {
    {
      //std::stringstream ss;
      //ss << "Timer Handler " << name_ << std::endl;
      //std::cout << ss.str() << std::flush;
    }
    call();
    if (!stopping_) {
      auto timer = timer_.lock();
      if (timer) {
        timer->expires_after(period_);
        timer->async_wait(std::dynamic_pointer_cast<EventProxy>(shared_from_this()));
      }
    }
  }

private:
  bool stopping_;
  std::weak_ptr<SystemTimer> timer_;
  std::string name_;
  SystemTimer::Duration period_;
};

void run_combo_timer_test(std::shared_ptr<EventDispatcher> dispatcher) {

  auto timer = dispatcher->get_timer();

  timer->expires_after(std::chrono::seconds(1));

  auto start1 = std::chrono::system_clock::now();
  auto cv_status1 = timer->wait_for(std::chrono::milliseconds(200));
  auto stop1 = std::chrono::system_clock::now();
  ASSERT_EQ(cv_status1, std::cv_status::timeout);
  auto delta1 = stop1 - start1;
  ASSERT_TRUE(delta1 > std::chrono::milliseconds(180));
  ASSERT_TRUE(delta1 < std::chrono::milliseconds(220));

  auto start2 = std::chrono::system_clock::now();
  auto cv_status2 = timer->wait_for(std::chrono::milliseconds(300));
  auto stop2 = std::chrono::system_clock::now();
  ASSERT_EQ(cv_status2, std::cv_status::timeout);
  auto delta2 = stop2 - start2;
  ASSERT_TRUE(delta2 > std::chrono::milliseconds(270));
  ASSERT_TRUE(delta2 < std::chrono::milliseconds(330));

  timer->wait();

  timer->expires_after(std::chrono::milliseconds(350));

  call_state cs;
  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      //std::cout << "cs.call() #1\n" << std::flush;
      cs.call();
    });
  });

  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      //std::cout << "cs.call() #2\n" << std::flush;
      cs.call();
    });
  });

  dispatcher->dispatch([&](){
    timer->async_wait([&](){
      //std::cout << "cs.call() #3\n" << std::flush;
      cs.call();
    });
  });

  auto start3 = std::chrono::system_clock::now();

  auto dsec = std::chrono::seconds(3);
  ASSERT_TRUE(cs.wait_expected(dsec, 3)) << "actual count = " << cs.count();
  auto stop3 = std::chrono::system_clock::now();
  auto delta3 = stop3 - start3;
  ASSERT_TRUE(delta3 > std::chrono::milliseconds(315));
  ASSERT_TRUE(delta3 < std::chrono::milliseconds(385));

  // repeating timer
  auto timer2 = dispatcher->get_timer();
  auto timer3 = dispatcher->get_timer();

  timer->expires_after(std::chrono::milliseconds(300));
  timer2->expires_after(std::chrono::milliseconds(500));
  timer3->expires_after(std::chrono::milliseconds(700));

  std::shared_ptr<timer_func_obj> f1(new timer_func_obj(timer, "Alpha", std::chrono::milliseconds(300)));
  std::shared_ptr<timer_func_obj> f2(new timer_func_obj(timer2, "Beta", std::chrono::milliseconds(500)));
  std::shared_ptr<timer_func_obj> f3(new timer_func_obj(timer3, "Gamma", std::chrono::milliseconds(700)));

  timer->async_wait(std::dynamic_pointer_cast<EventProxy>(f1));
  timer2->async_wait(std::dynamic_pointer_cast<EventProxy>(f2));
  timer3->async_wait(std::dynamic_pointer_cast<EventProxy>(f3));

  std::this_thread::sleep_for(std::chrono::milliseconds(4400));

  f1->stop();
  f2->stop();
  f3->stop();

  ASSERT_EQ(f1->count(), 14);
  ASSERT_EQ(f2->count(), 8);
  ASSERT_EQ(f3->count(), 6);
}

/* AceTest (Google Test Fixture for init / fini of ACE) */

class AceTest : public testing::Test {
public:
  struct InitHelper {
    InitHelper() {
      ACE::init();
      ACE_Log_Category::ace_lib().priority_mask(0);
    }
    ~InitHelper() {
      ACE::fini();
    }
  };

  AceTest() {
    if (!init_helper_) {
      init_helper_.reset(new InitHelper());
    }
  }

private:
  static std::shared_ptr<InitHelper> init_helper_;
};

std::shared_ptr<AceTest::InitHelper> AceTest::init_helper_ = std::shared_ptr<AceTest::InitHelper>();

/* Asio */

TEST(EventDispatcherTests, AsioVoid) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_void_handler_test(dispatcher, 10);
}

TEST(EventDispatcherTests, AsioFunObj) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_fobj_handler_test(dispatcher, 10);
}

TEST(EventDispatcherTests, AsioCombo) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_combo_test(dispatcher, 10);
}

TEST(EventDispatcherTests, AsioCleanShutdown) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_combo_test(dispatcher, 0);
}

TEST(EventDispatcherTests, AsioSpeed) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_speed_test(dispatcher);
}

TEST(EventDispatcherTests, AsioTimer) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<AsioEventDispatcher>();
  run_combo_timer_test(dispatcher);
}

/* Proactor */

TEST_F(AceTest, ProactorVoid) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_void_handler_test(dispatcher, 10);
}

TEST_F(AceTest, ProactorFunObj) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_fobj_handler_test(dispatcher, 10);
}

TEST_F(AceTest, ProactorCombo) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_combo_test(dispatcher, 10);
}

TEST_F(AceTest, ProactorCleanShutdown) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_combo_test(dispatcher, 0);
}

TEST_F(AceTest, ProactorSpeed) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_speed_test(dispatcher);
}

TEST_F(AceTest, ProactorTimer) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ProactorEventDispatcher>();
  run_combo_timer_test(dispatcher);
}

/* Reactor */

TEST_F(AceTest, ReactorVoid) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_void_handler_test(dispatcher, 10);
}

TEST_F(AceTest, ReactorFunObj) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_fobj_handler_test(dispatcher, 10);
}

TEST_F(AceTest, ReactorCombo) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_combo_test(dispatcher, 10);
}

TEST_F(AceTest, ReactorCleanShutdown) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_combo_test(dispatcher, 0);
}

TEST_F(AceTest, ReactorSpeed) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_speed_test(dispatcher);
}

TEST_F(AceTest, ReactorTimer) {
  std::shared_ptr<EventDispatcher> dispatcher = std::make_shared<ReactorEventDispatcher>();
  run_combo_timer_test(dispatcher);
}

