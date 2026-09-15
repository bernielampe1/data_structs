#pragma once

#include <cstddef>
#include <ostream>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Exception.h"
#include "types.h"

// SkipList<K, V, MAX_LEVEL>: a probabilistic ordered map from K to V.
//
// A rule-of-five implementation (see README.md): the destructor, copy
// and move construction, and copy and move assignment are all defined.
// The move operations hand the source a fresh empty list in exchange,
// so the source stays empty and reusable. For the same reason the
// replacement node is allocated BEFORE anything is mutated: the old
// move-assignment freed *this's head and aliased the source's before
// the replacement allocation, so a failed allocation left two lists
// sharing one node chain.
//
// insert() inserts a new key or REPLACES the value of an existing one.
// find() returns a reference and throws Exception for a missing key
// (same contract as HashTable::find); exists()/erase() merely report.
//
// Key requirements: K must support operator< and operator==. Heights
// are chosen geometrically (p = 0.5, capped at MAX_LEVEL) and do not
// affect observable behavior, only the expected O(log n) cost.
//
// clear() empty every chain; the list stays usable.
template <typename K, typename V, int MAX_LEVEL = 32> class SkipList {
private:
  struct Node {
    Node(int height, const K &k, const V &v)
        : _key(k), _value(v), _forward(height, 0) {}

    K _key;
    V _value;
    std::vector<Node *> _forward; // one pointer per tower height
  };

  Node *_head; // sentinel; never null after construction. Its tower is
               // MAX_LEVEL high; only the first _level entries are used.
  int _level;  // highest tower height currently in use (>= 1)
  u64 _count;

  // A fresh empty list's sentinel.
  static Node *emptyHead() { return new Node(MAX_LEVEL, K(), V()); }

  // Geometric tower height: 1 with probability 1/2, 2 with 1/4, ...
  // capped at MAX_LEVEL.
  int randomLevel() {
    static thread_local std::mt19937 rng{std::random_device{}()};
    static thread_local std::bernoulli_distribution coin(0.5);
    int lvl = 1;
    while (lvl < MAX_LEVEL && coin(rng))
      ++lvl;
    return lvl;
  }

  // Finds the last node before key at every level: after the walk,
  // _update[i] is the rightmost node whose key is < key at level i
  // (the sentinel where the chain starts). The scratch lives here so
  // insert/erase share one walk; _level never exceeds MAX_LEVEL.
  Node *_update[MAX_LEVEL];

  void track(const K &key) {
    Node *x = _head;
    for (int i = _level - 1; i >= 0; --i) {
      while (x->_forward[i] && x->_forward[i]->_key < key)
        x = x->_forward[i];
      _update[i] = x;
    }
  }

public:
  SkipList() : _head(emptyHead()), _level(1), _count(0) {}

  // Deep copy: same elements, same tower heights.
  //
  // Every clone is built before any cross-linking and the clone of
  // every forward target is guaranteed to exist (the level-0 chain is
  // complete), so the wiring is a pure lookup. On a failed clone the
  // partial towers are freed and the exception propagates with nothing
  // shared or leaked.
  SkipList(const SkipList &o)
      : _head(emptyHead()), _level(o._level), _count(o._count) {
    std::unordered_map<const Node *, Node *> clones;
    clones[o._head] = _head;

    try {
      // Clone the level-0 chain, preserving each node's height.
      for (const Node *s = o._head->_forward[0]; s; s = s->_forward[0]) {
        Node *c = new Node(int(s->_forward.size()), s->_key, s->_value);
        clones[s] = c;
      }

      // Wire the towers: clone's forward[l] is the clone of the
      // original's forward[l]. The level cap leaves the unused top of
      // our sentinel's tower null.
      for (typename std::unordered_map<const Node *, Node *>::value_type
               &pair : clones) {
        const Node *s = pair.first;
        Node *c = pair.second;
        int height = int(c->_forward.size());
        for (int l = 0; l < height && l < o._level; ++l) {
          // find(), NOT operator[]: a null forward on the original is
          // still null on the clone, and operator[] would INSERT a
          // (null, null) entry, corrupting the loop we are iterating.
          typename std::unordered_map<const Node *, Node *>::iterator
              it = clones.find(s->_forward[l]);
          if (s->_forward[l])
            c->_forward[l] = it->second;
        }
      }
    } catch (...) {
      for (typename std::unordered_map<const Node *, Node *>::value_type
               &pair : clones)
        if (pair.second != _head)
          delete pair.second;
      delete _head;
      throw;
    }
  }

  // Steal o's chains; o receives a fresh empty list (allocated first,
  // so a failed allocation leaves o intact).
  SkipList(SkipList &&o)
      : _head(emptyHead()), _level(1), _count(0) {
    swap(o);
  }

  ~SkipList() {
    clear();
    delete _head;
  }

  // Deep copy: the replacement is built completely before the old
  // chains are released, so a failed copy leaves *this untouched.
  SkipList &operator=(const SkipList &o) {
    if (this == &o)
      return *this;

    SkipList tmp(o); // may throw: *this untouched so far
    swap(tmp);       // tmp's destructor frees our old chains

    return *this;
  }

  // Steal o's chains. The replacement for o is allocated before any
  // mutation so a failed allocation leaves both lists intact
  // (regression: the old code aliased the two lists on that path).
  SkipList &operator=(SkipList &&o) {
    if (this != &o) {
      Node *fresh = emptyHead();

      clear();
      delete _head;
      _head = o._head;
      _level = o._level;
      _count = o._count;
      o._head = fresh;
      o._level = 1;
      o._count = 0;
    }
    return *this;
  }

  // Constant-time exchange of both lists.
  void swap(SkipList &o) {
    std::swap(_head, o._head);
    std::swap(_level, o._level);
    std::swap(_count, o._count);
  }

  u64 size() const { return (_count); } // stored pair count

  bool empty() const { return (_count == 0); }

  // Inserts (k, v), or replaces the stored value when k is present.
  void insert(const K &k, const V &v) {
    track(k);

    const Node *next = _update[0]->_forward[0];
    if (next && next->_key == k) {
      // update in place; const_cast is safe: x is reachable from the
      // mutable head chain.
      Node *x = const_cast<Node *>(next);
      x->_value = v;
      return;
    }

    int height = randomLevel();
    if (height > _level) {
      // Above the old level the only predecessor is the sentinel.
      for (int i = _level; i < height; ++i)
        _update[i] = _head;
      _level = height;
    }

    Node *node = new Node(height, k, v);
    for (int i = 0; i < height; ++i) {
      node->_forward[i] = _update[i]->_forward[i];
      _update[i]->_forward[i] = node;
    }
    ++_count;
  }

  bool exists(const K &k) const {
    const Node *x = _head;
    for (int i = _level - 1; i >= 0; --i) {
      while (x->_forward[i] && x->_forward[i]->_key < k)
        x = x->_forward[i];
    }
    x = x->_forward[0];
    return (x && x->_key == k);
  }

  // Reference to the value stored under k. Throws Exception when k is
  // absent (same contract as HashTable::find).
  V &find(const K &k) {
    Node *x = _head;
    for (int i = _level - 1; i >= 0; --i) {
      while (x->_forward[i] && x->_forward[i]->_key < k)
        x = x->_forward[i];
    }
    x = x->_forward[0];
    if (!x || !(x->_key == k))
      throw Exception("SkipList::find: key not found");
    return x->_value;
  }

  const V &find(const K &k) const {
    const Node *x = _head;
    for (int i = _level - 1; i >= 0; --i) {
      while (x->_forward[i] && x->_forward[i]->_key < k)
        x = x->_forward[i];
    }
    x = x->_forward[0];
    if (!x || !(x->_key == k))
      throw Exception("SkipList::find: key not found");
    return x->_value;
  }

  // Removes k when present, reporting whether anything was erased.
  bool erase(const K &k) {
    track(k);

    Node *x = _update[0]->_forward[0];
    if (!x || !(x->_key == k))
      return false;

    // Unlink x from every level it occupies. At the first level whose
    // predecessor differs, x is absent above it: stop.
    for (int i = 0; i < _level; ++i) {
      if (_update[i]->_forward[i] != x)
        break;
      _update[i]->_forward[i] = x->_forward[i];
    }
    delete x;
    --_count;

    // Shrink the used level while the top chain is empty.
    while (_level > 1 && _head->_forward[_level - 1] == 0)
      --_level;

    return true;
  }

  // Frees every node (but not the sentinel); the list stays usable.
  void clear() {
    Node *x = _head->_forward[0];
    while (x) {
      Node *next = x->_forward[0];
      delete x;
      x = next;
    }
    for (int i = 0; i < _level; ++i)
      _head->_forward[i] = 0;
    _level = 1;
    _count = 0;
  }

  // In-order traversal: f(key, value) for every pair, ascending by key.
  template <typename Func> void for_each(Func f) const {
    for (const Node *x = _head->_forward[0]; x; x = x->_forward[0])
      f(x->_key, x->_value);
  }

  friend std::ostream &operator<<(std::ostream &os, const SkipList &l) {
    l.for_each([&os](const K &k, const V &v) { os << k << " -> " << v << "\n"; });
    return os;
  }
};

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename K, typename V, int MAX_LEVEL>
void swap(SkipList<K, V, MAX_LEVEL> &a, SkipList<K, V, MAX_LEVEL> &b) {
  a.swap(b);
}
