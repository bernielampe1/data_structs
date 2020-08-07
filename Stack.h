#pragma once

#include<ostream>

template <typename T> class Stack;
template <typename T>
std::ostream &operator<<(std::ostream &, const Stack<T> &);

template <typename T> class Stack {
  struct Node {
    Node(const T &d, Node *p) : _data(d), _prev(p) {}

    T _data;
    Node *_prev;
  };

private:
  Node *_top;
  unsigned _n;

  void copy(const Stack<T> &rhs) {
    if (this != &rhs) {
      clear();

      _n = rhs._n;
      Node *ptr1 = rhs._top;
      Node *ptr2 = 0;
      for (unsigned i = 0; i < _n; i++) {
        if (ptr2)
          ptr2 = ptr2->_prev = new Node(ptr1->_data, 0);
        else
          ptr2 = new Node(ptr1->_data, 0);

        ptr1 = ptr1->_prev;

        if (i == 0)
          _top = ptr2;
      }
    }
  }

public:
  Stack() : _top(0), _n(0) {}

  Stack(const Stack &rhs) : _top(0), _n(rhs._n) { copy(rhs); }

  ~Stack() { clear(); }

  Stack &operator=(const Stack &rhs) {
    copy(rhs);
    return (*this);
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void clear() {
    Node *ptr = _top;
    while (ptr != 0) {
      Node *tmp = ptr;
      ptr = ptr->_prev;
      delete tmp;
    }

    _top = 0;
    _n = 0;
  }

  void push(const T &d) {
    _top = new Node(d, _top);
    _n++;
  }

  void pop() {
    if (!empty()) {
      Node *tmp = _top;
      _top = _top->_prev;
      delete tmp;
      _n--;
    }
  }

  T &top() { return (_top->_data); }

  friend std::ostream &operator<<<>(std::ostream &os, const Stack<T> &rhs);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const Stack<T> &rhs) {
  typename Stack<T>::Node *ptr = rhs._top;
  while (ptr != 0) {
    os << ptr->_data << ", ";
    ptr = ptr->_prev;
  }

  return (os);
}
