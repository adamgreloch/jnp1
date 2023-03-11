#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <algorithm>

namespace Detail {

static auto identity = [](auto x) { return x; };

/**
 * Klasa implementująca compose umożliwiająca złożenia typu:
 * ((((a -> b) -> c) -> d) -> e) -> ... )
 */
template <typename... Fs>
  requires(sizeof...(Fs) > 0)
class compose_helper {
 private:
  std::tuple<Fs...> functions;
  static constexpr std::size_t comps = sizeof...(Fs);

 public:
  compose_helper(Fs&&... fs) : functions(std::forward<Fs>(fs)...){};

  /**
   * Wywołuje złożenie funkcji fs... z argumentem t.
   */
  template <typename T>
  auto operator()(T&& t) const {
    return apply(std::integral_constant<std::size_t, 0>{}, std::forward<T>(t));
  }

 private:
  /**
   * Wykonuje krok złożenia N-tej funkcji ze złożeniem N-1 poprzednich funkcji
   * w ciągu.
   */
  template <std::size_t N, typename T>
  [[maybe_unused]] auto apply(std::integral_constant<std::size_t, N>, T&& t)
          const {
    return apply(std::integral_constant<std::size_t, N + 1>{},
                 std::get<N>(functions)(std::forward<T>(t)));
  }

  /**
   * Wykonuje złożenie ostatniej funkcji ze złożeniem wszystkich
   * poprzednich funkcji z ciągu.
   */
  template <typename T>
  auto apply(std::integral_constant<std::size_t, comps - 1>, T&& t) const {
    return std::get<comps - 1>(functions)(std::forward<T>(t));
  }
};
}  // namespace Detail

/**
 * Realizuje złożenie funkcji w następujący sposób: compose()(x) ==
 * identyczność(x); compose(f)(x) == f(x); compose(f, g)(x) == g(f(x)) itd.
 */
template <typename... Fs>
  requires(sizeof...(Fs) > 0)
[[maybe_unused]] auto compose(Fs... fs) {
  return Detail::compose_helper<Fs...>(std::forward<Fs>(fs)...);
}

template <typename... Fs>
  requires(sizeof...(Fs) == 0)
auto compose() {
  return Detail::identity;
}

/**
 * Realizuje "pointwise lifting".
 */
template <typename H, typename... Fs>
[[maybe_unused]] auto lift(const H& h, const Fs&... fs) {
  return [=](auto x) { return h(fs(x)...); };
}

template <typename H>
auto lift(const H& h) {
  return h;
}

#endif  // FUNCTIONAL_H
