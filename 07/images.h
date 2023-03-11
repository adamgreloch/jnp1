#ifndef IMAGES_H
#define IMAGES_H

#include <cmath>
#include <functional>
#include <numbers>

#include "color.h"
#include "coordinate.h"
#include "functional.h"

using Fraction = double;

/**
 * Base_image jest nazywany dalej obrazem bazowym.
 */
template <typename T>
using Base_image = std::function<T(const Point)>;

/**
 * Region jest nazywany regionem lub wycinanką. Regiony służą do składania
 * obrazów właściwych (zob. poniżej), np. z użyciem funkcji cond.
 */
using Region = Base_image<bool>;

/**
 * Image, zwany też obrazem właściwym, jest kanonicznym przykładem obrazu
 * funkcyjnego.
 */
using Image = Base_image<Color>;

/**
 * Blend, nazywany dalej mieszanką, służy do mieszania kolorów w
 * proporcjach zadanych parametrem typu Fraction.
 */
using Blend = Base_image<Fraction>;

namespace Detail {

inline Color choose_color(bool b, Color f1, Color f2) { return b ? f1 : f2; }

inline Color blend_colors(const Fraction& fraction, const Color& this_way,
                          const Color& that_way) {
  return this_way.weighted_mean(that_way, fraction);
}

/**
 * Zwraca współrzędne kartezjańskie punktu.
 */
inline Point cartesian(const Point p) { return p.is_polar ? from_polar(p) : p; }

/**
 * Zwraca współrzędne biegunowe punktu.
 */
inline Point polar(const Point p) { return p.is_polar ? p : to_polar(p); }

/*
 * Zwraca funkcję rzutującą punkt p(rho, phi) na punkt p'(x,y) następująco:
 * przypiszmy za x odległość punktu p od środka układu współrzędnych (czyli
 * rho), zaś za y numer wycinka koła od 0 do n-1 w którym znajduje się kąt phi
 * pomnożony o współczynnik d.
 */
inline auto radial_map(double d, int n) {
  return [=](const Point p) -> Point {
    return {p.first,
            static_cast<int>(p.second / (2 * std::numbers::pi / n)) * d};
  };
}

/**
 * Zwraca funkcję zwracającą podłogę współrzędnych punktu po
 * podzieleniu przez d. Zakłada, że współrzędne są kartezjańskie.
 */
inline auto floor(double d) {
  return [=](const Point p) -> std::pair<int, int> {
    return {std::floor((p.first) / d), std::floor((p.second) / d)};
  };
}

/**
 * Zwraca funkcję obracającą punkt p o kąt phi.
 */
inline auto rotate_point(double phi) {
  return [=](const Point p) -> Point {
    return {cos(phi) * p.first - sin(phi) * p.second,
            sin(phi) * p.first + cos(phi) * p.second};
  };
}

/**
 * Zwraca funkcję wykonującą translację punktu p o wektor v.
 */
inline auto translate_point(Vector v) {
  return [=](const Point p) -> Point {
    return {p.first - v.first, p.second - v.second};
  };
}

/**
 * Zwraca funkcję skalującą punkt o współczynnik 1/s
 */
inline auto scale_point(double s) {
  return [=](const Point p) -> Point { return {p.first / s, p.second / s}; };
}
}  // namespace Detail

/**
 * Tworzy stały obraz bazowy
 */
template <typename T>
Base_image<T> constant(const T& t) {
  return [=]([[maybe_unused]] const Point p) { return t; };
}

/**
 * Obraca obraz bazowy o kąt phi (wyrażony w radianach).
 */
template <typename T>
Base_image<T> rotate(Base_image<T> image, double phi) {
  return compose(Detail::cartesian, Detail::rotate_point(-phi), image);
}

/**
 * Przesuwa obraz bazowy o wektor v.
 */
template <typename T>
Base_image<T> translate(Base_image<T> image, const Vector& v) {
  return compose(Detail::cartesian, Detail::translate_point(v), image);
}

/**
 * Skaluje obraz bazowy o współczynnik s. Przykładowo, dla s == 2 otrzymuje się
 * obraz bazowy dwukrotnie powiększony.
 */
