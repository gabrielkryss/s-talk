#include "Tracy.hpp"
#include "TracyLock.hpp"

#include <chrono>
#include <thread>

TracyLockableN(std::mutex, demoLock, "demoLock");
std::condition_variable_any cv;
bool ready = false;

void worker(int id) {
  tracy::SetThreadName(("worker-" + std::to_string(id)).c_str());
  for (;;) {
    // wait for signal
    std::unique_lock<LockableBase(std::mutex)> lk(demoLock);
    ZoneScopedN("wait cv");
    cv.wait(lk, [] { return ready; });

    // acquire protected section and mark it for Tracy
    {
      std::lock_guard<LockableBase(std::mutex)> g(demoLock);
      LockMark(demoLock); // <-- this creates the held-location event
      ZoneScopedN("critical");
      std::this_thread::sleep_for(
          std::chrono::milliseconds(40)); // hold the lock
    }

    // short rest, then loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main() {
  tracy::SetThreadName("main");
  std::thread t1(worker, 1);
  std::thread t2(worker, 2);

  // Let threads spin up, then periodically wake them so they contend
  for (int i = 0; i < 20; ++i) {
    {
      std::lock_guard<LockableBase(std::mutex)> g(demoLock);
      ready = true;
    }
    cv.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    {
      std::lock_guard<LockableBase(std::mutex)> g(demoLock);
      ready = false;
    }
  }

  // join (in a real repro you might detach; we join to exit cleanly)
  t1.detach();
  t2.detach();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
