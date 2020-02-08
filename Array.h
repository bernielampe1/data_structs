#ifndef _BHL_ARRAY_H_
#define _BHL_ARRAY_H_

#include<ostream>

namespace bhl {
template <typename T> class Array {
private:
  T *_data;
  unsigned _n;

public:
  class iterator {
  private:
    Array<T> *_ptr;
    unsigned _p;

  public:
    iterator(Array<T> *ptr, unsigned p) : _ptr(ptr), _p(p) {}

    bool operator==(const iterator &rhs) const { return (_p == rhs._p); }
    bool operator!=(const iterator &rhs) const { return (_p != rhs._p); }
    bool operator<(const iterator &rhs) const { return (_p < rhs._p); }
    bool operator>(const iterator &rhs) const { return (_p > rhs._p); }
    iterator &operator++() {
      _p++;
      return (*this);
    }
    iterator operator++(int) {
      iterator it = *this;
      _p++;
      return (it);
    }
    iterator &operator--() {
      _p--;
      return (*this);
    }
    iterator operator--(int) {
      iterator it = *this;
      _p--;
      return (it);
    }
    iterator &operator+=(const int i) {
      _p += i;
      return (*this);
    }
    iterator &operator-=(const int i) {
      _p -= i;
      return (*this);
    }
    T &operator*() const { return ((*_ptr)[_p]); }
    T *operator->() const { return (&(operator*())); };
  };

  class const_iterator {
  private:
    const Array<T> *_ptr;
    unsigned _p;

  public:
    const_iterator(const Array<T> *ptr, unsigned p) : _ptr(ptr), _p(p) {}

    bool operator==(const const_iterator &rhs) const { return (_p == rhs._p); }
    bool operator!=(const const_iterator &rhs) const { return (_p != rhs._p); }
    bool operator<(const const_iterator &rhs) const { return (_p < rhs._p); }
    bool operator>(const const_iterator &rhs) const { return (_p > rhs._p); }
    const_iterator &operator++() {
      _p++;
      return (*this);
    }
    const_iterator operator++(int) {
      const_iterator it = *this;
      _p++;
      return (it);
    }
    const_iterator &operator--() {
      _p--;
      return (*this);
    }
    const_iterator operator--(int) {
      const_iterator it = *this;
      _p--;
      return (it);
    }
    const_iterator &operator+=(const int i) {
      _p += i;
      return (*this);
    }
    const_iterator &operator-=(const int i) {
      _p -= i;
      return (*this);
    }
    const T &operator*() const { return ((*_ptr)[_p]); }
    const T *operator->() const { return (&(operator*())); };
  };

public:
  Array() : _data(new T[0]), _n(0) {}

  Array(const unsigned n) : _data(new T[n]), _n(n) {}

  Array(const T &o, const unsigned n) : _data(new T[n]), _n(n) {
    for (unsigned i = 0; i < _n; i++) {
      _data[i] = o;
    }
  }

  Array(const Array &rhs) : _data(new T[rhs._n]), _n(rhs._n) {
    for (unsigned i = 0; i < _n; i++) {
      _data[i] = rhs._data[i];
    }
  }

  ~Array() { delete[] _data; }

  Array &operator=(const Array &rhs) {
    if (&rhs != this) {
      if (_data) {
        delete[] _data;
      }
      _n = rhs._n;
      _data = new T[_n];

      for (unsigned i = 0; i < _n; i++) {
        _data[i] = rhs._data[i];
      }
    }

    return (*this);
  }

  const T &operator[](const unsigned i) const { return (_data[i]); }

  T &operator[](const unsigned i) { return (_data[i]); }

  void resize(const unsigned n) {
    T *_tmpData = new T[n];

    unsigned m = n < _n ? n : _n;
    for (unsigned i = 0; i < m; i++) {
      _tmpData[i] = _data[i];
    }

    if (_data) {
      delete[] _data;
    }
    _data = _tmpData;
    _n = n;
  }

  const T *data() { return (_data); }

  unsigned size() const { return (_n); }

  bool empty() { return (_n == 0); }

  void clear() { resize(0); }

  iterator begin() { return (iterator(this, 0)); }
  const_iterator begin() const { return (const_iterator(this, 0)); }

  iterator end() { return (iterator(this, _n)); }
  const_iterator end() const { return (const_iterator(this, _n)); }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const Array<T> &rhs) {
  for (typename Array<T>::const_iterator it = rhs.begin(); it != rhs.end();
       it++) {
    os << *it << ", ";
  }

  return (os);
}
} // namespace bhl

#endif // _BHL_ARRAY_H_
