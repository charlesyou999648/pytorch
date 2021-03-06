#pragma once

#include <cstring>
#include <functional>
#include <cmath>
#include <type_traits>
#include <bitset>

#include "ATen/Utils.h"
#include <c10/util/C++17.h>

#if defined(__GNUC__)
#define __at_align32__ __attribute__((aligned(32)))
#elif defined(_WIN32)
#define __at_align32__ __declspec(align(32))
#else
#define __at_align32__
#endif

namespace at {
namespace vec256 {
namespace {

template<size_t n> struct int_of_size;

#define DEFINE_INT_OF_SIZE(int_t) \
template<> struct int_of_size<sizeof(int_t)> { using type = int_t; }

DEFINE_INT_OF_SIZE(int64_t);
DEFINE_INT_OF_SIZE(int32_t);
DEFINE_INT_OF_SIZE(int16_t);
DEFINE_INT_OF_SIZE(int8_t);

#undef DEFINE_INT_OF_SIZE

template <typename T>
using int_same_size_t = typename int_of_size<sizeof(T)>::type;

// NOTE: If you specialize on a type, you must define all operations!

// emulates vectorized types
template <class T>
struct Vec256 {
private:
  T values[32 / sizeof(T)] = {0};
public:
  static constexpr int size = 32 / sizeof(T);
  Vec256() {}
  Vec256(T val) {
    for (int i = 0; i != size; i++) {
      values[i] = val;
    }
  }
  template<typename... Args,
           typename = c10::guts::enable_if_t<(sizeof...(Args) == size)>>
  Vec256(Args... vals) {
    values = { vals... };
  }
  template <int64_t mask_>
  static Vec256<T> blend(const Vec256<T>& a, const Vec256<T>& b) {
    int64_t mask = mask_;
    Vec256 vec;
    for (int64_t i = 0; i < size; i++) {
      if (mask & 0x01) {
        vec[i] = b[i];
      } else {
        vec[i] = a[i];
      }
      mask = mask >> 1;
    }
    return vec;
  }
  static Vec256<T> blendv(const Vec256<T>& a, const Vec256<T>& b,
                          const Vec256<T>& mask) {
    Vec256 vec;
    int_same_size_t<T> buffer[size];
    mask.store(buffer);
    for (int64_t i = 0; i < size; i++) {
      if (buffer[i] & 0x01)
       {
        vec[i] = b[i];
      } else {
        vec[i] = a[i];
      }
    }
    return vec;
  }
  static Vec256<T> arange(T base = static_cast<T>(0), T step = static_cast<T>(1)) {
    Vec256 vec;
    for (int64_t i = 0; i < size; i++) {
      vec.values[i] = base + i * step;
    }
    return vec;
  }
  static Vec256<T> set(const Vec256<T>& a, const Vec256<T>& b, int64_t count = size) {
    Vec256 vec;
    for (int64_t i = 0; i < size; i++) {
      if (i < count) {
        vec[i] = b[i];
      } else {
        vec[i] = a[i];
      }
    }
    return vec;
  }
  static Vec256<T> loadu(const void* ptr) {
    Vec256 vec;
    std::memcpy(vec.values, ptr, 32);
    return vec;
  }
  static Vec256<T> loadu(const void* ptr, int64_t count) {
    Vec256 vec;
    std::memcpy(vec.values, ptr, count * sizeof(T));
    return vec;
  }
  void store(void* ptr, int count = size) const {
    std::memcpy(ptr, values, count * sizeof(T));
  }
  const T& operator[](int idx) const {
    return values[idx];
  }
  T& operator[](int idx) {
    return values[idx];
  }
  Vec256<T> map(T (*f)(T)) const {
    Vec256<T> ret;
    for (int64_t i = 0; i != size; i++) {
      ret[i] = f(values[i]);
    }
    return ret;
  }
  Vec256<T> abs() const {
    Vec256<T> ret;
    for (int64_t i = 0; i < size; i++) {
      ret[i] = values[i] < 0 ? -values[i] : values[i];
    }
    return ret;
  }
  Vec256<T> acos() const {
    return map(std::acos);
  }
  Vec256<T> asin() const {
    return map(std::asin);
  }
  Vec256<T> atan() const {
    return map(std::atan);
  }
  Vec256<T> erf() const {
    return map(std::erf);
  }
  Vec256<T> erfc() const {
    return map(std::erfc);
  }
  Vec256<T> exp() const {
    return map(std::exp);
  }
  Vec256<T> expm1() const {
    return map(std::expm1);
  }
  Vec256<T> log() const {
    return map(std::log);
  }
  Vec256<T> log10() const {
    return map(std::log10);
  }
  Vec256<T> log1p() const {
    return map(std::log1p);
  }
  Vec256<T> log2() const {
    return map(std::log2);
  }
  Vec256<T> ceil() const {
    return map(std::ceil);
  }
  Vec256<T> cos() const {
    return map(std::cos);
  }
  Vec256<T> cosh() const {
    return map(std::cosh);
  }
  Vec256<T> floor() const {
    return map(std::floor);
  }
  Vec256<T> neg() const {
    return map([](T x) { return -x; });
  }
  Vec256<T> round() const {
    return map(std::round);
  }
  Vec256<T> sin() const {
    return map(std::sin);
  }
  Vec256<T> sinh() const {
    return map(std::sinh);
  }
  Vec256<T> tan() const {
    return map(std::tan);
  }
  Vec256<T> tanh() const {
    return map(std::tanh);
  }
  Vec256<T> trunc() const {
    return map(std::trunc);
  }
  Vec256<T> sqrt() const {
    return map(std::sqrt);
  }
  Vec256<T> reciprocal() const {
    return map([](T x) { return (T)(1) / x; });
  }
  Vec256<T> rsqrt() const {
    return map([](T x) { return 1 / std::sqrt(x); });
  }
  Vec256<T> pow(const Vec256<T> &exp) const {
    Vec256<T> ret;
    for (int64_t i = 0; i < size; i++) {
      ret[i] = std::pow(values[i], exp[i]);
    }
    return ret;
  }
#define DEFINE_COMP(binary_pred)                                              \
  Vec256<T> operator binary_pred(const Vec256<T> &other) const {              \
    Vec256<T> vec;                                                            \
    for (int64_t i = 0; i != size; i++) {                                     \
      if (values[i] binary_pred other.values[i]) {                            \
        std::memset(static_cast<void*>(vec.values + i), 0xFF, sizeof(T));     \
      } else {                                                                \
        std::memset(static_cast<void*>(vec.values + i), 0, sizeof(T));        \
      }                                                                       \
    }                                                                         \
    return vec;                                                               \
  }
  DEFINE_COMP(==)
  DEFINE_COMP(!=)
  DEFINE_COMP(>=)
  DEFINE_COMP(<=)
  DEFINE_COMP(>)
  DEFINE_COMP(<)
#undef DEFINE_COMP
};

template <class T> Vec256<T> inline operator+(const Vec256<T> &a, const Vec256<T> &b) {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = a[i] + b[i];
  }
  return c;
}

template <class T> Vec256<T> inline operator-(const Vec256<T> &a, const Vec256<T> &b) {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = a[i] - b[i];
  }
  return c;
}

template <class T> Vec256<T> inline operator*(const Vec256<T> &a, const Vec256<T> &b) {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = a[i] * b[i];
  }
  return c;
}

