#pragma once

template <typename T> class BinTree {
private:
  struct Node {
    Node(const T &d) : _left(0), _right(0), _data(d) {}

    Node *_left, *_right;
    T _data;
  };

private:
  Node *_root;
  unsigned _size;

  void _insert(Node **n, const T &d) {
    if (*n != 0) {
      if (d < (*n)->_data) {
        if ((*n)->_left == 0)
          (*n)->_left = new Node(d);
        else
          return (_insert(&((*n)->_left), d));
      } else if (d > (*n)->_data) {
        if ((*n)->_right == 0)
          (*n)->_right = new Node(d);
        else
          return (_insert(&((*n)->_right), d));
      }
    } else {
      *n = new Node(d);
    }
  }

  bool _exists(const Node *n, const T &d) const {
    if (n != 0) {
      if (d == n->_data) {
        return (true);
      }

      if (d < n->_data) {
        return (_exists(n->_left, d));
      } else if (d > n->_data) {
        return (_exists(n->_right, d));
      }
    }

    return (false);
  }

  const T &_find(const Node *n, const T &d) const {
    if (n != 0) {
      if (d == n->_data) {
        return (n->_data);
      }

      if (d < n->_data) {
        return (_find(n->_left, d));
      } else if (d > n->_data) {
        return (_find(n->_right, d));
      }
    }
  }

  void _clear(Node **n) {
    if ((*n)->_left)
      _clear(&((*n)->_left));
    if ((*n)->_right)
      _clear(&((*n)->_right));

    delete *n;
    *n = 0;
  }

  void _copynode(Node **dest, Node **src) {
    if (*src) {
      if (*dest != 0) {
        (*dest)->_data = (*src)->_data; // copy data over
      }
      else {
        *dest = new Node((*src)->_data);
      }

      if ((*src)->_left != 0) _copynode(&(*dest)->_left, &(*src)->_left);
      if ((*src)->_right != 0) _copynode(&(*dest)->_right, &(*src)->_right);
    }
  }

public:
  BinTree() : _root(0), _size(0) {}

  BinTree(const BinTree &o): _root(0), _size(o._size) {
    _copynode(&this->_root, &o._root);
  }

  BinTree& operator=(BinTree &o) {
    if (this != &o) {
        _copynode(&this->_root, &o._root);
        this->_size = o._size;
    }
    return *this;
  }

  ~BinTree() { clear(); }

  void insert(const T &d) {
    _insert(&_root, d);
    _size++;
  }

  bool exists(const T &d) const { return (_exists(_root, d)); }

  const T &find(const T &d) const { return (_find(_root, d)); }

  void clear() { _clear(&_root); }

  bool empty() const { return (_size == 0); }

  unsigned size() const { return (_size); }
};

#include<iostream>
template <typename T>
std::ostream &operator<<(std::ostream &os, const BinTree<T> &o) {
/*
  for (typename BinTree<T>::const_iterator it = o.begin(); it != o.end(); it++) {
    os << *it << ", ";
  }
*/

  return (os);
}
