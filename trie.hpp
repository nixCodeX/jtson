#ifndef trie_hpp
#define trie_hpp

#include <array>
#include <limits>
#include <optional>
#include <vector>

#include "unique_ptr_constexpr.hpp"

template <typename T>
class trie {
  private:
    static constexpr auto factor = static_cast<std::size_t>(std::numeric_limits<unsigned char>::max()) + 1;

    // TODO could be std::optional once constexpr optional is available (DR to 20)
    unique_ptr_constexpr<T> here;
    std::array<unique_ptr_constexpr<trie>, factor> nexts;

  public:
    constexpr trie() = default;

    constexpr trie(trie const & that)
      : here{make_unique_constexpr<T>(that.here)}
      , nexts
        { [&] <std::size_t ...Is> (std::index_sequence<Is...>) {
            return std::array
              { [&]() -> unique_ptr_constexpr<trie> {
                  if (auto& next = std::get<Is>(that.nexts)) {
                    return make_unique_constexpr<trie>(*next);
                  } else {
                    return nullptr;
                  }
                }()...
              };
          }(std::make_index_sequence<factor>{})
        }
      {}

    constexpr trie(trie&&) = default;
    constexpr trie& operator=(trie&&) = default;

    class const_it {
      private:
        std::string key;
        std::vector<trie const *> path;

        void step() {
          auto next = std::find_if(path.back()->nexts.begin(), path.back()->nexts.end(), std::identity{});
          while (next == path.back()->nexts.end()) {
            auto last = key.back();
            key.pop_back();
            path.pop_back();

            if (path.empty()) {
              return;
            }

            next = std::find_if(path.back()->nexts.begin() + last + 1, path.back()->nexts.end(), std::identity{});
          }

          key.push_back(next - path.back()->nexts.begin());
          path.push_back(next->get());
        }

        void advance_to_active() {
          while (!path.empty() && !path.back()->here) {
            step();
          }
        };

        const_it(trie const * start) : path{start} {
          advance_to_active();
        }

        const_it() : path{} {}

      public:
        constexpr auto operator*() const {
          return std::pair<std::string, T const &>{key, *path.back()->here};
        }

        constexpr auto operator->() const {
          struct proxy {
            std::pair<std::string, T const &> value;
            constexpr auto operator->() const { return &value; }
          };
          return proxy{**this};
        }

        constexpr auto operator++() -> const_it& {
          step();
          advance_to_active();
          return *this;
        }

        friend constexpr auto operator==(const_it const & lhs, const_it const & rhs) -> bool {
          return (lhs.path.size() == rhs.path.size() && std::equal(lhs.path.begin(), lhs.path.end(), rhs.path.begin()));
        }

        friend class trie;
    };

    constexpr auto begin() const { return const_it{this}; }
    constexpr auto end()   const { return const_it{}; }

    constexpr void emplace(std::string_view key, auto&& ...args) {
      if (key.size() == 0) {
        here = make_unique_constexpr<T>(std::forward<decltype(args)>(args)...);
      } else {
        auto& next = nexts[static_cast<unsigned char>(key[0])];
        if (!next) {
          next = make_unique_constexpr<trie>();
        }
        next->emplace(key.substr(1), std::forward<decltype(args)>(args)...);
      }
    }

    constexpr auto get_if(std::string_view key) -> T* {
      if (key.size() == 0) {
        if (here) {
          return &*here;
        } else {
          return nullptr;
        }
      } else {
        if (auto& next = nexts[static_cast<unsigned char>(key[0])]) {
          return next->get_if(key.substr(1));
        } else {
          return nullptr;
        }
      }
    }

    constexpr auto get_if(std::string_view key) const -> T const * {
      if (key.size() == 0) {
        if (here) {
          return &*here;
        } else {
          return nullptr;
        }
      } else {
        if (auto& next = nexts[static_cast<unsigned char>(key[0])]) {
          return next->get_if(key.substr(1));
        } else {
          return nullptr;
        }
      }
    }
};

#endif