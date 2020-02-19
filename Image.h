#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <fstream>
#include <string>

#include "Exception.h"
#include "Vec.h"
#include "drawLine.h"
#include "types.h"

/* Abstraction of a 2-D image with template pixel type. */
template <typename T> class Image {
private:
  T *_data;            // dynamic data storage for 2-D array of pixel
  u32 _height, _width; // image dimensions

public:
  Image() : _data(0), _height(0), _width(0) {}

  Image(const u32 h, const u32 w) : _data(0), _height(h), _width(w) {
    init(_height, _width);
  }

  Image(const Image<T> &o) : _data(0), _height(o._height), _width(o._width) {
    init(_height, _width);
    memcpy(_data, o._data, _height * _width * sizeof(T));
  }

  Image(Image<T> &&o) : _data(o._data), _height(o._height), _width(o._width) {
    o._data = 0;
    o._height = o._width = 0;
  }

  ~Image() { clear(); }

  void init(const u32 h, const u32 w) {
    T *temp = new T[h * w];

    if (_data)
      clear();

    _height = h;
    _width = w;
    _data = temp;
    memset(_data, 0, _height * _width * sizeof(T));
  }

  void clear() {
    if (_data)
      delete[] _data;
    _height = _width = 0;
    _data = 0;
  }

  u32 height() const { return (_height); }

  u32 width() const { return (_width); }

  Image<T> &operator=(const Image<T> &o) {
    if (this != &o) {
      init(o._height, o._width);
      memcpy(_data, o._data, _height * _width * sizeof(T));
    }

    return *this;
  }

  Image<T> &operator=(Image<T> &&o) {
    if (this != &o) {
      if (_data)
        clear();

      _data = o._data;
      _height = o._height;
      _width = o._width;

      o._data = 0;
      o._rows = o._cols = 0;
    }

    return *this;
  }

  T &operator[](const u32 i) const { return (_data[i]); }

  T &get(const u32 i) const { return (_data[i]); }

  void set(const u32 i, const T &v) { _data[i] = v; }

  void convolve(const float *k, const u32 ksize);

  void convolve(const float *k, const u32 kheight, const u32 kwidth);

  Image<T> operator+(const Image<T> &im) const;

  Image<T> operator-(const Image<T> &im) const;

  Image<T> operator*(const Image<T> &im) const;

  void readFromFile(const std::string &fname);

  void writeToFile(const std::string &fname) const;
};

#include "Image.inl"

#endif // __IMAGE_H__
