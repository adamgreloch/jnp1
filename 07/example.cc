#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "bmp.h"
#include "color.h"
#include "coordinate.h"
#include "functional.h"
#include "images.h"

namespace Fibonacci {
Color simple_colors[16] = {
    {0,0,0},
    {0x80,0,0},
    {0,0x80,0},
    {0x80,0x80,0},
    {0,0,0x80},
    {0x80,0,0x80},
    {0,0x80,0x80},
    {0xc0,0xc0,0xc0},
    {0x80,0x80,0x80},
    {0xff,0,0},
    {0,0xff,0},
    {0xff,0xff,0},
    {0,0,0xff},
    {0xff,0,0xff},
    {0,0xff,0xff},
    {0xff,0xff,0xff}
};

Image gradient = [](auto q){
  const Point p = q.is_polar ? from_polar(q) : q;
  const double v = exp(-0.01 * distance(p, {0,-100,false}));
  auto c = static_cast<unsigned char>(255.0 * v);
  return Color{c, c, 0};
};

double visor_dist(Point p) {
  double x = p.first - 30.7;
  double y = p.second - 50.2;
  return x*x*x*x + 8.0*y*y*y*y - 1000000.0;
}

double body_dist(Point p) {
  double x = p.first;
  double y = p.second;
  return 8.0*x*x*x*x + y*y*y*y - 50000000.0;
}

double legs(Point p) {
  double x = p.first;
  return -0.0001 * x * x * x * x -30 - p.second;
}

Image amongus(int i) {
  return [=](auto q) {
    const Point p = q.is_polar ? from_polar(q) : q;
    const double epsilon = 4500.0;
    const double epsilon2 = 2;
    const Color border{10,10,10};
    const Color visor{0xe0, 0xd0, 0x90};
    const double vd = visor_dist(p);
    const double bd = body_dist(p);
    const double l = legs(p);
    return vd < -1.0 * epsilon ? visor : (
        vd < epsilon ? border : (
            bd > epsilon ? gradient(p) : (
                l > epsilon2 ? gradient(p) : (
                    l > -1*epsilon2 ? border : (
                        bd > -1*epsilon ? border : simple_colors[i]
                    )
                )
            )
        )
    );
  };
}

Region amongus_sil = [](auto q) {
  const Point p = q.is_polar ? from_polar(q) : q;
  const double epsilon = 4500.0;
  const double epsilon2 = 2;
  const double vd = visor_dist(p);
  const double bd = body_dist(p);
  const double l = legs(p);
  return vd < -1.0 * epsilon || vd < epsilon || (bd <= epsilon && l <= epsilon2);
};

Vector shift(int i) {
  return {-175 + 50 * ((i-1) % 8), i <= 8 ? 50 : -50};
}

Image amongi(int n) {
  Region scaled_sil = scale(amongus_sil, 0.39);
  return [=](auto p) {
    const Vector s = shift(n);
    const Image scaled_amongus = scale(amongus(n-1), 0.39);
    return n==0 ? gradient(p) : (cond(translate(scaled_sil, s), translate(scaled_amongus, s), amongi(n-1)))(p);
  };
}

void test() {
  const uint32_t width = 400;
  const uint32_t height = 300;
  create_BMP("fib_hex_polar_checker.bmp",
             width,
             height,
             polar_checker(10, 6, simple_colors[15], simple_colors[10]));
  create_BMP("fib_correct_polar_coordinates.bmp",
             width,
             height,
             [](auto p){
               Point const q = p.is_polar ? p : to_polar(p);
               return rotate(vertical_stripe(10, simple_colors[1], simple_colors[2]), 0.7)(q);
             });
  create_BMP("fib_darken_lighten.bmp",
             width,
             height,
             [](auto p){
               return darken(lighten(vertical_stripe(21.37, simple_colors[3], simple_colors[6]), checker(17, 0.3, 0.9)),
                             checker(27, 0.1, 0.8))(p);
             });
  create_BMP("fib_translate_and_rotate.bmp",
             width,
             height,
             rotate(translate(amongus(4), {-20,40}), 0.2 * M_PI));
  create_BMP("fib_many_region_translate_and_scale.bmp",
             width,
             225,
             amongi(16));
  create_BMP("fib_all_at_once.bmp",
             width,
             height,
             cond(amongus_sil, polar_checker(10, 12, simple_colors[11], simple_colors[0]),
                  lighten(darken(
                      lerp(checker(7.3, 0.2, 0.8),
                           scale(rotate(translate(amongus(12), {10, 10}), 7.0), 3)
                          , rings({40, 1.73, true}, 22.222, simple_colors[1], simple_colors[14])),
                      vertical_stripe(30, 0.4, 0.1)), circle({70, 5.0, true}, 40, 0.7, 0.1))));
}
}

