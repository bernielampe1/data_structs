#pragma once

enum Color { RED, BLACK };

template< typename T >
struct Node<T> {
  T _data;
  Node<T> *left, *right, *parent;
  int color;

  Node(const T d) {
    _data = d;
    left = right = parent = 0;
    color = RED;
  }
};

template< typename T >
class RBTree<T> {
  private:
    Node *_root;
    unsigned _size;

    void _rotateleft(Node **, Node **);
    void _rotateright(Node **, Node **);
    void _fixup(Node **, Node **);

    T& _find();
    T& _insert();
    bool _exists();
    void _clear(Node **n);

  public:
    RBTree(): _root(0), _size(0) { }

    RBTree(const RBTree &o): _root(0), _size(o._size) {
      // TODO
    }

    RBTree& operator=(const RBTree &o) {
      // TODO
    }

    ~RbTree() { clear(); }

    void insert(const T &d) {
      _insert(&_root, d);
      _size++;
    }

    unsigned size() const { return _size; }

    unsigned empty() const { return _size == 0; }

    bool exist(const T &d) const { return _exists(_root, d); }

    const T& find(const T &d) const { return _find(_root, d); }

    void clear() { _clear(&_root); }
};
