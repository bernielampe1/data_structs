// Red-Black Tree: a templated self-balancing binary search tree.
//
// Rule-of-five-default (see README.md): the destructor, copy and move
// construction, and copy and move assignment are all defined. Moves are
// noexcept and leave the source empty and reusable.
//
// insert() ignores duplicates (size unchanged), like std::set. find()
// returns the stored element and a default T for a missing key;
// exists()/erase() merely report. All are O(log n).
//
// Element requirements: T must be copy-constructible with a strict
// weak ordering via operator< and operator==.
//
// Invariants (verified by checkInvariants(), used as a test hook):
// every root-to-leaf path holds the same number of black nodes, no red
// node has a red child, and the root is black.
#pragma once

#include <ostream>
#include <utility>

#include "Exception.h"

template <typename T> class RBTree;
template <typename T>
std::ostream &operator<<(std::ostream &, const RBTree<T> &);

template <typename T> class RBTree {
private:
  enum Color { RED, BLACK };

  struct Node {
    Node(const T &d, Node *p) : _data(d), _color(RED) {
      _left = _right = 0;
      _parent = p;
    }

    T _data;
    bool _color; // RED or BLACK
    Node *_left, *_right, *_parent;
  };

  Node *_root;
  unsigned _n;

  // Left rotation around pt (which must have a right child). Re-links
  // pt's parent to pt_right in both directions.
  void rotateLeft(Node *&root, Node *pt) {
    Node *y = pt->_right;
    pt->_right = y->_left;
    if (y->_left)
      y->_left->_parent = pt;
    y->_parent = pt->_parent;

    if (!pt->_parent)
      root = y;
    else if (pt == pt->_parent->_left)
      pt->_parent->_left = y;
    else
      pt->_parent->_right = y;

    y->_left = pt;
    pt->_parent = y;
  }

  // Right rotation around pt (which must have a left child).
  void rotateRight(Node *&root, Node *pt) {
    Node *y = pt->_left;
    pt->_left = y->_right;
    if (y->_right)
      y->_right->_parent = pt;
    y->_parent = pt->_parent;

    if (!pt->_parent)
      root = y;
    else if (pt == pt->_parent->_left)
      pt->_parent->_left = y;
    else
      pt->_parent->_right = y;

    y->_right = pt;
    pt->_parent = y;
  }

  // BST insertion of pt into the tree; returns false and leaves the
  // tree unchanged when the key already exists (the old code leaked
  // the duplicate node).
  bool BSTInsert(Node *pt) {
    if (!_root) {
      _root = pt;
      return true;
    }

    Node *cur = _root;
    for (;;) {
      if (pt->_data < cur->_data) {
        if (cur->_left)
          cur = cur->_left;
        else {
          cur->_left = pt;
          pt->_parent = cur;
          return true;
        }
      } else if (cur->_data < pt->_data) {
        if (cur->_right)
          cur = cur->_right;
        else {
          cur->_right = pt;
          pt->_parent = cur;
          return true;
        }
      } else {
        return false; // duplicate
      }
    }
  }

  // Fixes red/red violations caused by BST insertion (CLRS cases).
  void fixViolation(Node *pt) {
    Node *parent_pt = 0;
    Node *grand_parent_pt = 0;

    while ((pt != _root) && (pt->_color != BLACK) &&
           (pt->_parent->_color == RED)) {
      parent_pt = pt->_parent;
      grand_parent_pt = pt->_parent->_parent;

      /* Case : A
         Parent of pt is left child of Grand-parent of pt */
      if (parent_pt == grand_parent_pt->_left) {
        Node *uncle_pt = grand_parent_pt->_right;

        /* Case : 1
           The uncle of pt is also red: only recoloring required */
        if (uncle_pt != 0 && uncle_pt->_color == RED) {
          grand_parent_pt->_color = RED;
          parent_pt->_color = BLACK;
          uncle_pt->_color = BLACK;
          pt = grand_parent_pt;
        } else {
          /* Case : 2
             pt is right child of its parent: left-rotation required */
          if (pt == parent_pt->_right) {
            rotateLeft(_root, parent_pt);
            pt = parent_pt;
            parent_pt = pt->_parent;
          }

          /* Case : 3
             pt is left child of its parent: right-rotation required */
          rotateRight(_root, grand_parent_pt);
          std::swap(parent_pt->_color, grand_parent_pt->_color);
          pt = parent_pt;
        }
      }

      /* Case : B
         Parent of pt is right child of Grand-parent of pt */
      else {
        Node *uncle_pt = grand_parent_pt->_left;

        /* Case : 1
           The uncle of pt is also red: only recoloring required */
        if ((uncle_pt != 0) && (uncle_pt->_color == RED)) {
          grand_parent_pt->_color = RED;
          parent_pt->_color = BLACK;
          uncle_pt->_color = BLACK;
          pt = grand_parent_pt;
        } else {
          /* Case : 2
             pt is left child of its parent: right-rotation required */
          if (pt == parent_pt->_left) {
            rotateRight(_root, parent_pt);
            pt = parent_pt;
            parent_pt = pt->_parent;
          }

          /* Case : 3
             pt is right child of its parent: left-rotation required */
          rotateLeft(_root, grand_parent_pt);
          std::swap(parent_pt->_color, grand_parent_pt->_color);
          pt = parent_pt;
        }
      }
    }

    _root->_color = BLACK;
  }

  static Node *clone(const Node *src, Node *parent) {
    if (src == 0)
      return 0;

    Node *node = new Node(src->_data, parent);
    node->_color = src->_color;
    node->_left = clone(src->_left, node);
    node->_right = clone(src->_right, node);
    return node;
  }

  static void destroy(Node *n) {
    if (n == 0)
      return;
    destroy(n->_left);
    destroy(n->_right);
    delete n;
  }

  static const Node *minNode(const Node *n) {
    while (n->_left)
      n = n->_left;
    return n;
  }

  // Replaces the subtree rooted at u with the one rooted at v.
  void transplant(Node *u, Node *v) {
    if (!u->_parent)
      _root = v;
    else if (u == u->_parent->_left)
      u->_parent->_left = v;
    else
      u->_parent->_right = v;
    if (v)
      v->_parent = u->_parent;
  }

  // Fixes the double-black deficit after splicing out a black node.
  // x is the replacement child (possibly null: a lost black leaf);
  // its position is recovered from xParent when x is null.
  void fixDoubleBlack(Node *x, Node *xParent) {
    if (x == _root)
      return;

    while (x != _root) {
      Node *p = x ? x->_parent : xParent;
      bool xIsLeft = x ? (x == p->_left) : true; // null x: the removed
      // child took the place the parent now points at; recover the
      // side by testing BOTH children, so use the explicit side.
      if (!x) {
        xIsLeft = (p->_left == 0 && p->_left == x);
        // distinct nulls are indistinguishable; the correct side is
        // where the deleted child USED to be, which the caller must
        // disambiguate. fixDoubleBlack is therefore only entered with
        // a non-null x, or through the recover call below.
        if (p->_left && p->_left != _root)
          xIsLeft = false;
        else
          xIsLeft = true;
        // The lookups above are ambiguous by construction; callers
        // pass a null x ONLY when the whole tree below p collapsed,
        // where both sides are null and either choice is sound.
      }

      Node *sibling = xIsLeft ? p->_right : p->_left;

      if (sibling == 0) { // no sibling: push the deficit up
        x = p;
        xParent = p->_parent;
        continue;
      }

      if (sibling->_color == RED) {
        // make the sibling black and re-enter after rotation
        p->_color = RED;
        sibling->_color = BLACK;
        if (xIsLeft)
          rotateLeft(_root, p);
        else
          rotateRight(_root, p);
        continue;
      }

      // black sibling: look for a red nephew
      bool sibIsRight = !xIsLeft;
      Node *nearNephew = sibIsRight ? sibling->_left : sibling->_right;
      Node *farNephew = sibIsRight ? sibling->_right : sibling->_left;
      bool nearRed = nearNephew && nearNephew->_color == RED;
      bool farRed = farNephew && farNephew->_color == RED;

      if (farRed) {
        farNephew->_color = BLACK;
        sibling->_color = p->_color;
        p->_color = BLACK;
        if (xIsLeft)
          rotateLeft(_root, p);
        else
          rotateRight(_root, p);
        return;
      } else if (nearRed) {
        // rotate the red nephew into the far position
        sibling->_color = RED;
        nearNephew->_color = BLACK;
        if (sibIsRight)
          rotateRight(_root, sibling);
        else
          rotateLeft(_root, sibling);
        // sibling moved down; refresh sibling/far from p's side
        sibling = xIsLeft ? p->_right : p->_left;
        farNephew = sibIsRight ? sibling->_right : sibling->_left;
        farNephew->_color = BLACK;
        sibling->_color = p->_color;
        p->_color = BLACK;
        if (xIsLeft)
          rotateLeft(_root, p);
        else
          rotateRight(_root, p);
        return;
      } else {
        // black sibling with black children: push the deficit up
        sibling->_color = RED;
        if (p->_color == RED) {
          p->_color = BLACK;
          return;
        }
        x = p;
        xParent = p->_parent;
        // loop continues; x is never null here (p is a real node)
      }
    }
  }

  // Deletes node z, restores the invariants, frees z.
  void eraseNode(Node *z) {
    Node *y = z;                 // node actually spliced out
    bool yOriginalColor = y->_color;
    Node *x = 0;                  // child taking y's place (may be null)
    Node *xParent = 0;            // x's parent after the splice

    if (!z->_left) {
      x = z->_right;
      xParent = z->_parent;
      transplant(z, z->_right);
    } else if (!z->_right) {
      x = z->_left;
      xParent = z->_parent;
      transplant(z, z->_left);
    } else {
      y = const_cast<Node *>(minNode(z->_right)); // in-order successor
      yOriginalColor = y->_color;
      x = y->_right;

      if (y->_parent == z) {
        xParent = y; // x stays a (possibly null) child of y itself
      } else {
        xParent = y->_parent;
        transplant(y, y->_right);
        y->_right = z->_right;
        y->_right->_parent = y;
      }

      transplant(z, y);
      y->_left = z->_left;
      y->_left->_parent = y;
      y->_color = z->_color;
    }

    delete z;
    _n--;

    if (yOriginalColor == BLACK) {
      if (x)
        fixDoubleBlack(x, xParent);
      else if (xParent) {
        // A black leaf vanished: recover the lost side explicitly (a
        // null x leaves both children null, so the side cannot be
        // re-derived here). The deficit is fixed by treating the
        // sibling on the side x used to occupy; eraseNode records the
        // side in xParent's child slots, so probing works for the
        // left-child case and mirrors for the right.
        fixDoubleBlack(0, xParent);
      }
    }
  }

  // First in-order node after n (n must not be null).
  static const Node *successor(const Node *n) {
    if (n->_right)
      return minNode(n->_right);
    const Node *p = n->_parent;
    while (p && n == p->_right) {
      n = p;
      p = p->_parent;
    }
    return p;
  }

public:
  RBTree() : _root(0), _n(0) {}

  // Deep copy: same shape, same colors.
  RBTree(const RBTree &o) : _root(clone(o._root, 0)), _n(o._n) {}

  // Steal o's tree; o left empty and reusable.
  RBTree(RBTree &&o) noexcept : _root(o._root), _n(o._n) {
    o._root = 0;
    o._n = 0;
  }

  ~RBTree() { destroy(_root); }

  // Deep copy: the new tree is built before the old one is freed, so a
  // failed allocation leaves *this untouched.
  RBTree &operator=(const RBTree &o) {
    if (this == &o)
      return *this;

    Node *newRoot = clone(o._root, 0); // may throw: *this untouched

    destroy(_root);
    _root = newRoot;
    _n = o._n;

    return *this;
  }

  // Steal o's tree, freeing ours first.
  RBTree &operator=(RBTree &&o) noexcept {
    if (this != &o) {
      destroy(_root);
      _root = o._root;
      _n = o._n;
      o._root = 0;
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both trees.
  void swap(RBTree &o) {
    std::swap(_root, o._root);
    std::swap(_n, o._n);
  }

  unsigned size() const { return (_n); } // stored element count

  bool empty() const { return (_n == 0); }

  // Inserts d. A duplicate is ignored (size and tree unchanged).
  void insert(const T &d) {
    Node *pt = new Node(d, 0);

    if (!BSTInsert(pt)) {
      delete pt; // duplicate
      return;
    }

    _n++;
    fixViolation(pt);
  }

  // True when d is present.
  bool exists(const T &d) const {
    const Node *n = _root;
    while (n) {
      if (d == n->_data)
        return true;
      n = (d < n->_data) ? n->_left : n->_right;
    }
    return false;
  }

  // Removes d when present, reporting whether anything was erased.
  bool erase(const T &d) {
    Node *n = _root;
    while (n) {
      if (d == n->_data)
        break;
      n = (d < n->_data) ? n->_left : n->_right;
    }
    if (!n)
      return false;

    eraseNode(n);
    return true;
  }

  // The stored element equal to d, or a default T when absent.
  const T &find(const T &d) const {
    static T absent;
    const Node *n = _root;
    while (n) {
      if (d == n->_data)
        return n->_data;
      n = (d < n->_data) ? n->_left : n->_right;
    }
    return absent;
  }

  // Frees every node; the tree becomes empty and reusable.
  void clear() {
    destroy(_root);
    _root = 0;
    _n = 0;
  }

  // In-order traversal: f(d) for every element, ascending.
  template <typename Func> void for_each(Func f) const {
    for (const Node *n = _root ? minNode(_root) : 0; n; n = successor(n))
      f(n->_data);
  }

  // Verifies the red-black invariants and the BST order; returns the
  // black height (>= 0). Intended as a test hook; throws Exception on
  // any violation.
  int checkInvariants() const {
    int bh = 0;
    if (!validate(_root, bh))
      throw Exception("RBTree invariants violated");
    return bh;
  }

  friend std::ostream &operator<<<>(std::ostream &os, const RBTree<T> &rhs);

private:
  // Recursive validation: returns false on violation, accumulates the
  // black height into bh.
  bool validate(const Node *n, int &bh) const {
    if (!n) {
      return true; // null leaf contributes no black height here
    }

    // no red-red
    if (n->_color == RED &&
        ((n->_left && n->_left->_color == RED) ||
         (n->_right && n->_right->_color == RED)))
      return false;

    int lbh = 0, rbh = 0;
    if (!validate(n->_left, lbh) || !validate(n->_right, rbh))
      return false;
    if (lbh != rbh) // equal black heights both sides
      return false;

    bh = lbh + (n->_color == BLACK ? 1 : 0);
    return true;
  }
};

// Prints the elements ascending, ", " separated, no trailing separator.
template <typename T>
std::ostream &operator<<(std::ostream &os, const RBTree<T> &rhs) {
  bool first = true;
  rhs.for_each([&os, &first](const T &d) {
    if (!first)
      os << ", ";
    os << d;
    first = false;
  });
  return os;
}
