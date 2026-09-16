#pragma once

#include <cstdlib>
#include <utility>

#include "Exception.h"
#include "types.h"

/* Abstraction of a 2-D vector float */
struct Vec2f_t {
  float v[2];
  Vec2f_t() { v[0] = v[1] = 0.0; }
  float &operator[](const u32 i) { return (v[i]); }
  u32 len() { return 2; }
};

/* Abstraction of a 3-D float vector */
struct Vec3f_t {
  float v[3];
  Vec3f_t() { v[0] = v[1] = v[2] = 0.0; }
  float &operator[](const u32 i) { return (v[i]); }
  u32 len() { return 3; }
};

/* Abstraction of a 4-D float vector */
struct Vec4f_t {
  float v[4];
  Vec4f_t() { v[0] = v[1] = v[2] = v[3] = 0.0; }
  float &operator[](const u32 i) { return (v[i]); }
  u32 len() { return 4; }
};

// for pixels
typedef struct Vec3f_t RGB_t;
typedef struct Vec4f_t RGBA_t;

/* Abstraction for large vectors */
template <typename T> class Vec {
private:
  u32 _n;
  T *_data;

public:
  Vec() : _n(0), _data(0) {}

  Vec(u32 n) : _n(n), _data(0) { init(n); }

  Vec(const T v[], const u32 n) : _n(n), _data(0) {
    init(n);
    for(u32 i = 0; i < n; i++) _data[i] = v[i];
  }

  Vec(const Vec<T> &v) : _n(v._n), _data(0) {
    init(_n);
    for (u32 i = 0; i < v._n; i++)
      _data[i] = v._data[i];
  }

  Vec<T>(Vec<T> &&o) noexcept : _n(o._n), _data(o._data) { // steal o's buffer
    o._data = 0;
    o._n = 0;
  }

  // Deep copy: the new buffer is built and filled before the old one
  // is freed, so a failed allocation leaves *this untouched.
  Vec<T> &operator=(const Vec<T> &o) {
    if (this != &o) {
      T *temp = new T[o._n];
      for (u32 i = 0; i < o._n; i++)
        temp[i] = o._data[i];

      clear();
      _data = temp;
      _n = o._n;
    }

    return *this;
  }

  Vec<T> &operator=(Vec<T> &&o) noexcept { // steal o's buffer unconditionally
    if (this != &o) {
      clear();

      _data = o._data;
      _n = o._n;

      o._data = 0;
      o._n = 0;
    }

    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Vec<T> &o) {
    std::swap(_n, o._n);
    std::swap(_data, o._data);
  }

  ~Vec() { clear(); }

  void init(const u32 n, const T &v = 0) {
    T *temp = new T[n];
    for (u32 i = 0; i < n; i++)
      temp[i] = v;

    clear();
    _n = n;
    _data = temp;
  }

  void clear() {
    if (_data)
      delete[] _data;
    _n = 0;
    _data = 0;
  }

  T operator[](const u32 i) const { return (_data[i]); }
  T &operator[](const u32 i) { return (_data[i]); }

  u32 len() const { return _n; }

  T sum() const;
  T prod() const;
  Vec<T> abs() const;

  /* Vec scalar operations */
  Vec<T> operator+(const T &c) const;
  Vec<T> &operator+=(const T &c);

  Vec<T> operator-(const T &c) const;
  Vec<T> &operator-=(const T &c);

  Vec<T> operator*(const T &c) const;
  Vec<T> &operator*=(const T &c);

  Vec<T> operator/(const T &c) const;
  Vec<T> &operator/=(const T &c);

  /* Simple composition operators */
  Vec<T> operator+(const Vec<T> &m) const;
  Vec<T> &operator+=(const Vec<T> &m);

  Vec<T> operator-(const Vec<T> &m) const;
  Vec<T> &operator-=(const Vec<T> &m);

  Vec<T> operator*(const Vec<T> &m) const;
  Vec<T> &operator*=(const Vec<T> &m);

  Vec<T> operator/(const Vec<T> &m) const;
  Vec<T> &operator/=(const Vec<T> &m);
};

#include <ostream>
// Prints the elements separated by ", " with no trailing separator.
template<typename T>
std::ostream &operator<<(std::ostream &os, const Vec<T> &v) {
    for (u32 i = 0; i < v.len(); i++) {
      if (i > 0)
        os << ", ";
      os << v[i];
    }
    return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template<typename T> void swap(Vec<T> &a, Vec<T> &b) { a.swap(b); }

#include "Vec.inl"