template <class T> Vec256<T> inline operator/(const Vec256<T> &a, const Vec256<T> &b) __ubsan_ignore_float_divide_by_zero__ {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = a[i] / b[i];
  }
  return c;
}


template <class T> Vec256<T> inline max(const Vec256<T> &a, const Vec256<T> &b) {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = std::max(a[i], b[i]);
  }
  return c;
}

template <class T> Vec256<T> inline min(const Vec256<T> &a, const Vec256<T> &b) {
  Vec256<T> c = Vec256<T>();
  for (int i = 0; i != Vec256<T>::size; i++) {
    c[i] = std::min(a[i], b[i]);
  }
  return c;
}

#define DEFINE_BITWISE_OP(op)                                               \
template <class T>                                                          \
Vec256<T> inline operator op(const Vec256<T> &a, const Vec256<T> &b) {      \
  using iT = int_same_size_t<T>;                                            \
  iT buffer[Vec256<T>::size];                                               \
  for (int64_t i = 0; i != Vec256<T>::size; i++) {                          \
    auto a_val = a[i];                                                      \
    auto b_val = b[i];                                                      \
    iT *i_a_ptr = reinterpret_cast<iT*>(&a_val);                            \
    iT *i_b_ptr = reinterpret_cast<iT*>(&b_val);                            \
    buffer[i] = *i_a_ptr op *i_b_ptr;                                       \
  }                                                                         \
  return Vec256<T>::loadu(buffer);                                          \
}
DEFINE_BITWISE_OP(&)
DEFINE_BITWISE_OP(|)
DEFINE_BITWISE_OP(^)
#undef DEFINE_BITWISE_OP

