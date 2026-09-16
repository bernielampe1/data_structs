#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "types.h"

template <typename K, typename V> class Map;
template <typename K, typename V>
std::ostream &operator<<(std::ostream &, const Map<K, V> &);

// Map<K, V>: an ordered map from keys to values in a sorted dynamic
// array of (key, value) entries.
//
// Iteration order is ascending by key (deterministic, unlike a hash
// table). Backing store doubles on demand; insert/erase shift entries
// O(n) in exchange for cache-friendly storage and O(log n) lookup --
// the same trade the sorted-array Set makes.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// insert(k, v) INSERTS OR REPLACES (matching HashTable::insert) and
// returns true when k was new. erase(k)/exists(k) merely report.
// find(k) returns a reference to the stored value and throws Exception
// for a missing key (same contract as HashTable::find). Non-const
// operator[] inserts a default V when k is absent; the const overload
// throws instead. for_each(f) visits f(key, value) ascending by key.
//
// Key requirements: K must be copy-assignable with a strict weak
// ordering via operator<; V must be default-constructible (for
// operator[]) and copy-assignable.
//
// Iterator note: Map has no raw iterators (the flat buffer holds
// private entries); traversal is via for_each. Any mutation may
// reallocate, so only for_each/const access are coherent mid-use.
template <typename K, typename V> class Map {
private:
  struct Entry {
    Entry(const K &k, const V &v) : key(k), value(v) {}
    Entry() : key(), value() {}

    K key;
    V value;
  };

  Entry *_data;     // entry buffer, ascending by key; null when empty
  u64 _n;           // stored entry count
  u64 _capacity;    // allocated slot count

  // Binary search for k. Returns true and sets idx to the entry's
  // position when present; returns false and sets idx to the position
  // where k belongs (the insertion point) either way.
  bool locate(const K &k, u64 &idx) const {
    u64 lo = 0, hi = _n;
    while (lo < hi) {
      u64 mid = lo + (hi - lo) / 2;
      if (_data[mid].key < k)
        lo = mid + 1;
      else
        hi = mid;
    }
    idx = lo;
    return idx < _n && !(_data[idx].key < k) && !(k < _data[idx].key);
  }

  // Ensures capacity >= want, allocating a doubled buffer under an
  // RAII guard and moving the existing entries over (element-wise, so
  // a throwing copy never loses the old buffer).
  void grow(u64 want) {
    u64 newCap = _capacity ? _capacity : 8;
    while (newCap < want)
      newCap *= 2;

    std::unique_ptr<Entry[]> fresh(new Entry[newCap]);
    for (u64 i = 0; i < _n; i++)
      fresh[i] = _data[i];

    delete[] _data;
    _data = fresh.release();
    _capacity = newCap;
  }

public:
  typedef u64 size_type; // entry counts and indices, up to 64 bits

  Map() : _data(0), _n(0), _capacity(0) {}

  // Deep copy: same entries, same order.
  Map(const Map &o) : _data(0), _n(o._n), _capacity(o._n) {
    if (o._n) {
      std::unique_ptr<Entry[]> fresh(new Entry[o._n]);
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._data[i];
      _data = fresh.release();
    }
  }

  // Steal o's buffer; o left empty and reusable.
  Map(Map &&o) noexcept
      : _data(o._data), _n(o._n), _capacity(o._capacity) {
    o._data = 0;
    o._n = 0;
    o._capacity = 0;
  }

  ~Map() { delete[] _data; }

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  Map &operator=(const Map &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<Entry[]> fresh;
    if (o._n) {
      fresh.reset(new Entry[o._n]);
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._data[i];
    }

    delete[] _data;
    _data = fresh.release();
    _n = o._n;
    _capacity = o._n;

    return *this;
  }

  // Steal o's buffer (freeing ours, unconditionally).
  Map &operator=(Map &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _n = o._n;
      _capacity = o._capacity;
      o._data = 0;
      o._n = 0;
      o._capacity = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Map &o) {
    std::swap(_data, o._data);
    std::swap(_n, o._n);
    std::swap(_capacity, o._capacity);
  }

  size_type size() const { return _n; } // entry count

  bool empty() const { return _n == 0; } // no entries?

  size_type capacity() const { return _capacity; } // allocated slots

  // Stores value under key. Returns true when key was new; false when
  // an existing mapping was replaced (a duplicate never grows the map).
  bool insert(const K &k, const V &v) {
    u64 idx;
    if (locate(k, idx)) {
      _data[idx].value = v; // replace in place
      return false;
    }

    if (_n == _capacity)
      grow(_n + 1);

    // shift the tail [idx, _n) right by one, from the back
    for (u64 i = _n; i > idx; i--)
      _data[i] = _data[i - 1];
    _data[idx] = Entry(k, v);
    _n++;

    return true;
  }

  // Removes k when present, reporting whether anything was erased.
  bool erase(const K &k) {
    u64 idx;
    if (!locate(k, idx))
      return false;

    for (u64 i = idx; i + 1 < _n; i++)
      _data[i] = _data[i + 1];
    _n--;

    return true;
  }

  // True when k is mapped.
  bool exists(const K &k) const {
    u64 idx;
    return locate(k, idx);
  }

  // Releases all memory; the map becomes empty and reusable.
  void clear() {
    delete[] _data;
    _data = 0;
    _n = _capacity = 0;
  }

  // Reference to the value stored under k. Throws Exception when k is
  // absent (same contract as HashTable::find).
  V &find(const K &k) {
    u64 idx;
    if (!locate(k, idx))
      throw Exception("Map::find: key not found");
    return _data[idx].value;
  }

  const V &find(const K &k) const {
    u64 idx;
    if (!locate(k, idx))
      throw Exception("Map::find: key not found");
    return _data[idx].value;
  }

  // Read/write access by key: reading an absent key inserts a default
  // V (std::map's operator[] contract); the const overload reports an
  // absent key by throwing.
  V &operator[](const K &k) {
    u64 idx;
    if (locate(k, idx))
      return _data[idx].value;
    insert(k, V());
    locate(k, idx); // re-locate: insert may have shifted entries
    return _data[idx].value;
  }

  const V &operator[](const K &k) const {
    u64 idx;
    if (!locate(k, idx))
      throw Exception("Map: key not found");
    return _data[idx].value;
  }

  // The key at sorted position i (unchecked, like operator[] on
  // Array<T>); bounds are the caller's responsibility.
  const K &keyAt(size_type i) const { return _data[i].key; }

  const V &valueAt(size_type i) const { return _data[i].value; }

  // In-order traversal: f(key, value) for every entry, ascending by
  // key (SkipList's traversal contract).
  template <typename Func> void for_each(Func f) const {
    for (u64 i = 0; i < _n; i++)
      f(_data[i].key, _data[i].value);
  }

  // True when both maps hold exactly the same (key, value) entries.
  bool operator==(const Map &o) const {
    if (_n != o._n)
      return false;
    for (u64 i = 0; i < _n; i++) {
      if (_data[i].key < o._data[i].key || o._data[i].key < _data[i].key)
        return false;
      if (_data[i].value < o._data[i].value ||
          o._data[i].value < _data[i].value)
        return false;
    }
    return true;
  }

  bool operator!=(const Map &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os,
                                    const Map<K, V> &m);
};

// Prints the entries ascending, one "key -> value" line per entry
// (SkipList's printer format; an empty map prints nothing).
template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const Map<K, V> &m) {
  m.for_each([&os](const K &k, const V &v) { os << k << " -> " << v << "\n"; });
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename K, typename V>
void swap(Map<K, V> &a, Map<K, V> &b) {
  a.swap(b);
}
