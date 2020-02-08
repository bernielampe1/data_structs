#ifndef _BHL_BINHEAP_H_
#define _BHL_BINHEAP_H_

#define MINHEAP 0
#define MAXHEAP 1
#define HEAPTYPE MINHEAP

#if HEAPTYPE == MINHEAP
#define CMPDIR <
#elif HEAPTYPE == MAXHEAP
#define CMPDIR >
#endif

namespace bhl {
template <typename T> class BinHeap {
private:
  T *_data;
  unsigned _n;
  unsigned _last;

  unsigned _parent(unsigned i) { return (i - 1) / 2; }

  unsigned _left(unsigned i) { return (2 * i + 1); }

  unsigned _right(unsigned i) { return (2 * i + 2); }

  void _swap(T *x, T *y) {
    T temp;
    *x = *y;
    *y = temp;
  }

  void _heapify(unsigned i) {
    unsigned l = _left(i);
    unsigned r = _right(i);
    unsigned t = i;

    if (l < size() && _data[l] CMPDIR _data[i])
      t = l;
    if (r < size() && _data[r] CMPDIR _data[i])
      t = r;
    if (t != i) {
      _swap(&_data[i], &_data[t]);
      _heapify(t);
    }
  }

public:
  BinHeap(const unsigned n) : _data(new T[n]), _n(n), _last(0) {}

  BinHeap(const BinHeap &rhs)
      : _data(new T[rhs._n]), _n(rhs._n), _last(rhs._last) {
    for (unsigned i = 0; i < _n; i++) {
      _data[i] = rhs._data[i];
    }
  }

  ~BinHeap() { delete[] _data; }

  BinHeap &operator=(const BinHeap &rhs) {
    if (&rhs != this) {
      if (_data) {
        delete[] _data;
      }
      _n = rhs._n;
      _last = rhs._last;
      _data = new T[_n];

      for (unsigned i = 0; i < _n; i++) {
        _data[i] = rhs._data[i];
      }
    }

    return (*this);
  }

  unsigned size() const { return (_last); }

  unsigned remaining() const { return (_n - size()); }

  unsigned capacity() const { return (_n); }

  bool empty() { return (_last == 0); }

  void clear() { _last = 0; }

  bool insert(const T &elem) {
    unsigned i = _last;

    if (size() >= _n) {
      return false;
    }

    _data[_last++] = elem;

    while (i > 0 && _data[_parent(i)] CMPDIR _data[i]) {
      _swap(&_data[_parent(i)], &_data[i]);
      i = _parent(i);
    }

    return true;
  }

  bool heapify(T arry[], unsigned n) {
    if (n > remaining())
      return false;

    for (int i = 0; i < n; i++) {
      insert(arry[i]);
    }

    return true;
  }

  T &peekroot() { return (empty() ? T() : _data[0]); }

  T extractroot() {
    T root;

    if (!empty()) {
      root = _data[0];
      _data[0] = _data[_parent(_last)];
      _last--;
      _heapify(0);
    }

    return root;
  }
};
} // namespace bhl

#endif // _BHL_BINHEAP_H_