int
main()
{
  const uint32_t width = 400;
  const uint32_t height = 300;
  const Region rc = circle(Point(50., 100.), 10., true, false);
  const Image vs = vertical_stripe(100, Colors::Vermilion, Colors::blue);
  const Blend cb = constant<Fraction>(.42);

//  auto f = [](int x) { return x + 1; };
//  auto g = [](int x) { return x / 2; };
//  auto h = [](int x, int y) { return x == y; };
//
//  auto f1 = [](int x) { return std::make_pair(x, x+42); };
//  auto f2 = [](std::pair<int, int> p) { return p.first + p.second; };
//
//  std::cout << compose(f1, f2)(2) << std::endl;
//
//  std::cout << compose()(2) << std::endl;
//  std::cout << compose(f)(2) << std::endl;
//  std::cout << compose(f, f)(2) << std::endl;
//  std::cout << compose(g, f, f)(2) << std::endl;
//  std::cout << compose(f, f, g)(2) << std::endl;
//  std::cout << lift(h, f, f)(2) << std::endl;

  create_BMP("constant.bmp",
             width,
             height,
             constant(Colors::Vermilion));
  create_BMP("rotate.bmp",
             width,
             height,
             rotate(vs, M_PI / 4.));
  create_BMP("translate.bmp",
             width,
             height,
             translate(vs, Vector(100., 0.)));
  create_BMP("scale.bmp",
             width,
             height,
             scale(vs, 2.));
  create_BMP("circle.bmp",
             width,
             height,
             circle(Point(50., 100.), 10., Colors::Vermilion, Colors::blue));
  create_BMP("checker.bmp",
             width,
             height,
             checker(10., Colors::Vermilion, Colors::blue));
  create_BMP("polar_checker.bmp",
             width,
             height,
             polar_checker(10., 4, Colors::Vermilion, Colors::blue));
  create_BMP("rings.bmp",
             width,
             height,
             rings(Point(50., 100.), 10., Colors::Vermilion, Colors::blue));
  create_BMP("vertical_stripe.bmp",
             width,
             height,
             vs);
  create_BMP("cond.bmp",
             width,
             height,
             cond(rc, constant(Colors::Vermilion), constant(Colors::blue)));
  create_BMP("lerp.bmp",
             width,
             height,
             lerp(cb, constant(Colors::blue), constant(Colors::white)));
  create_BMP("dark_vs.bmp",
             width,
             height,
             darken(vs, cb));
  create_BMP("light_vs.bmp",
             width,
             height,
             lighten(vs, cb));
  create_BMP("checker2.bmp",
               40,
               40,
               checker(10., Colors::Vermilion, Colors::blue));

  assert(compose()(42) == 42);
  assert(compose([](auto x) {return x + 1;},
                 [](auto x) {return x * x;})(1) == 4);

  const auto h1 = [](auto a, auto b) {auto g = a * b; return g;};
  const auto h2 = [](auto a, auto b) {auto g = a + b; return g;};
  const auto f1 = [](auto p) {auto a = p; return a;};
  const auto f2 = [](auto p) {auto b = p; return b;};
  assert(lift(h1, f1, f2)(42) == 42 * 42);
  assert(lift(h2, f1, f2)(42) == 42 + 42);

  Fibonacci::test();
}
