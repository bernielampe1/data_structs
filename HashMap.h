#pragma once

#include <cstddef>
#include <functional>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "types.h"

template <typename K, typename V> class HashMap;
template <typename K, typename V>
std::ostream &operator<<(std::ostream &, const HashMap<K, V> &);

// HashMap<K, V>: an unordered map from K to V with open addressing.
//
// Uses std::hash<K> directly (K needs a std::hash specialization and
// operator==; V needs operator== for map comparison) -- the contract
// HashTable<K, T> supplies through an explicit function pointer.
//
// Collision handling is LINEAR PROBING over a power-of-two cell array
// (index = hash & (capacity - 1)), the contrasting technique to
// HashTable's separate chaining. Erased cells become tombstones so a
// probe chain never breaks; when occupied+tombstoned cells exceed a
// 0.7 load factor the table rehashes into a doubled array, dropping
// the tombstones. Insert/erase/exists/find are O(1) expected.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// insert(k, v) inserts or replaces and returns true when k was new
// (Map contract). find(k) returns a reference and throws Exception
// for a missing key; operator[] inserts a default V when absent
// (non-const) and throws when absent (const); erase/exists merely
// report. for_each(f) visits every mapping in BUCKET order -- which is
// not key order for an unordered container. operator== compares
// mappings, not cell placement.
template <typename K, typename V> class HashMap {
private:
  enum State { EMPTY, FULL, TOMB };

  struct Cell {
    K _key;
    V _value;
    State _state;
    Cell() : _state(EMPTY) {}
  };

  Cell *_cells;   // power-of-two sized; null when capacity is 0
  u64 _cap;       // cell count (power of two, or 0)
  u64 _n;         // mapped entries
  u64 _used;      // FULL + TOMB cells: what drives the load factor

  static const u64 MINCAP = 8; // smallest real capacity
  static const double MAXLOAD; // (full + tomb) / cap threshold

  u64 hashOf(const K &k) const { return u64(std::hash<K>()(k)); }

  // Locates k by probing. Returns the index of its FULL cell with
  // found = true; with found = false, the first TOMB index seen if any
  // (the reusable slot), else the first EMPTY index (append point).
  u64 locate(const K &k, bool &found) const {
    found = false;
    if (_cap == 0)
      return 0; // empty table: caller must ensure capacity first
    u64 mask = _cap - 1;
    u64 i = hashOf(k) & mask;
    u64 firstTomb = _cap; // sentinel: no tomb seen yet

    for (u64 probes = 0; probes < _cap; probes++) {
      const Cell &c = _cells[i];
      if (c._state == EMPTY)
        return firstTomb < _cap ? firstTomb : i;
      if (c._state == TOMB) {
        if (firstTomb == _cap)
          firstTomb = i;
      } else if (c._key == k) { // FULL and matching
        found = true;
        return i;
      }
      i = (i + 1) & mask;
    }

    // no EMPTY anywhere: table is saturated (rehash overdue); the
    // first tomb, if any, is the only place to write
    return firstTomb < _cap ? firstTomb : 0;
  }

  // Rebuilds into a doubled table with no tombstones.
  void rehash() {
    Cell *old = _cells;
    u64 oldCap = _cap;

    _cap = oldCap ? oldCap * 2 : MINCAP;
    _cells = new Cell[_cap];
    _used = _n; // only FULL cells survive a rehash

    u64 mask = _cap - 1;
    for (u64 i = 0; i < oldCap; i++) {
      if (old[i]._state != FULL)
        continue;

      u64 j = hashOf(old[i]._key) & mask;
      while (_cells[j]._state == FULL)
        j = (j + 1) & mask;
      _cells[j]._key = old[i]._key;
      _cells[j]._value = old[i]._value;
      _cells[j]._state = FULL;
    }

    delete[] old;
  }

public:
  typedef u64 size_type; // entry counts, up to 64 bits

  HashMap() : _cells(0), _cap(0), _n(0), _used(0) {}

  // Deep copy: same mappings, fresh cell array (rebuilt key by key, so
  // the copy has no tombstones).
  HashMap(const HashMap &o) : _cells(0), _cap(o._cap), _n(o._n), _used(o._n) {
    if (o._cap) {
      _cells = new Cell[o._cap];
      u64 mask = _cap - 1;
      for (u64 i = 0; i < o._cap; i++) {
        if (o._cells[i]._state != FULL)
          continue;

        u64 j = hashOf(o._cells[i]._key) & mask;
        while (_cells[j]._state == FULL)
          j = (j + 1) & mask;
        _cells[j]._key = o._cells[i]._key;
        _cells[j]._value = o._cells[i]._value;
        _cells[j]._state = FULL;
      }
    }
  }

  // Steal o's cells; o left empty and reusable.
  HashMap(HashMap &&o) noexcept
      : _cells(o._cells), _cap(o._cap), _n(o._n), _used(o._used) {
    o._cells = 0;
    o._cap = 0;
    o._n = 0;
    o._used = 0;
  }

  ~HashMap() { delete[] _cells; }

  // Deep copy: the new table is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  HashMap &operator=(const HashMap &o) {
    if (this == &o)
      return *this;

    Cell *fresh = o._cap ? new Cell[o._cap] : 0;
    if (fresh) {
      u64 mask = o._cap - 1;
      for (u64 i = 0; i < o._cap; i++) {
        if (o._cells[i]._state != FULL)
          continue;

        u64 j = hashOf(o._cells[i]._key) & mask;
        while (fresh[j]._state == FULL)
          j = (j + 1) & mask;
        fresh[j]._key = o._cells[i]._key;
        fresh[j]._value = o._cells[i]._value;
        fresh[j]._state = FULL;
      }
    }

    delete[] _cells;
    _cells = fresh;
    _cap = o._cap;
    _n = o._n;
    _used = o._n;

    return *this;
  }

  // Steal o's cells (freeing ours, unconditionally).
  HashMap &operator=(HashMap &&o) noexcept {
    if (this != &o) {
      delete[] _cells;
      _cells = o._cells;
      _cap = o._cap;
      _n = o._n;
      _used = o._used;
      o._cells = 0;
      o._cap = 0;
      o._n = 0;
      o._used = 0;
    }
    return *this;
  }

  // Constant-time exchange of both tables.
  void swap(HashMap &o) {
    std::swap(_cells, o._cells);
    std::swap(_cap, o._cap);
    std::swap(_n, o._n);
    std::swap(_used, o._used);
  }

  size_type size() const { return _n; } // mapped entry count

  bool empty() const { return _n == 0; }

  size_type capacity() const { return _cap; } // cell count (power of 2)

  double load() const { // FULL + TOMB over capacity
    return _cap ? double(_used) / double(_cap) : 0.0;
  }

  // Stores value under key. True when key was new; false when an
  // existing mapping was replaced.
  bool insert(const K &k, const V &v) {
    if (_cap == 0 || _used + 1 > _cap * MAXLOAD)
      rehash();

    bool found = false;
    u64 idx = locate(k, found);
    if (found) {
      _cells[idx]._value = v;
      return false;
    }

    Cell &c = _cells[idx];
    if (c._state == TOMB)
      _used--; // the tomb is reclaimed, not added
    c._key = k;
    c._value = v;
    c._state = FULL;
    _n++;
    _used++;

    return true;
  }

  // Removes k when present, reporting whether anything was erased.
  bool erase(const K &k) {
    if (_cap == 0)
      return false;

    bool found = false;
    u64 idx = locate(k, found);
    if (!found)
      return false;

    _cells[idx]._state = TOMB; // keep the cell: probes must pass through
    _n--;
    // _used holds: the tomb still costs load factor space until the
    // next rehash compacts it away

    // rehash eagerly when the table is mostly tombstones
    if (_n < _used / 4)
      rehash();
    return true;
  }

  // True when k is mapped.
  bool exists(const K &k) const {
    if (_cap == 0)
      return false;
    bool found = false;
    locate(k, found);
    return found;
  }

  // Reference to the value stored under k. Throws Exception when k is
  // absent (same contract as Map::find / HashTable::find).
  V &find(const K &k) {
    if (_cap == 0)
      throw Exception("HashMap::find: key not found");
    bool found = false;
    u64 idx = locate(k, found);
    if (!found)
      throw Exception("HashMap::find: key not found");
    return _cells[idx]._value;
  }

  const V &find(const K &k) const {
    if (_cap == 0)
      throw Exception("HashMap::find: key not found");
    bool found = false;
    u64 idx = locate(k, found);
    if (!found)
      throw Exception("HashMap::find: key not found");
    return _cells[idx]._value;
  }

  // Read/write access by key: reading an absent key inserts a default
  // V; the const overload reports an absent key by throwing.
  //
  // Correctness note: insert() UNCONDITIONALLY writes its value
  // argument, so blindly calling insert(k, V()) here would clobber an
  // existing mapping with a default on every read. Locate first.
  V &operator[](const K &k) {
    if (_cap == 0) {
      insert(k, V()); // fresh table: insert sizes up and finds an EMPTY slot
      bool found = false;
      u64 idx = locate(k, found);
      return _cells[idx]._value;
    }

    bool found = false;
    u64 idx = locate(k, found);
    if (found)
      return _cells[idx]._value;

    // key absent on a non-empty table; insert may rehash when this
    // entry crosses the load factor, so re-locate afterwards
    insert(k, V());
    locate(k, found);
    return _cells[idx]._value;
  }

  const V &operator[](const K &k) const {
    if (_cap == 0)
      throw Exception("HashMap: key not found");
    bool found = false;
    u64 idx = locate(k, found);
    if (!found)
      throw Exception("HashMap: key not found");
    return _cells[idx]._value;
  }

  // Traversal in bucket order (not key order): f(key, value).
  template <typename Func> void for_each(Func f) const {
    for (u64 i = 0; i < _cap; i++)
      if (_cells[i]._state == FULL)
        f(_cells[i]._key, _cells[i]._value);
  }

  // Releases every mapping; the cell array and capacity are kept.
  void clear() {
    for (u64 i = 0; i < _cap; i++)
      _cells[i]._state = EMPTY;
    _n = _used = 0;
  }

  // Releases the cell array entirely; back to the fresh state.
  void hardReset() {
    delete[] _cells;
    _cells = 0;
    _cap = _n = _used = 0;
  }

  // True when both maps hold exactly the same (key, value) mappings --
  // cell placement is irrelevant.
  bool operator==(const HashMap &o) const {
    if (_n != o._n)
      return false;
    bool same = true;
    for_each([&](const K &k, const V &v) {
      if (!o.exists(k) || !(o.find(k) == v))
        same = false;
    });
    return same;
  }

  bool operator!=(const HashMap &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os,
                                    const HashMap<K, V> &m);
};

template <typename K, typename V>
const double HashMap<K, V>::MAXLOAD = 0.7;

// Prints the entries in bucket order, one "key -> value" line per
// mapping (Map's printer format); an empty map prints nothing.
template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const HashMap<K, V> &m) {
  m.for_each([&os](const K &k, const V &v) { os << k << " -> " << v << "\n"; });
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename K, typename V>
void swap(HashMap<K, V> &a, HashMap<K, V> &b) {
  a.swap(b);
}
