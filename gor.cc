// Original source:
// https://godbolt.org/g/26viuZ (Gor Nishanov)

#include <cstddef>

#if __has_include(<coroutine>)
#include <coroutine>
#else
#include <experimental/coroutine>
namespace std {
    using std::experimental::suspend_always;
    using std::experimental::suspend_never;
    using std::experimental::noop_coroutine;
    using std::experimental::coroutine_handle;
}
#endif // __has_include(<coroutine>)

#include <exception>
#include <utility>
#include <optional>

template<class T>
struct generator {
    struct promise_type {
        T current_value_;
        std::optional<generator> innerCoroutine_;
        auto yield_value(T value) {
            this->current_value_ = value;
            return std::suspend_always{};
        }

        // std::suspend_always yield_value(generator<T> &&generator) {
        //   for(auto &&val : generator) {
        //     co_yield val;
        //   }
        // }
      std::suspend_always yield_value(generator innerCoroutine) {
        innerCoroutine_ = std::move(innerCoroutine);
        if(!innerCoroutine_->isResumable()) {
          innerCoroutine_ = {};
        } else {
          innerCoroutine_->resume();
          current_value_ = *innerCoroutine_;
        }
        return std::suspend_always{};
      }


        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        generator get_return_object() { return generator{this}; };
        void unhandled_exception() { std::terminate(); }
        void return_void() {}

    };

    using handle_t = std::coroutine_handle<promise_type>;

    struct iterator {
        handle_t coro_;
        bool done_;

        iterator() : done_(true) {}

        explicit iterator(handle_t coro, bool done)
            : coro_(coro), done_(done) {}

        iterator& operator++() {
            coro_.resume();
            done_ = coro_.done();
            return *this;
        }

        bool operator==(const iterator& rhs) const { return done_ == rhs.done_; }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
        const T& operator*() const { return coro_.promise().current_value_; }
        const T *operator->() const { return &*(*this); }
    };

    iterator begin() {
        coro_.resume();
        return iterator(coro_, coro_.done());
    }

    iterator end() { return iterator(); }

    generator(const generator&) = delete;
  generator& operator=(const generator&) = delete;

    generator(generator&& rhs) : coro_(std::exchange(rhs.coro_, nullptr)) {}

  generator& operator=(generator&& other) noexcept
  {
    std::swap(coro_, other.coro_);
    return *this;
  }

    ~generator() {
        if (coro_) {
            coro_.destroy();
        }
    }

  bool isResumable() {
    return coro_ && coro_.done();
  }

  [[nodiscard]] const T& get() const
  {
    return coro_.promise().current_value_;
  }

  [[nodiscard]] T& get()
  {
    return coro_.promise().current_value_;
  }

  bool resume() {
    if(coro_.promise().innerCoroutine_) {
      coro_.promise().innerCoroutine_->resume();
      if(coro_.promise().innerCoroutine_.isResumable()) {
          coro_.promise().current_value_ = std::move(coro_.promise().innerCoroutine_.get());
          return true;
      } else {
        coro_.promise().innerCoroutine = {};
        coro_();

        return !coro_.done();
      }
    } else {
      coro_();
      return !coro_.done();
    }
  }

private:
    explicit generator(promise_type *p)
        : coro_(handle_t::from_promise(*p)) {}

    handle_t coro_;
};

#include <iostream>

#include <string>
#include <string_view>
#include <dirent.h>

inline generator<struct dirent> directory_entries(std::string_view path) {
  DIR *dir_handle = opendir(path.data());

  while(auto dir_entry = readdir(dir_handle))
  {
    co_yield *dir_entry;
  }

  closedir(dir_handle);
}

inline generator<std::pair<struct dirent, std::string>> directory_entries_with_paths(std::string path) {
  for(auto const &entry : directory_entries(path)) {
    auto full_path = path + '/' + entry.d_name;
    co_yield std::make_pair(entry, full_path);
  }
}

inline generator<std::pair<struct dirent, std::string>> recursive_directory_entries(std::string path) {
  for(auto [entry, entry_path] : directory_entries_with_paths(path)) {
    if(entry.d_name == std::string(".") || entry.d_name == std::string("..")) {
      continue;
    }

    co_yield std::make_pair(entry, entry_path);
    if(entry.d_type == DT_DIR) {
      co_yield recursive_directory_entries(entry_path);
      // for(auto const &inner_entry : recursive_directory_entries(entry_path)) {
      //   co_yield inner_entry;
      // }
    }
  }
}


int main(int argc, char **argv) {
  auto path = ".";
  auto directory_generator = recursive_directory_entries(path);

  for (auto [entry, entry_path ]: directory_generator) {
    std::cout << entry_path << '\n';
  }
}
