#pragma once

#include<ostream>

template <typename T> class Queue;
template <typename T>
std::ostream &operator<<(std::ostream &, const Queue<T> &);

template <typename T> class Queue {
  struct Node {
    Node(const T &d, Node *n, Node *p) : _data(d), _next(n), _prev(p) {}

    T _data;
    Node *_next;
    Node *_prev;
  };

private:
  Node *_front, *_back;
  unsigned _n;

  void copy(const Queue<T> &rhs) {
    if (this != &rhs) {
      clear();

      _n = rhs._n;
      Node *ptr1 = rhs._front;
      Node *ptr2 = 0;

      for (unsigned i = 0; i < _n; i++) {
        if (ptr2)
          ptr2 = ptr2->_next = new Node(ptr1->_data, 0, ptr2);
        else
          ptr2 = new Node(ptr1->_data, 0, 0);

        ptr1 = ptr1->_next;

        if (i == 0)
          _front = ptr2;
      }

      _back = ptr2;
    }
  }

public:
  Queue() : _front(0), _back(0), _n(0) {}

  Queue(const Queue &rhs) : _front(0), _back(0), _n(rhs._n) { copy(rhs); }

  ~Queue() { clear(); }

  Queue &operator=(const Queue &rhs) {
    copy(rhs);
    return (*this);
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void clear() {
    Node *ptr = _front;
    while (ptr != 0) {
      Node *tmp = ptr;
      ptr = ptr->_next;
      delete tmp;
    }

    _front = _back = 0;
    _n = 0;
  }

  void push(const T &d) {
    if (!empty())
      _front = _front->_prev = new Node(d, _front, 0);
    else {
      _front = new Node(d, 0, 0);
      _back = _front;
    }
    _n++;
  }

  void pop() {
    if (!empty()) {
      Node *tmp = _back;
      _back = _back->_prev;
      delete tmp;
      _n--;

      if (_n == 0)
        _front = _back = 0;
      if (_back)
        _back->_next = 0;
    }
  }

  T &front() { return (_front->_data); }

  T &back() { return (_back->_data); }

  friend std::ostream &operator<<<>(std::ostream &os, const Queue<T> &rhs);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const Queue<T> &rhs) {
  typename Queue<T>::Node *ptr = rhs._back;
  while (ptr != 0) {
    os << ptr->_data << ", ";
    ptr = ptr->_prev;
  }

  return (os);
}