template <typename T>
inline T fmadd(const T& a, const T& b, const T& c) {
  return a * b + c;
}

template <int64_t scale = 1, typename T = void>
c10::guts::enable_if_t<scale == 1 || scale == 2 || scale == 4 || scale == 8, Vec256<T>>
inline gather(T const* base_addr, const Vec256<int_same_size_t<T>>& vindex) {
  static constexpr int size = Vec256<T>::size;
  int_same_size_t<T> index_arr[size];
  vindex.store(static_cast<void*>(index_arr));
  T buffer[size];
  for (int64_t i = 0; i < size; i++) {
    buffer[i] = base_addr[index_arr[i] * scale / sizeof(T)];
  }
  return Vec256<T>::loadu(static_cast<void*>(buffer));
}

template <int64_t scale = 1, typename T = void>
c10::guts::enable_if_t<scale == 1 || scale == 2 || scale == 4 || scale == 8, Vec256<T>>
inline mask_gather(const Vec256<T>& src, T const* base_addr,
                   const Vec256<int_same_size_t<T>>& vindex, Vec256<T>& mask) {
  static constexpr int size = Vec256<T>::size;
  T src_arr[size];
  int_same_size_t<T> mask_arr[size];  // use int type so we can logical and
  int_same_size_t<T> index_arr[size];
  src.store(static_cast<void*>(src_arr));
  mask.store(static_cast<void*>(mask_arr));
  vindex.store(static_cast<void*>(index_arr));
  T buffer[size];
  for (int64_t i = 0; i < size; i++) {
    if (mask_arr[i] & 0x01) {  // check highest bit
      buffer[i] = base_addr[index_arr[i] * scale / sizeof(T)];
    } else {
      buffer[i] = src_arr[i];
    }
  }
  mask = Vec256<T>();  // "zero out" mask
  return Vec256<T>::loadu(static_cast<void*>(buffer));
}

// Cast a given vector to another type without changing the bits representation.
// So a Vec<double> of 256 bits containing all ones can be cast to a
// Vec<int64_t> of 256 bits containing all ones (i.e., four negative 1s).
namespace {
  // There is a struct here because we don't have static_if and I can't
  // partially specialize a templated function.
  template<typename dst_t, typename src_t>
  struct CastImpl {
    static inline Vec256<dst_t> apply(const Vec256<src_t>& src) {
      src_t src_arr[Vec256<src_t>::size];
      src.store(static_cast<void*>(src_arr));
      return Vec256<dst_t>::loadu(static_cast<const void*>(src_arr));
    }
  };

