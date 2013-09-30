// Multiprecision integer arithmetic for exact geometric predicates
#pragma once

#include <othercore/exact/config.h>
#include <othercore/array/RawArray.h>
#include <othercore/math/One.h>
#include <othercore/math/integer_log.h>
#include <othercore/utility/move.h>
#include <boost/detail/endian.hpp>
#include <gmp.h>
namespace other {

struct Uninit {};
static const Uninit uninit = Uninit();

// A fixed width 2's complement integer compatible with GMP's low level interface.
// See http://gmplib.org/manual/Low_002dlevel-Functions.html#Low_002dlevel-Functions.
// Exact<d> holds a signed integer with exactly d*sizeof(Quantized) bytes, suitable
// for representing the values of polynomial predicates of degree d.
template<int d> struct Exact {
  BOOST_STATIC_ASSERT(d>=1);
  static const int degree = d;
  static const int ratio = sizeof(Quantized)/sizeof(mp_limb_t);
  BOOST_STATIC_ASSERT(sizeof(Quantized)==ratio*sizeof(mp_limb_t)); // Ensure limb counts are always integral
  static const int limbs = d*ratio;

  // 2's complement, little endian array of GMP limbs
  mp_limb_t n[limbs];

  Exact() {
    memset(n,0,sizeof(n));
  }

  explicit Exact(Uninit) {}

  explicit Exact(const ExactInt x) {
    BOOST_STATIC_ASSERT(d==1 && sizeof(x)==sizeof(n) && limbs<=2);
    memcpy(n,&x,sizeof(x));
#ifdef BOOST_BIG_ENDIAN
    if (limbs==2) // Convert from big endian limb order to little endian
      swap(n[0],n[1]);
#endif
  }

  template<int smaller_d> explicit Exact(const Exact<smaller_d>& rhs) {
    static_assert(d > smaller_d, "Can only assign if increasing precision"); // should fall back to default operator for a == b
    memcpy(n,&rhs.n,sizeof(rhs.n));
    memset(n + smaller_d, 0, (d - smaller_d)*sizeof(n[0]));
  }
};

template<int d> static inline bool is_negative(const Exact<d>& x) {
  return (mp_limb_signed_t(x.n[x.limbs-1])<0);
}

template<int d> static inline bool is_nonzero(const Exact<d>& x) {
  for (int i=0;i<x.limbs;i++)
    if (x.n[i])
      return true;
  return false;
}

template<int d> static inline int sign(const Exact<d>& x) {
  if (is_negative(x))
    return -1;
  return is_nonzero(x);
}

template<int d> static inline bool operator==(const Exact<d>& lhs, const Exact<d>& rhs) {
  if(Exact<d>::limbs == 1)
    return lhs.n[0] == rhs.n[0];
  else
    return mpn_cmp(lhs.n, rhs.n, Exact<d>::limbs) == 0;
}

template<int d> static inline bool operator<(const Exact<d>& lhs, const Exact<d>& rhs) {
  return sign(lhs - rhs) < 0;
}
template<int d> static inline bool operator<=(const Exact<d>& lhs, const Exact<d>& rhs) {
  return sign(lhs - rhs) <= 0;
}
template<int d> static inline bool operator>=(const Exact<d>& lhs, const Exact<d>& rhs) {
  return sign(lhs - rhs) >= 0;
}
template<int d> static inline bool operator>(const Exact<d>& lhs, const Exact<d>& rhs) {
  return sign(lhs - rhs) > 0;
}

template<int a> OTHER_PURE static inline Exact<a> operator+(const Exact<a> x, const Exact<a> y) {
  Exact<a> r(uninit);
  if (r.limbs==1)
    r.n[0] = x.n[0] + y.n[0];
  else
    mpn_add_n(r.n,x.n,y.n,r.limbs);
  return r;
}

template<int a> static inline void operator+=(Exact<a>& x, const Exact<a>& y) {
  if (x.limbs==1)
    x.n[0] += y.n[0];
  else
    mpn_add_n(x.n,x.n,y.n,x.limbs);
}

template<int a> OTHER_PURE static inline Exact<a> operator-(const Exact<a> x, const Exact<a> y) {
  Exact<a> r(uninit);
  if (r.limbs==1)
    r.n[0] = x.n[0] - y.n[0];
  else
    mpn_sub_n(r.n,x.n,y.n,r.limbs);
  return r;
}

template<int a,int b> OTHER_PURE static inline Exact<a+b> operator*(const Exact<a> x, const Exact<b> y) {
  // Perform multiplication as if inputs were unsigned
  Exact<a+b> r(uninit);
  if (a>=b)
    mpn_mul(r.n,x.n,x.limbs,y.n,y.limbs);
  else
    mpn_mul(r.n,y.n,y.limbs,x.n,x.limbs);
  // Correct for negative numbers
  if (is_negative(x)) {
    if (y.limbs==1)
      r.n[x.limbs] -= y.n[0];
    else
      mpn_sub_n(r.n+x.limbs,r.n+x.limbs,y.n,y.limbs);
  }
  if (is_negative(y)) {
    if (x.limbs==1)
      r.n[y.limbs] -= x.n[0];
    else
      mpn_sub_n(r.n+y.limbs,r.n+y.limbs,x.n,x.limbs);
  }
  return r;
}

template<int a> OTHER_PURE static inline Exact<a> operator-(const Exact<a> x) {
  Exact<a> r(uninit);
  if (r.limbs==1)
    r.n[0] = mp_limb_t(-mp_limb_signed_t(x.n[0]));
  else
    mpn_neg(r.n,x.n,x.limbs);
  return r;
}

template<int a> OTHER_PURE static inline Exact<2*a> sqr(const Exact<a> x) {
  Exact<2*a> r(uninit);
  mp_limb_t nx[x.limbs];
  const bool negative = is_negative(x);
  if (negative)
    mpn_neg(nx,x.n,x.limbs);
  mpn_sqr(r.n,negative?nx:x.n,x.limbs);
  return r;
}

template<int a> OTHER_PURE static inline Exact<3*a> cube(const Exact<a> x) {
  return x*sqr(x);
}

template<int a> void operator++(Exact<a>& x) {
  mpn_add_1(x.n,x.n,x.limbs,1);
}

template<int a> OTHER_PURE static inline Exact<(a>>1) + (a&1)> ceil_sqrt(const Exact<a> x) {
  assert(!is_negative(x));
  Exact<(a>>1)+(a&1)> r;
  const auto trim_x = trim(asarray(x.n));
  const auto inexact = mpn_sqrtrem(r.n,0,trim_x.data(),trim_x.size());
  if(inexact)
    ++r;
  return r;
}

template<int a> static inline Exact<a>& operator>>=(Exact<a>& x, const int s) {
  assert(!is_negative(x));
  assert(0<s && s<=3);
  mpn_rshift(x.n,x.n,x.limbs,s);
  return x;
}

template<int a> OTHER_PURE static inline Exact<a> operator>>(const Exact<a> x, const int s) {
  Exact<1> r = x;
  r >>= s;
  return r;
}

// Multiplication by small constants, assumed to not increase the precision required

template<int a> OTHER_PURE static inline Exact<a> operator<<(const Exact<a> x, const int s) {
  assert(0<s && s<=3);
  Exact<a> r(uninit);
  mpn_lshift(r.n,x.n,x.limbs,s);
  return r;
}

template<int a> OTHER_PURE static inline Exact<a> small_mul(const int n, const Exact<a> x) {
  assert(n);
  Exact<a> r(uninit);
  if (power_of_two(uint32_t(abs(n)))) { // This routine will normally be inlined with constant n, so this check is cheap
    if (abs(n) > 1)
      mpn_lshift(r.n,x.n,x.limbs,integer_log_exact(uint32_t(abs(n))));
  } else
    mpn_mul_1(r.n,x.n,x.limbs,abs(n));
  return n<0 ? -r : r;
}

// Copy from Exact<d> to an array with sign extension

static inline void mpz_set(RawArray<mp_limb_t> x, RawArray<const mp_limb_t> y) {
  x.slice(0,y.size()) = y;
  x.slice(y.size(),x.size()).fill(mp_limb_signed_t(y.back())>=0 ? 0 : mp_limb_t(mp_limb_signed_t(-1)));
}

static inline void mpz_set_nonnegative(RawArray<mp_limb_t> x, RawArray<const mp_limb_t> y) {
  assert(mp_limb_signed_t(y.back())>=0);
  x.slice(0,y.size()) = y;
  x.slice(y.size(),x.size()).fill(0);
}

template<int d> static inline void mpz_set(RawArray<mp_limb_t> x, const Exact<d>& y) {
  mpz_set(x,asarray(y.n));
}

// String conversion

using std::ostream;
OTHER_CORE_EXPORT string mpz_str(RawArray<const mp_limb_t> limbs, const bool hex=false);
OTHER_CORE_EXPORT string mpz_str(Subarray<const mp_limb_t,2> limbs, const bool hex=false);

template<int d> static inline ostream& operator<<(ostream& output, const Exact<d>& x) {
  return output << mpz_str(asarray(x.n));
}

// Overloads to make One work with perturbed_predicate_sqrt and friends

static inline int weak_sign(One) {
  return 1;
}

static inline int mpz_set(RawArray<mp_limb_t> x, One) {
  // We'll never reach here, since the "filter" step always succeeds for One
  OTHER_UNREACHABLE();
}

struct SmallShift {
  int s;
};
static inline SmallShift operator<<(One, const int s) {
  SmallShift r;
  r.s = s;
  return r;
}
template<class T> static inline T operator*(const T& x, const SmallShift s) {
  return x<<s.s;
}

RawArray<mp_limb_t> trim(RawArray<mp_limb_t> x);
RawArray<const mp_limb_t> trim(RawArray<const mp_limb_t> x);

}