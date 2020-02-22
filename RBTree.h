#ifndef __RBTREE_H__
#define __RBTREE_H__

enum Color { RED, BLACK };

template< typename T >
struct Node<T> {
  T _data;
  Node<T> *left, *right, *parent;
  bool color;

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

    void _rotateleft(Nodef **, Nodef **);
    void _rotateright(Nodef **, Nodef **);
    void _fixup(Nodef **, Nodef **);

  public:
    RBTree(): _root(0) { }

    T& insert();

};

#endif // __RBTREE_H__
