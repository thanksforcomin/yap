#pragma once

#include <algorithm>

#include "runtime_observer.hpp"

namespace observer {
  template <Subscriber... Subscribers>
  template <Subscriber T>
  auto RuntimeObserver<Subscribers...>::subscribe(std::shared_ptr<T> &sub) -> Result<void> {
    std::unique_lock lock(mutex);

    if (contains(sub))
      return std::unexpected(dublicateObserver);

    subscribers.push_back(std::weak_ptr<T>(sub));
  }

  template <Subscriber... Subscribers>
  template <Subscriber T>
  auto RuntimeObserver<Subscribers...>::unsubscribe(std::shared_ptr<T> &sub) -> Result<void> {
    std::unique_lock lock(mutex);

    auto it = std::ranges::find_if(
        subscribers.begin(), subscribers.end(), [&](const auto &item) -> bool {
          return std::visit(
              [&](const auto &wp) {
                return !wp.expired() && wp.lock().get() == item.get();
              },
              item);
        });

    if (it != subscribers.end())
      subscribers.erase(it);
    else
      return std::unexpected(noSubscriber);
  }

  template <Subscriber... Subscribers>
  auto RuntimeObserver<Subscribers...>::process(auto &&data) -> void {
    std::for_each(subscribers.begin(), subscribers.end(), [&](auto& sub){
      std::visit([&](auto *ptr) { sub->process(data); }, sub);
    });
  };
  
  template <Subscriber... Subscribers>
  auto RuntimeObserver<Subscribers...>::contains(auto &sub) -> bool {
    auto it = std::ranges::find_if(
        subscribers.begin(), subscribers.end(), [&](const auto &item) -> bool {
          return std::visit(
              [&](const auto &wp) {
                return !wp.expired() && wp.lock().get() == item.get();
              },
              item);
        });

    if (it != subscribers.end())
      return true;
    else
      return false;
  }
  
}
