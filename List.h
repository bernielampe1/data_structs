#pragma once

#include <cstddef>
#include <ostream>
#include <utility>

// List<T>: doubly-linked list organized as a ring around a sentinel.
//
// _head is a sentinel node, not an element. _head->_next is the first
// element and _head->_prev the last; when the list is empty both point
// back at _head. Because every node's next/prev is another node in the
// ring, no traversal ever sees a null pointer and no separate _tail
// bookkeeping exists to fall out of sync.
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// Iterators wrap a Node* and nothing else, so equality is node identity
// (iterators from different lists never compare equal) and end() is
// the sentinel -- a real past-the-end position, with --end() naming the
// last element. STL algorithms that need only bidirectional traversal
// work on these iterators (std::find, std::reverse); std::sort does not,
// because a list is not random-access -- the same contract as std::list.
//
// Element requirements: T must be default-constructible and
// copy-assignable (nodes are allocated with data attached).

template <typename T> class List {
  struct Node {
    Node(Node *n, Node *p) : _next(n), _prev(p) {}
    Node(const T &d, Node *n, Node *p) : _data(d), _next(n), _prev(p) {}

    T _data;
    Node *_next, *_prev;
  };

private:
  Node *_head;    // sentinel; first element is _head->_next
  unsigned _n;

  // Builds the empty ring: sentinel pointing at itself both ways.
  static Node *sentinel() {
    Node *s = new Node(0, 0);
    s->_next = s;
    s->_prev = s;
    return s;
  }

  // Links node between prev and prev->_next (the ring closure keeps the
  // sentinel's _prev naming the new last element). Returns node.
  Node *link(Node *prev, Node *node) {
    node->_prev = prev;
    node->_next = prev->_next;
    prev->_next->_prev = node;
    prev->_next = node;
    _n++;
    return node;
  }

  // Unlinks and frees node (never the sentinel). Returns its successor.
  Node *unlink(Node *node) {
    Node *after = node->_next;
    node->_prev->_next = after;
    after->_prev = node->_prev;
    delete node;
    _n--;
    return after;
  }

  // Makes *this hold the same elements as rhs. The copy is built in a
  // temporary and adopted by swap, so a throwing element copy leaves
  // *this untouched (strong exception safety) and leaks nothing.
  void copy(const List<T> &rhs) {
    if (this == &rhs)
      return;

    List<T> tmp;
    Node *ptr1 = rhs._head->_next;
    for (unsigned i = 0; i < rhs._n; i++) {
      tmp.link(tmp._head->_prev, new Node(ptr1->_data, 0, 0));
      ptr1 = ptr1->_next;
    }

    swap(tmp); // tmp's destructor frees our old ring
  }

public:
  // One class serves both iterator flavors: U is T for iterator and
  // const T for const_iterator.
  template <typename U> class Iterator {
  private:
    Node *_node; // current node; end() is the sentinel

    Iterator(Node *node) : _node(node) {}

    friend class List<T>;

  public:
    // Traits naming this a bidirectional iterator to the STL.
    using value_type = U;
    using difference_type = std::ptrdiff_t;
    using pointer = U *;
    using reference = U &;
    using iterator_category = std::bidirectional_iterator_tag;

    bool operator==(const Iterator &o) const { return _node == o._node; }
    bool operator!=(const Iterator &o) const { return _node != o._node; }

    Iterator &operator++() { // advance one (pre)
      _node = _node->_next;
      return *this;
    }
    Iterator operator++(int) { // advance one (post)
      Iterator it = *this;
      _node = _node->_next;
      return it;
    }
    Iterator &operator--() { // retreat one (pre)
      _node = _node->_prev;
      return *this;
    }
    Iterator operator--(int) { // retreat one (post)
      Iterator it = *this;
      _node = _node->_prev;
      return it;
    }

    U &operator*() const { return _node->_data; }   // the current element
    U *operator->() const { return &_node->_data; } // member access on it
  };

  using iterator = Iterator<T>;
  using const_iterator = Iterator<const T>;

  List() : _head(sentinel()), _n(0) {}

  List(const unsigned n) : _head(sentinel()), _n(0) {
    for (unsigned i = 0; i < n; i++)
      link(_head->_prev, new Node(0, 0, 0));
    _n = n;
  }

  List(const T &o, const unsigned n) : _head(sentinel()), _n(0) {
    for (unsigned i = 0; i < n; i++)
      link(_head->_prev, new Node(o, 0, 0));
    _n = n;
  }

  List(const List &rhs) : _head(sentinel()), _n(0) { copy(rhs); }

  List(List &&o) noexcept // steal o's ring; o left empty
      : _head(o._head), _n(o._n) {
    o._head = sentinel();
    o._n = 0;
  }

  ~List() {
    clear();
    delete _head;
  }

  List &operator=(const List &rhs) {
    copy(rhs);
    return *this;
  }

  List &operator=(List &&o) noexcept { // steal o's ring
    if (this != &o) {
      clear();
      delete _head;
      _head = o._head;
      _n = o._n;
      o._head = sentinel();
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both rings.
  void swap(List &o) {
    Node *h = _head;
    unsigned n = _n;
    _head = o._head;
    _n = o._n;
    o._head = h;
    o._n = n;
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void resize(const unsigned n) {
    while (_n < n) // grow with default-constructed elements
      link(_head->_prev, new Node(0, 0, 0));
    while (_n > n) // shrink from the back
      unlink(_head->_prev);
  }

  void clear() {
    Node *ptr = _head->_next;
    while (ptr != _head) {
      Node *tmp = ptr;
      ptr = ptr->_next;
      delete tmp;
    }
    _head->_next = _head->_prev = _head; // back to the empty ring
    _n = 0;
  }

  void push_back(const T &d) { link(_head->_prev, new Node(d, 0, 0)); }

  void pop_back() {
    if (!empty())
      unlink(_head->_prev);
  }

  const T &back() const { return (_head->_prev->_data); }

  void push_front(const T &d) { link(_head, new Node(d, 0, 0)); }

  void pop_front() {
    if (!empty())
      unlink(_head->_next);
  }

  const T &front() const { return (_head->_next->_data); }

  // Inserts d before loc (at the end when loc == end(), and into an
  // empty list when loc == begin() == end()). Returns an iterator to
  // the new element.
  iterator insert(const iterator &loc, const T &d) {
    // end() is the sentinel: inserting "before end" appends.
    Node *prev = (loc._node == _head) ? _head->_prev : loc._node->_prev;
    return iterator(link(prev, new Node(d, 0, 0)));
  }

  // Erases the element at loc (end() is a no-op returning end()).
  // Returns an iterator to the element that followed it.
  iterator erase(const iterator &loc) {
    if (loc._node == _head)
      return end();

    return iterator(unlink(loc._node));
  }

  iterator begin() { return iterator(_head->_next); }
  const_iterator begin() const { return const_iterator(_head->_next); }

  // The sentinel is the past-the-end position; on an empty list,
  // begin() == end().
  iterator end() { return iterator(_head); }
  const_iterator end() const { return const_iterator(_head); }
};

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T> void swap(List<T> &a, List<T> &b) { a.swap(b); }

// Prints the elements separated by ", " with no trailing separator.
template <typename T>
std::ostream &operator<<(std::ostream &os, const List<T> &rhs) {
  bool first = true;
  for (typename List<T>::const_iterator it = rhs.begin(); it != rhs.end();
       ++it) {
    if (!first)
      os << ", ";
    os << *it;
    first = false;
  }

  return (os);
}
