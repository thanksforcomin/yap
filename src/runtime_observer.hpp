#pragma once

#include <atomic>
#include <expected>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <variant>
#include <vector>
#include <unordered_map>

namespace observer {

  enum Errors { noSubscriber, vectorExhausted, dublicateObserver };

  template <typename T>
  using Result = std::expected<T, Errors>;

  struct SimpleCaster {
    template <typename T>
    constexpr operator T&();
    
    template <typename T>
    constexpr operator T&&();
  };

  template <typename T>
  concept Subscriber = requires(T t, SimpleCaster data) {
    { t.process(data) } -> std::same_as<void>;
  };

  template <Subscriber... Subscribers> class RuntimeObserver {
    using ObserverType = std::variant<std::weak_ptr<Subscribers>...>;

    std::vector<ObserverType> subscribers;
    std::shared_mutex mutex;

  public:
    RuntimeObserver();

    RuntimeObserver(const RuntimeObserver &) = delete;
    auto operator=(const RuntimeObserver &) -> RuntimeObserver & = delete;

    RuntimeObserver(RuntimeObserver &&) = default;
    auto operator=(RuntimeObserver &&) -> RuntimeObserver & = default;

    ~RuntimeObserver() = default;

    template <Subscriber T>
    auto subscribe(std::shared_ptr<T>& sub) -> Result<void>;

    template <Subscriber T>
    auto unsubscribe(std::shared_ptr<T>& sub) -> Result<void>;

    auto process(auto&& data) -> void;

  private:
    auto contains(auto &sub) -> bool;
  };
  
}

#include "runtime_observer.ipp"