template <typename T>
Base_image<T> scale(Base_image<T> image, double s) {
  return compose(Detail::cartesian, Detail::scale_point(s), image);
}

/**
 * Tworzy obraz bazowy koła o środku q, promieniu r oraz zwracanych wartościach
 * inner w kole i na obwodzie oraz outer poza nim.
 */
template <typename T>
Base_image<T> circle(Point const q, double r, const T& inner, const T& outer) {
  return compose(Detail::cartesian, [=](const Point p) {
    return distance(p, Detail::cartesian(q)) > r ? outer : inner;
  });
}

/**
 * Tworzy obraz bazowy szachownicy o szerokości kratki d oraz zwracanych
 * wartościach zadanych przez pozostałe dwa argumenty.
 */
template <typename T>
Base_image<T> checker(double d, const T& this_way, const T& that_way) {
  return compose(Detail::cartesian, Detail::floor(d),
                 [=](const std::pair<int, int> p) {
                   return (p.first + p.second) % 2 == 0 ? this_way : that_way;
                 });
}

/**
 * Tworzy obraz bazowy „szachownicy biegunowej”. Implementacja odwołuje się się
 * funkcji checker i przekazuje jej argument d. Zakłada parzystość argumentu n.
 */
template <typename T>
Base_image<T> polar_checker(double d, int n, const T& this_way,
                            const T& that_way) {
  return compose(Detail::polar, Detail::radial_map(d, n),
                 checker(d, this_way, that_way));
}

/**
 * Tworzy obraz bazowy koncentrycznych naprzemiennych pasów szerokości d o
 * środku w punkcie q.
 */
template <typename T>
Base_image<T> rings(const Point q, double d, const T& this_way,
                    const T& that_way) {
  return compose(Detail::cartesian, [=](const Point p) {
    return static_cast<int>(distance(p, Detail::cartesian(q)) / d) % 2
               ? that_way
               : this_way;
  });
}

/**
 * Tworzy obraz bazowy w postaci wycentrowanego paska this_way o szerokości d
 * otoczonego przez that_way.
 */
template <typename T>
Base_image<T> vertical_stripe(double d, const T& this_way, const T& that_way) {
  return compose(Detail::cartesian, [=](const Point p) {
    return -d <= 2 * p.first && 2 * p.first <= d ? this_way : that_way;
  });
}

/**
 * Korzysta z regionu (wycinanki), który wycina i klei dwa obrazy właściwe,
 * tworząc w ten sposób nowy. Dla punktów p, takich że region(p) == true, brany
 * jest obraz this_way, zaś dla pozostałych - that_way.
 */
[[maybe_unused]] static Image cond(const Region& region, const Image& this_way,
                  const Image& that_way) {
  return lift(Detail::choose_color, region, this_way, that_way);
}

/**
 * Korzysta z mieszanki, która wyznacza proporcje mieszania kolorów dwóch
 * obrazów właściwych, tworząc w ten sposób nowy. Dla każdego punktu p
 * argument blend zwraca parametr mieszania oznaczony jako w, który informuje,
 * ile w finalnym obrazie dla punktu p będzie koloru z obrazu this_way, a ile
 * z obrazu that_way (por. argument o nazwie w metody weighted_mean struktury
 * Color w pliku color.h oraz poniższe funkcje darken i lighten)
 */
[[maybe_unused]] static Image lerp(const Blend& blend, const Image& this_way,
                  const Image& that_way) {
  return lift(Detail::blend_colors, blend, this_way, that_way);
}

/**
 * Ściemnia obraz. Dla szczególnego przypadku blend tożsamościowo równego: 1
 * oznacza stworzenie czarnego obrazu, 0 oznacza kopię image.
 */
[[maybe_unused]] static Image darken(const Image& image, const Blend& blend) {
  return lerp(blend, image, constant(Colors::black));
}

/**
 * Rozjaśnia obraz. Dla szczególnego przypadku blend tożsamościowo równego: 1
 * oznacza stworzenie białego obrazu, 0 oznacza kopię image.
 */
[[maybe_unused]] static Image lighten(Image image, Blend blend) {
  return lerp(blend, image, constant(Colors::white));
}

#endif  // IMAGES_H
