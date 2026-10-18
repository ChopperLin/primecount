///
/// @file  S2Central.hpp
/// @brief Select the U = y central bands of Deleglise-Rivat's hard
///        special leaves and precompute their Moebius boundaries.
///
/// Copyright (C) 2026 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef S2_CENTRAL_HPP
#define S2_CENTRAL_HPP

#include "../gourdon/DCentral.hpp"

#include <imath.hpp>
#include <min.hpp>
#include <phi_vector.hpp>
#include <PiTable.hpp>
#include <Vector.hpp>

#include <stdint.h>

namespace primecount {

struct S2CentralRow
{
  int64_t lower = 0;
  int64_t middle = 0;
  int64_t mertens = 0;
};

/// Require p^2 <= floor(X/y) <= floor(sqrt(X)) < y < p^3.
/// Division avoids forming p^3 or y^2, and caps X/y before narrowing.
///
template <typename T>
bool s2_central_bounds(T xp, int64_t p, int64_t y,
                       int64_t& lower, int64_t& middle)
{
  if (p > y / p || p * p <= y / p)
    return false;

  int64_t limit = min(xp / y, y);
  if (limit < p * p || limit >= y)
    return false;

  lower = limit;
  middle = isqrt(xp);
  return true;
}

template <typename T>
int64_t s2_central_max_prime(T x, int64_t y, int64_t z)
{
  int64_t p = min(iroot<3>(z), isqrt(y));
  int64_t lower, middle;
  if (p > 0 && s2_central_bounds(x / p, p, y, lower, middle))
    return y;
  return min(y, z / isqrt(y));
}

template <typename T, typename Primes>
Vector<S2CentralRow> s2_central_rows(T x, int64_t y, int64_t c,
                                     const Primes& primes, const PiTable& pi)
{
  Vector<S2CentralRow> rows(pi[isqrt(y)] + 1);
  int64_t max_p = min(iroot<3>(x / y), isqrt(y));
  int64_t max_b = pi[max_p];
  int64_t last = 0;
  for (int64_t b = c + 1; b <= max_b; b++)
    if (s2_central_bounds(x / primes[b], primes[b], y,
                          rows[b].lower, rows[b].middle))
      last = b;

  if (last > 0)
  {
    auto phi = phi_vector(y, last, primes, pi);
    for (int64_t b = c + 1; b <= last; b++)
      if (rows[b].middle)
        rows[b].mertens = phi[b] - central_correction(y, primes[b], b, pi);
  }
  return rows;
}

} // namespace

#endif