  template<typename scalar_t>
  struct CastImpl<scalar_t, scalar_t> {
    static inline Vec256<scalar_t> apply(const Vec256<scalar_t>& src) {
      return src;
    }
  };
}
template<typename dst_t, typename src_t>
Vec256<dst_t> cast(const Vec256<src_t>& src) {
  return CastImpl<dst_t, src_t>::apply(src);
}

template <typename T>
inline Vec256<int_same_size_t<T>> convert_to_int_of_same_size(const Vec256<T>& src) {
  static constexpr int size = Vec256<T>::size;
  T src_arr[size];
  src.store(static_cast<void*>(src_arr));
  int_same_size_t<T> buffer[size];
  for (int64_t i = 0; i < size; i++) {
    buffer[i] = static_cast<int_same_size_t<T>>(src_arr[i]);
  }
  return Vec256<int_same_size_t<T>>::loadu(static_cast<void*>(buffer));
}

// E.g., inputs: a           Vec256<float>   = {a0, b0, a1, b1, a2, b2, a3, b3}
//               b           Vec256<float>   = {a4, b4, a5, b5, a6, b6, a7, b7}
//       returns:            Vec256<float>   = {a0, a1, a2, a3, a4, a5, a6, a7}
//                           Vec256<float>   = {b0, b1, b2, b3, b4, b5, b6, b7}
template <typename T>
inline c10::guts::enable_if_t<Vec256<T>::size % 2 == 0, std::pair<Vec256<T>, Vec256<T>>>
deinterleave2(const Vec256<T>& a, const Vec256<T>& b) {
  static constexpr int size = Vec256<T>::size;
  static constexpr int half_size = size / 2;
  T a_arr[size];
  T b_arr[size];
  T buffer1[size];
  T buffer2[size];
  a.store(static_cast<void*>(a_arr));
  b.store(static_cast<void*>(b_arr));
  for (int64_t i = 0; i < half_size; i++) {
    buffer1[i] = a_arr[i * 2];
    buffer1[half_size + i] = b_arr[i * 2];
    buffer2[i] = a_arr[i * 2 + 1];
    buffer2[half_size + i] = b_arr[i * 2 + 1];
  }
  return std::make_pair(Vec256<T>::loadu(static_cast<void*>(buffer1)),
                        Vec256<T>::loadu(static_cast<void*>(buffer2)));
}

// inverse operation of deinterleave2
// E.g., inputs: a           Vec256<float>   = {a0, a1, a2, a3, a4, a5, a6, a7}
//               b           Vec256<float>   = {b0, b1, b2, b3, b4, b5, b6, b7}
//       returns:            Vec256<float>   = {a0, b0, a1, b1, a2, b2, a3, b3}
//                           Vec256<float>   = {a4, b4, a5, b5, a6, b6, a7, b7}
template <typename T>
inline c10::guts::enable_if_t<Vec256<T>::size % 2 == 0, std::pair<Vec256<T>, Vec256<T>>>
interleave2(const Vec256<T>& a, const Vec256<T>& b) {
  static constexpr int size = Vec256<T>::size;
  static constexpr int half_size = size / 2;
  T a_arr[size];
  T b_arr[size];
  T buffer1[size];
  T buffer2[size];
  a.store(static_cast<void*>(a_arr));
  b.store(static_cast<void*>(b_arr));
  for (int64_t i = 0; i < half_size; i++) {
    buffer1[i * 2] = a_arr[i];
    buffer1[i * 2 + 1] = b_arr[i];
    buffer2[i * 2] = a_arr[half_size + i];
    buffer2[i * 2 + 1] = b_arr[half_size + i];
  }
  return std::make_pair(Vec256<T>::loadu(static_cast<void*>(buffer1)),
                        Vec256<T>::loadu(static_cast<void*>(buffer2)));
}

template <typename src_T, typename dst_T>
void convert(const src_T *src, dst_T *dst, int64_t n) {
#pragma unroll
  for (int64_t i = 0; i < n; i++) {
    *dst = static_cast<dst_T>(*src);
    src++;
    dst++;
  }
}

}}}
