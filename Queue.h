#pragma once

#include <ostream>
#include <type_traits>
#include <utility>

 template <typename T> class Queue;
template <typename T>
std::ostream &operator<<(std::ostream &, const Queue<T> &);

// Queue<T>: FIFO container built on a doubly-linked node chain.
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// Semantics match std::queue: push() appends at the back, pop() removes
// the front (the oldest element), front() references the next element
// pop() would remove, back() references the newest.
//
// Element requirements: T must be copyable (nodes are allocated with
// data attached).

template <typename T> class Queue {
  struct Node {
    Node(const T &d, Node *n, Node *p) : _data(d), _next(n), _prev(p) {}

    T _data;
    Node *_next; // toward the back (newer)
    Node *_prev; // toward the front (older)
  };

private:
  Node *_front; // oldest element; null when empty
  Node *_back;  // newest element; null when empty
  unsigned _n;

  // Makes *this a copy of rhs. The new chain is built in a temporary
  // and adopted only when complete, so a throwing element copy leaves
  // *this untouched (strong exception safety) and leaks nothing.
  void copy(const Queue &rhs) {
    if (this == &rhs)
      return;

    Queue tmp;
    Node *ptr1 = rhs._front;
    Node *tail = 0;
    for (unsigned i = 0; i < rhs._n; i++) {
      Node *node = new Node(ptr1->_data, 0, tail);
      if (tail)
        tail->_next = node;
      else
        tmp._front = node; // first copied = oldest = front
      tail = node;
      ptr1 = ptr1->_next;
    }
    tmp._back = tail;
    tmp._n = rhs._n;

    swap(tmp); // tmp's destructor frees our old chain
  }

public:
  Queue() : _front(0), _back(0), _n(0) {}

  Queue(const Queue &rhs) : _front(0), _back(0), _n(0) { copy(rhs); }

  Queue(Queue &&o) noexcept // steal o's chain; o left empty
      : _front(o._front), _back(o._back), _n(o._n) {
    o._front = o._back = 0;
    o._n = 0;
  }

  ~Queue() { clear(); }

  Queue &operator=(const Queue &rhs) {
    copy(rhs);
    return (*this);
  }

  Queue &operator=(Queue &&o) noexcept { // steal o's chain
    if (this != &o) {
      clear();
      _front = o._front;
      _back = o._back;
      _n = o._n;
      o._front = o._back = 0;
      o._n = 0;
    }
    return (*this);
  }

  // Constant-time exchange of both chains.
  void swap(Queue &o) {
    Node *f = _front, *b = _back;
    unsigned n = _n;
    _front = o._front;
    _back = o._back;
    _n = o._n;
    o._front = f;
    o._back = b;
    o._n = n;
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void clear() { // free every node; queue becomes empty and reusable
    Node *ptr = _front;
    while (ptr != 0) {
      Node *tmp = ptr;
      ptr = ptr->_next;
      delete tmp;
    }
    _front = _back = 0;
    _n = 0;
  }

  void push(const T &d) { // append d at the back of the queue
    Node *node = new Node(d, 0, _back);
    if (_back)
      _back->_next = node;
    else
      _front = node; // first element is both front and back
    _back = node;
    _n++;
  }

  void pop() { // remove the front element (no-op if empty)
    if (empty())
      return;

    Node *old = _front;
    _front = _front->_next;
    if (_front)
      _front->_prev = 0;
    else
      _back = 0; // the queue just became empty

    delete old;
    _n--;
  }

  T &front() { return (_front->_data); } // oldest (UB if empty)

  const T &front() const { return (_front->_data); } // oldest (UB if empty)

  T &back() { return (_back->_data); } // newest (UB if empty)

  const T &back() const { return (_back->_data); } // newest (UB if empty)

  // Forward iterator over the node chain: begin() is the FRONT
  // (oldest) element and the walk follows _next toward the back;
  // end() is one past the back. Popping invalidates the iterator
  // naming the popped element only.
  template <typename U> class Iterator {
  private:
    U *_node;

    explicit Iterator(U *node) : _node(node) {}

    friend class Queue<T>;

  public:
    using value_type = typename std::remove_const<decltype(_node->_data)>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = typename std::conditional<
        std::is_const<typename std::remove_pointer<U>::type>::value,
        const value_type &, value_type &>::type;
    using iterator_category = std::forward_iterator_tag;

    bool operator==(const Iterator &o) const { return _node == o._node; }
    bool operator!=(const Iterator &o) const { return _node != o._node; }

    Iterator &operator++() { // advance toward the back (pre)
      _node = _node->_next;
      return *this;
    }
    Iterator operator++(int) { // advance (post)
      Iterator it = *this;
      _node = _node->_next;
      return it;
    }

    reference operator*() const { return _node->_data; }
    value_type *operator->() const { return &_node->_data; }
  };

  using iterator = Iterator<Node>;
  using const_iterator = Iterator<const Node>;

  iterator begin() { return iterator(_front); }         // the front node
  const_iterator begin() const { return const_iterator(_front); }

  iterator end() { return iterator(0); }                // past the back
  const_iterator end() const { return const_iterator(0); }

  friend std::ostream &operator<<<>(std::ostream &os, const Queue<T> &rhs);
};

// Prints the elements front to back (oldest first), ", " separated,
// with no trailing separator.
template <typename T>
std::ostream &operator<<(std::ostream &os, const Queue<T> &rhs) {
  bool first = true;
  for (typename Queue<T>::Node *ptr = rhs._front; ptr != 0;
       ptr = ptr->_next) {
    if (!first)
      os << ", ";
    os << ptr->_data;
    first = false;
  }

  return (os);
}
