#pragma once

#include <ostream>
#include <utility>

#include "types.h"

// BinTree<T>: an unbalanced binary search tree (no duplicates).
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// Element requirements: T must be default-constructible and
// copy-constructible with a strict weak ordering via operator< and
// operator== (insert and lookup compare with both).
//
// size() uses u64 (types.h): the node count is bounded by memory, not by
// a 32-bit counter.

template <typename T> class BinTree {
private:
  struct Node {
    Node(const T &d) : _left(0), _right(0), _data(d) {}

    Node *_left, *_right;
    T _data;
  };

  Node *_root;
  u64 _size;

  // Inserts d under *n, growing the subtree. Returns the number of
  // nodes added (0 when d was already present).
  u64 insert(Node **n, const T &d) {
    if (*n == 0) {
      *n = new Node(d);
      return 1;
    }

    if (d < (*n)->_data)
      return insert(&(*n)->_left, d);
    if ((*n)->_data < d)
      return insert(&(*n)->_right, d);

    return 0; // equal: already in the tree
  }

  // True when the subtree at n holds d.
  bool exists(const Node *n, const T &d) const {
    while (n != 0) {
      if (d == n->_data)
        return true;
      n = (d < n->_data) ? n->_left : n->_right;
    }
    return false;
  }

  // The subtree at n holds d; null when absent.
  const Node *find(const Node *n, const T &d) const {
    while (n != 0) {
      if (d == n->_data)
        return n;
      n = (d < n->_data) ? n->_left : n->_right;
    }
    return 0;
  }

  // Frees every node of the subtree at *n (safe on null).
  void clear(Node **n) {
    if (*n == 0)
      return;

    clear(&(*n)->_left);
    clear(&(*n)->_right);

    delete *n;
    *n = 0;
  }

  // Builds a copy of the subtree at src; null stays null.
  static Node *clone(const Node *src) {
    if (src == 0)
      return 0;

    Node *node = new Node(src->_data);
    node->_left = clone(src->_left);
    node->_right = clone(src->_right);
    return node;
  }

public:
  BinTree() : _root(0), _size(0) {}

  // Deep copy: same elements, same shape.
  BinTree(const BinTree &o) : _root(clone(o._root)), _size(o._size) {}

  // Steal o's nodes; o left empty.
  BinTree(BinTree &&o) noexcept : _root(o._root), _size(o._size) {
    o._root = 0;
    o._size = 0;
  }

  ~BinTree() { clear(&_root); }

  // Deep copy: the new tree is built before the old one is freed, so a
  // failed allocation leaves *this untouched. Replaces all contents.
  BinTree &operator=(const BinTree &o) {
    if (this == &o)
      return *this;

    Node *newRoot = clone(o._root); // may throw: *this untouched until here

    clear(&_root);
    _root = newRoot;
    _size = o._size;

    return *this;
  }

  // Steal o's nodes (freeing ours, unconditionally).
  BinTree &operator=(BinTree &&o) noexcept {
    if (this != &o) {
      clear(&_root);
      _root = o._root;
      _size = o._size;
      o._root = 0;
      o._size = 0;
    }
    return *this;
  }

  // Constant-time exchange of both trees.
  void swap(BinTree &o) {
    std::swap(_root, o._root);
    std::swap(_size, o._size);
  }

  // Inserts d. A duplicate is ignored (size unchanged).
  void insert(const T &d) { _size += insert(&_root, d); }

  // True when d is present.
  bool exists(const T &d) const { return exists(_root, d); }

  // The stored element equal to d, or a default T when absent.
  const T &find(const T &d) const {
    static T absent;
    const Node *n = find(_root, d);
    return n ? n->_data : absent;
  }

  // Frees every node; the tree becomes empty and reusable.
  void clear() {
    clear(&_root);
    _size = 0;
  }

  bool empty() const { return (_size == 0); }

  u64 size() const { return (_size); }
};

// Prints nothing: BinTree has no iteration order exposed yet. Kept so
// existing includes keep compiling; a real implementation belongs with
// in-order iterators.
template <typename T>
std::ostream &operator<<(std::ostream &os, const BinTree<T> &o) {
  (void)o; // no iteration order exposed yet
  return (os);
}
