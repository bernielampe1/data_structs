#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "Exception.h"
#include "types.h"

// Trie<V>: an ordered map from string keys to values, stored as a
// character tree.
//
// Traversal order is ascending lexicographic by key (deterministic, and
// the natural order for a trie). Each node holds its children in a
// sorted dynamic array of (edge char, node*) pairs -- binary-searched,
// doubling on demand -- so lookup along a path of length L costs
// O(L log C) with C the branching factor.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// insert(k, v) inserts or replaces and returns true when k was new;
// a key mapping through a longer existing key ("in" inside "inn") is
// fine: _end marks which nodes terminate a key. find(k) returns a
// reference and throws Exception for a missing key (same contract as
// Map::find / HashTable::find); operator[] inserts a default V when
// absent (non-const) and throws when absent (const). erase(k) unmaps a
// key and prunes now-useless nodes, reporting whether anything was
// erased. exists(k) reports membership; hasPrefix(p) reports whether
// ANY key starts with p.
//
// Value requirements: V must be default-constructible and
// copy-assignable (interior nodes carry default values).
template <typename V> class Trie;
template <typename V>
std::ostream &operator<<(std::ostream &, const Trie<V> &);

template <typename V> class Trie {
private:
  struct Node {
    V _value;   // meaningful when _end
    bool _end;  // a key terminates at this node
    // children: sorted array of (char, node*) pairs, doubling storage
    struct Child {
      Child() : _c(0), _node(0) {} // default needed for new Child[n]
      Child(char c, Node *n) : _c(c), _node(n) {}
      char _c;
      Node *_node;
    };
    Child *_kids;    // ascending by _c; null when none yet
    u64 _nkids;      // used child count
    u64 _kidCap;     // allocated slots

    Node() : _value(), _end(false), _kids(0), _nkids(0), _kidCap(0) {}

    ~Node() {
      for (u64 i = 0; i < _nkids; i++)
        delete _kids[i]._node;
      delete[] _kids;
    }

    // Binary search: returns the index of _c's slot or, when absent,
    // the insertion point (lower bound).
    u64 locate(char c) const {
      u64 lo = 0, hi = _nkids;
      while (lo < hi) {
        u64 mid = lo + (hi - lo) / 2;
        if (_kids[mid]._c < c)
          lo = mid + 1;
        else
          hi = mid;
      }
      return lo;
    }

    Node *child(char c) {
      u64 i = locate(c);
      return (i < _nkids && _kids[i]._c == c) ? _kids[i]._node : 0;
    }

    const Node *child(char c) const {
      u64 i = locate(c);
      return (i < _nkids && _kids[i]._c == c) ? _kids[i]._node : 0;
    }

    // Creates (or returns the existing) child on edge c, keeping the
    // array ascending.
    Node *childOrCreate(char c) {
      u64 i = locate(c);
      if (i < _nkids && _kids[i]._c == c)
        return _kids[i]._node;

      if (_nkids == _kidCap) {
        u64 newCap = _kidCap ? _kidCap * 2 : 4;
        Child *fresh = new Child[newCap];
        for (u64 j = 0; j < _nkids; j++)
          fresh[j] = _kids[j];
        delete[] _kids;
        _kids = fresh;
        _kidCap = newCap;
      }

      // shift [i, _nkids) right, from the back
      for (u64 j = _nkids; j > i; j--)
        _kids[j] = _kids[j - 1];
      _kids[i] = Child(c, new Node());
      _nkids++;
      return _kids[i]._node;
    }

    // Removes the child on edge c (must exist); returns nothing.
    void removeChild(char c) {
      u64 i = locate(c);
      if (i >= _nkids || _kids[i]._c != c)
        return;

      delete _kids[i]._node;
      for (u64 j = i; j + 1 < _nkids; j++)
        _kids[j] = _kids[j + 1];
      _nkids--;
    }
  };

  Node *_root;
  u64 _n; // number of mapped keys

  // Walks the path for key k, materializing missing nodes only when
  // create is set. Returns the terminal node, or null when absent.
  Node *walk(const std::string &k, bool create) {
    Node *cur = _root;
    for (u64 i = 0; i < k.size(); i++) {
      if (create)
        cur = cur->childOrCreate(k[i]);
      else {
        cur = cur->child(k[i]);
        if (!cur)
          return 0;
      }
    }
    return cur;
  }

  const Node *walk(const std::string &k) const {
    const Node *cur = _root;
    for (u64 i = 0; i < k.size(); i++) {
      cur = cur->child(k[i]);
      if (!cur)
        return 0;
    }
    return cur;
  }

  // Recursive preorder walk: emits _end nodes in lexicographic order.
  template <typename Func>
  static void dfs(const Node *n, std::string &path, Func f) {
    if (n->_end)
      f(path, n->_value);

    for (u64 i = 0; i < n->_nkids; i++) {
      path.push_back(n->_kids[i]._c);
      dfs(n->_kids[i]._node, path, f);
      path.pop_back();
    }
  }

  // Recursive prune: deletes nodes with no children and no _end. This
  // node survives when it keeps a child or terminates a key; the
  // caller unlinks it otherwise.
  static void prune(Node *n) {
    for (u64 i = 0; i < n->_nkids;) {
      prune(n->_kids[i]._node);
      if (n->_kids[i]._node->_nkids == 0 && !n->_kids[i]._node->_end) {
        n->removeChild(n->_kids[i]._c); // shifts the tail left
      } else {
        i++; // child kept its subtree
      }
    }
  }

  static Node *clone(const Node *src) {
    Node *node = new Node();
    node->_value = src->_value;
    node->_end = src->_end;
    node->_nkids = src->_nkids;
    node->_kidCap = src->_nkids ? src->_nkids : 0;
    if (src->_nkids) {
      node->_kids = new typename Node::Child[src->_nkids];
      for (u64 i = 0; i < src->_nkids; i++) {
        node->_kids[i]._c = src->_kids[i]._c;
        node->_kids[i]._node = clone(src->_kids[i]._node);
      }
    } else {
      node->_kids = 0;
    }
    return node;
  }

public:
  typedef u64 size_type; // key counts, up to 64 bits

  Trie() : _root(new Node()), _n(0) {}

  // Deep copy: same keys, same values, same trie shape.
  Trie(const Trie &o) : _root(clone(o._root)), _n(o._n) {}

  // Steal o's tree; o left empty and reusable.
  Trie(Trie &&o) noexcept : _root(o._root), _n(o._n) {
    o._root = new Node();
    o._n = 0;
  }

  ~Trie() { delete _root; }

  // Deep copy: the new tree is built before the old one is freed, so a
  // failed allocation leaves *this untouched.
  Trie &operator=(const Trie &o) {
    if (this == &o)
      return *this;

    Node *newRoot = clone(o._root); // may throw: *this untouched

    delete _root;
    _root = newRoot;
    _n = o._n;

    return *this;
  }

  // Steal o's tree, freeing ours first. Self-move guarded (the old
  // swap-then-delete pattern would zero our own tree).
  Trie &operator=(Trie &&o) noexcept {
    if (this != &o) {
      delete _root;
      _root = o._root;
      _n = o._n;
      o._root = new Node();
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both trees.
  void swap(Trie &o) {
    std::swap(_root, o._root);
    std::swap(_n, o._n);
  }

  size_type size() const { return _n; } // mapped key count

  bool empty() const { return _n == 0; }

  // Stores value under key. True when key was new; false when an
  // existing mapping was replaced.
  bool insert(const std::string &k, const V &v) {
    Node *n = walk(k, true);
    bool wasNew = !n->_end;
    n->_value = v;
    n->_end = true;
    if (wasNew)
      _n++;
    return wasNew;
  }

  // True when k is mapped.
  bool exists(const std::string &k) const {
    const Node *n = walk(k);
    return n && n->_end;
  }

  // Reference to the value stored under k. Throws Exception when k is
  // absent (same contract as Map::find).
  V &find(const std::string &k) {
    Node *n = walk(k, false);
    if (!n || !n->_end)
      throw Exception("Trie::find: key not found");
    return n->_value;
  }

  const V &find(const std::string &k) const {
    const Node *n = walk(k);
    if (!n || !n->_end)
      throw Exception("Trie::find: key not found");
    return n->_value;
  }

  // Read/write access by key: reading an absent key inserts a default
  // V (std::map's operator[] contract); the const overload reports an
  // absent key by throwing.
  V &operator[](const std::string &k) {
    Node *n = walk(k, true);
    if (!n->_end) {
      n->_end = true;
      n->_value = V();
      _n++;
    }
    return n->_value;
  }

  const V &operator[](const std::string &k) const {
    const Node *n = walk(k);
    if (!n || !n->_end)
      throw Exception("Trie: key not found");
    return n->_value;
  }

  // Unmaps k (removing it does not touch keys that EXTEND k, such as
  // erasing "in" while "inn" stays), pruning dead nodes, and reports
  // whether anything was erased.
  bool erase(const std::string &k) {
    Node *n = walk(k, false);
    if (!n || !n->_end)
      return false;

    n->_end = false;
    n->_value = V();
    _n--;

    prune(_root); // drop nodes no longer on any key's path
    return true;
  }

  // True when at least one mapped key starts with the prefix p (the
  // operation that sets a trie apart from a hash table).
  bool hasPrefix(const std::string &p) const {
    const Node *n = walk(p);
    return n != 0;
  }

  // In-order traversal: f(key, value) for every mapped key,
  // lexicographically ascending.
  template <typename Func> void for_each(Func f) const {
    std::string path;
    dfs(_root, path, f);
  }

  // Traversal restricted to keys starting with the prefix p: f(key,
  // value) with key = p + suffix, ascending.
  template <typename Func>
  void prefix_each(const std::string &p, Func f) const {
    const Node *start = walk(p);
    if (!start)
      return;

    std::string path = p;
    dfs(start, path, f);
  }

  // Releases every key; the trie becomes empty and reusable.
  void clear() {
    delete _root;
    _root = new Node();
    _n = 0;
  }

  // True when both tries map exactly the same keys to the same values.
  bool operator==(const Trie &o) const {
    if (_n != o._n)
      return false;
    bool same = true;
    for_each([&](const std::string &k, const V &v) {
      try {
        if (o.find(k) != v)
          same = false;
      } catch (Exception &) {
        same = false;
      }
    });
    return same;
  }

  bool operator!=(const Trie &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os, const Trie<V> &t);
};

// Prints the entries lexicographically ascending, one "key -> value"
// line per key (Map's printer format); an empty trie prints nothing.
template <typename V>
std::ostream &operator<<(std::ostream &os, const Trie<V> &t) {
  t.for_each([&os](const std::string &k, const V &v) {
    os << k << " -> " << v << "\n";
  });
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename V> void swap(Trie<V> &a, Trie<V> &b) { a.swap(b); }
