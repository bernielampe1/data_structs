#pragma once

#include <fstream>
#include <memory>
#include <string>

#include "Exception.h"
#include "types.h"
#include "Vec.h"
#include "drawLine.h"

/* Abstraction of a 2-D image with template pixel type.
 *
 * Rule-of-five implementation (see README.md): the destructor, copy and
 * move construction, and copy and move assignment are all defined.
 * Moves are noexcept and leave the source empty and reusable.
 *
 * The pixel buffer is an owned dynamic array; copies are deep, and the
 * buffer is filled by value-assignment (element by element) rather than
 * memcpy, so a throwing pixel copy leaves *this untouched and leaks
 * nothing. operator[] and get() are unchecked (same contract as
 * std::vector's operator[]); at(r, c) is bounds-checked and throws.
 *
 * operator+, -, * require equal dimensions and throw Exception
 * otherwise (the old versions read/wrote out of bounds on mismatch).
 *
 * File I/O: Image<u8> reads and writes PGM (P5); Image<RGB_t> reads and
 * writes PPM (P6). PGM writes normalize the dynamic range of T to 0..255;
 * PPM writes the three RGB channel bytes directly. Anything reading a
 * malformed or mismatched file throws Exception.
 */
template <typename T> class Image {
private:
  T *_data;            // dynamic data storage for 2-D array of pixels
  u32 _height, _width; // image dimensions

public:
  Image() : _data(0), _height(0), _width(0) {}

  Image(const u32 h, const u32 w) : _data(0), _height(h), _width(w) {
    init(_height, _width);
  }

  // Deep copy: the new buffer is built and filled under an RAII guard
  // before the old one is released, so a failed pixel copy leaves *this
  // untouched and leaks nothing.
  Image(const Image<T> &o)
      : _data(new T[o._height * o._width]()), _height(o._height),
        _width(o._width) {
    std::unique_ptr<T[]> guard(_data);
    for (u32 i = 0; i < _height * _width; i++)
      _data[i] = o._data[i];
    guard.release();
  }

  // Steal o's buffer; o left empty and reusable.
  Image(Image<T> &&o) noexcept
      : _data(o._data), _height(o._height), _width(o._width) {
    o._data = 0;
    o._height = o._width = 0;
  }

  ~Image() { delete[] _data; }

  // Reinitializes to h x w zeroed pixels, releasing any previous
  // storage. The new buffer exists before the old one is freed, so a
  // failed allocation leaves *this untouched.
  void init(const u32 h, const u32 w) {
    std::unique_ptr<T[]> temp(new T[h * w]());

    delete[] _data;
    _data = temp.release();
    _height = h;
    _width = w;
  }

  // Releases all memory; the image becomes empty and reusable.
  void clear() {
    delete[] _data;
    _data = 0;
    _height = _width = 0;
  }

  u32 height() const { return (_height); }

  u32 width() const { return (_width); }

  u32 size() const { return (_height * _width); } // pixel count

  bool empty() const { return (_height * _width == 0); }

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  Image<T> &operator=(const Image<T> &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<T[]> temp(new T[o._height * o._width]());
    for (u32 i = 0; i < o._height * o._width; i++)
      temp[i] = o._data[i];

    delete[] _data;
    _data = temp.release();
    _height = o._height;
    _width = o._width;

    return *this;
  }

  // Steal o's buffer, freeing ours first (nothrow deletes).
  Image<T> &operator=(Image<T> &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _height = o._height;
      _width = o._width;

      o._data = 0;
      o._height = o._width = 0;
    }

    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Image<T> &o) {
    std::swap(_data, o._data);
    std::swap(_height, o._height);
    std::swap(_width, o._width);
  }

  T &operator[](const u32 i) { return (_data[i]); }             // unchecked

  const T &operator[](const u32 i) const { return (_data[i]); } // unchecked read

  T &get(const u32 i) { return (_data[i]); }                    // unchecked

  const T &get(const u32 i) const { return (_data[i]); }        // unchecked read

  // Bounds-checked element access: throws Exception when (r, c) is
  // outside the image.
  T &at(const u32 r, const u32 c) {
    if (r >= _height || c >= _width)
      throw Exception("Image::at: coordinates outside the image");
    return _data[r * _width + c];
  }

  const T &at(const u32 r, const u32 c) const {
    if (r >= _height || c >= _width)
      throw Exception("Image::at: coordinates outside the image");
    return _data[r * _width + c];
  }

  void set(const u32 i, const T &v) { _data[i] = v; }

  void convolve(const float *k, const u32 ksize);

  void convolve(const float *k, const u32 kheight, const u32 kwidth);

  Image<T> operator+(const Image<T> &im) const;

  Image<T> operator-(const Image<T> &im) const;

  Image<T> operator*(const Image<T> &im) const;

  void readFromFile(const std::string &fname);

  void writeToFile(const std::string &fname) const;

  friend std::ostream &operator<<(std::ostream &os, const Image<T> &im) {
    for (u32 r = 0; r < im._height; r++) {
      for (u32 c = 0; c < im._width; c++) {
        os << im._data[r * im._width + c] << " ";
      }
      os << "\n";
    }
    return os;
  }
};

#include "Image.inl"
