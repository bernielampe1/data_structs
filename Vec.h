#ifndef __VEC_H__
#define __VEC_H__

#include <cstdlib>
#include <ostream>
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

  Vec<T>(Vec<T> &&o) : _n(o._n), _data(o._data) {
    o._data = 0;
    o._n = 0;
  }

  Vec<T> &operator=(const Vec<T> &o) {
    if (this != &o) {
      init(o._n);
      for (u32 i = 0; i < o.n; i++)
        _data[i] = o._data[i];
    }

    return *this;
  }

  Vec<T> &operator=(Vec<T> &&o) {
    if (this != &o) {
      if (_data)
        clear();

      _data = o._data;
      _n = o._n;

      o._data = 0;
      o._n = 0;
    }

    return *this;
  }

  ~Vec() { clear(); }

  void init(const u32 n, const T &v = 0) {
    T *temp = new T[n];

    if (_data)
      clear();

    _n = n;
    _data = temp;
    for (u32 i = 0; i < n; i++)
      _data[i] = v;
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

template<typename T>
std::ostream &operator<<(std::ostream &os, const Vec<T> &v) {
    for(u32 i = 0; i < v.len(); i++) os << v[i] << ",";
    return os;
}

#include "Vec.inl"

#endif // __VEC_H__
