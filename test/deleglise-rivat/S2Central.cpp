///
/// @file  S2Central.cpp
/// @brief Check DR central bands, segment query order and full hard-leaf
///        sums against independent dense sieve and Moebius prefixes.
///
/// Copyright (C) 2026 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <S2Central.hpp>
#include <generate_primes.hpp>
#include <int128_t.hpp>
#include <PhiTiny.hpp>
#include <S.hpp>

#include <stdint.h>
#include <cstdlib>
#include <iostream>

using namespace primecount;

void check(bool ok, const char* message)
{
  if (!ok)
  {
    std::cerr << message << '\n';
    std::exit(1);
  }
}

template <typename T, typename Prime>
void check_row(T xp, int64_t p, int64_t y, int64_t size,
               const Vector<Prime>& primes, const PiTable& pi)
{
  int64_t lower, middle;
  check(s2_central_bounds(xp, p, y, lower, middle), "Expected a central band");
  int64_t limit = xp / (y / p + 1);
  auto dense_primes = generate_primes<uint32_t>(limit);
  auto lpf = generate_lpf(limit, dense_primes);
  auto mu = generate_moebius(limit, dense_primes);
  Vector<int64_t> phi(limit + 1);
  Vector<int64_t> mertens(limit + 1);
  phi[0] = mertens[0] = 0;
  for (int64_t n = 1; n <= limit; n++)
  {
    phi[n] = phi[n - 1] + (n == 1 || lpf[n] >= p);
    mertens[n] = mertens[n - 1] + (n == 1 || lpf[n] > p ? mu[n] : 0);
  }
  auto rows = s2_central_rows(xp * p, y, int64_t(8), primes, pi);
  check(rows[pi[p]].mertens == mertens[y], "Incorrect Moebius boundary precomputation");

  T expected = 0;
  for (int64_t m = y / p + 1; m <= y; m++)
    if (lpf[m] > p)
      expected -= T(mu[m]) * phi[int64_t(xp / m)];

  Vector<CentralEvent> events;
  SemiprimeEventBitmap sorter;
  T actual = 0;
  for (int64_t low = 0; low <= limit; low += size)
  {
    int64_t high = min(low + size, limit + 1);
    uint64_t previous = 0;
    auto count = [&](uint64_t position) -> uint64_t {
      check(position < uint64_t(high - low), "DR query escaped its segment");
      check(position >= previous, "DR queries are not monotonic");
      previous = position;
      return phi[low + position] - phi[low];
    };
    actual += central_y_segment(xp, p, pi[p], lower, middle, mertens[y],
                                low, high, phi[low], primes, pi,
                                count, events, sorter);
    for (int64_t m = lower; m > y / p; m--)
      if (lpf[m] > p && mu[m])
      {
        int64_t t = xp / m;
        if (low <= t && t < high)
        {
          int64_t value = phi[low] + count(t - low);
          actual -= T(mu[m]) * value;
        }
      }
  }
  check(actual == expected, "Reflected band plus remainder differs from the DR row");
}

void check_hard(int64_t x, int64_t y)
{
  int64_t z = x / y;
  int64_t c = PhiTiny::get_c(y);
  int64_t max_p = min(isqrt(z), y);
  auto primes = generate_primes<uint32_t>(max(y, z));
  auto lpf = generate_lpf(max(y, z), primes);
  auto mu = generate_moebius(max(y, z), primes);
  Vector<int64_t> phi(z + 1);
  int64_t expected = 0;
  for (int64_t b = c + 1; primes[b] <= max_p; b++)
  {
    int64_t p = primes[b];
    phi[0] = 0;
    for (int64_t n = 1; n <= z; n++)
      phi[n] = phi[n - 1] + (n == 1 || lpf[n] >= p);
    if (p <= isqrt(y))
    {
      for (int64_t m = y / p + 1; m <= y; m++)
        if (lpf[m] > p)
          expected -= mu[m] * phi[x / (p * m)];
    }
    else
      for (int64_t j = b + 1; primes[j] <= min(y, z / p); j++)
        expected += phi[x / (p * primes[j])];
  }

  for (int threads : {1, 4})
  {
    check(S2_hard(x, y, z, c, threads) == expected, "64-bit S2_hard differs from dense reference");
    #ifdef HAVE_INT128_T
      check(S2_hard(int128_t(x), y, z, c, threads) == expected, "128-bit S2_hard differs from dense reference");
    #endif
  }
  std::cout << "S2_hard(" << x << ", " << y << ") = " << expected << " OK\n";
}

int main()
{
  int64_t lower, middle;
  for (int64_t y : {600, 1000, 2500})
  {
    int64_t p = 23;
    int64_t start = p * p * y;
    check(!s2_central_bounds(start - 1, p, y, lower, middle), "Accepted L < p^2");
    check(s2_central_bounds(start, p, y, lower, middle), "Rejected L = p^2");
    check(s2_central_bounds(y * y - 1, p, y, lower, middle), "Rejected H = y-1");
    check(!s2_central_bounds(y * y, p, y, lower, middle), "Accepted H = y");
    for (int64_t x : {p * start - 1, p * start, p * start + 1,
                       p * y * y - 1, p * y * y, p * y * y + 1})
      check_hard(x, y);
  }
  for (int64_t y : {12166, 12167, 12168})
  {
    check(s2_central_bounds(int64_t(100000000), 23, y, lower, middle) == (y < 12167),
          "Incorrect y = p^3 boundary");
    check_hard(2300000000, y);
  }
  check_hard(100000000, 464);
  check_hard(100000000, 2500);

  PiTable pi(2500, 1);
  auto primes32 = pi.get_primes<uint32_t>(2500, 1);
  auto primes64 = pi.get_primes<int64_t>(2500, 1);
  check_row(int64_t(280370), 23, 530, 239, primes32, pi);
  for (int64_t p : {23, 29, 31})
    for (int64_t size : {1, 239, 240, 1680})
    {
      check_row(int64_t(100000000 / p), p, 2500, size, primes32, pi);
      check_row(int64_t(100000000 / p), p, 2500, size, primes64, pi);
      #ifdef HAVE_INT128_T
        check_row(int128_t(100000000 / p), p, 2500, size, primes32, pi);
        check_row(int128_t(100000000 / p), p, 2500, size, primes64, pi);
      #endif
    }

  #ifdef HAVE_INT128_T
    int128_t x = ipow<31>(int128_t(10));
    check(s2_central_bounds(x / 100003, 100003, 100000000000000, lower, middle),
          "Rejected a valid large 128-bit band");
    check(!s2_central_bounds(x / 50000017, 50000017, 3000000000000000, lower, middle),
          "Accepted a large row outside the central band");
    check(!s2_central_bounds(x, 23, 2500, lower, middle), "Narrowed an oversized quotient");
  #endif
  std::cout << "All DR central-band tests passed successfully!\n";
}
