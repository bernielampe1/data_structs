#pragma once

#include <ostream>
#include <utility>

template <typename T> class Stack;
template <typename T>
std::ostream &operator<<(std::ostream &, const Stack<T> &);

// Stack<T>: LIFO container built on a singly-linked node chain.
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// Element requirements: T must be default-constructible and
// copy-assignable (nodes are allocated with data attached).

template <typename T> class Stack {
  struct Node {
    Node(const T &d, Node *p) : _data(d), _prev(p) {}

    T _data;
    Node *_prev;
  };

private:
  Node *_top;   // top node; null when empty
  unsigned _n;  // number of nodes

  // Makes *this a copy of rhs. The new chain is built in a temporary
  // and adopted only when complete, so a throwing element copy leaves
  // *this untouched (strong exception safety) and leaks nothing.
  // Building in isolation also makes self-assignment safe.
  void copy(const Stack<T> &rhs) {
    Stack<T> tmp;

    Node *ptr1 = rhs._top;
    Node *ptr2 = 0;
    while (ptr1 != 0) {
      Node *node = new Node(ptr1->_data, 0);
      if (ptr2)
        ptr2->_prev = node;
      else
        tmp._top = node;
      ptr2 = node;
      ptr1 = ptr1->_prev;
    }
    tmp._n = rhs._n;

    swap(tmp); // tmp's destructor frees our old chain
  }

public:
  // Constant-time exchange of both chains.
  void swap(Stack &o) {
    Node *t = _top;
    unsigned n = _n;
    _top = o._top;
    _n = o._n;
    o._top = t;
    o._n = n;
  }

public:
  Stack() : _top(0), _n(0) {}

  Stack(const Stack &rhs) : _top(0), _n(0) { copy(rhs); }

  Stack(Stack &&o) noexcept // steal o's chain; o left empty
      : _top(o._top), _n(o._n) {
    o._top = 0;
    o._n = 0;
  }

  ~Stack() { clear(); }

  Stack &operator=(const Stack &rhs) {
    copy(rhs);
    return (*this);
  }

  Stack &operator=(Stack &&o) noexcept { // steal o's chain
    if (this != &o) {
      clear();
      _top = o._top;
      _n = o._n;
      o._top = 0;
      o._n = 0;
    }
    return (*this);
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void clear() { // free every node; stack becomes empty
    Node *ptr = _top;
    while (ptr != 0) {
      Node *tmp = ptr;
      ptr = ptr->_prev;
      delete tmp;
    }

    _top = 0;
    _n = 0;
  }

  void push(const T &d) { // push d onto the top
    _top = new Node(d, _top);
    _n++;
  }

  void pop() { // remove the top element (no-op if empty)
    if (!empty()) {
      Node *tmp = _top;
      _top = _top->_prev;
      delete tmp;
      _n--;
    }
  }

  T &top() { return (_top->_data); } // top element (UB if empty)

  friend std::ostream &operator<<<>(std::ostream &os, const Stack<T> &rhs);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const Stack<T> &rhs) {
  bool first = true;
  typename Stack<T>::Node *ptr = rhs._top;
  while (ptr != 0) {
    if (!first)
      os << ", ";
    os << ptr->_data;
    ptr = ptr->_prev;
    first = false;
  }

  return (os);
}
