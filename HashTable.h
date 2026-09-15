#pragma once

#include <utility>

#include "Exception.h"

// HashTable<K, T>: chaining hash table mapping K to T.
//
// A rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined. Moves
// are noexcept and leave the source bucket-less and destructible, but NOT
// reusable until assigned to (the bucket count and hash function are part
// of the resource that moved). insert()/find() on such a table throw.
//
// insert() stores d under k, replacing the value when k is already
// present. No bucket growth: the bucket count is fixed at construction,
// so keeping the table lightly loaded is the caller's responsibility
// (this matches the repo's fixed-capacity containers).
//
// Key requirements: K must work with the supplied int-returning hash
// function (any 32-bit pattern, including negative values, is handled)
// and support operator==. Inserts chain-identical keys together, so a
// poor hash only costs time, never correctness. T must be
// default-constructible (non-const operator[]) and copy-assignable.
//
// find() on a missing key throws Exception (operator[] const shares this;
// non-const operator[] inserts a default T instead). erase()/exists() on
// a missing key simply report false.
template <typename K, typename T> class HashTable {
  struct Node {
    Node(const K &k, const T &d) : _key(k), _data(d), _next(0) {}

    K _key;
    T _data;
    Node *_next;
  };

private:
  Node **_array;      // bucket array; null when the table has no buckets
  unsigned _n;        // stored element count
  unsigned _size;     // bucket count (fixed at construction)
  int (*_hash)(const K &);

  // Deletes every node in every chain of arr, which must hold size
  // buckets. Nothrow; used by the destructor, clear(), and the cleanup
  // path of a failed copy.
  static void freeChains(Node **arr, unsigned size) {
    for (unsigned i = 0; i < size; i++) {
      Node *ptr = arr[i];
      while (ptr != 0) {
        Node *nx = ptr->_next;
        delete ptr;
        ptr = nx;
      }
      arr[i] = 0;
    }
  }

  // Builds a deep copy of o's bucket array. The array is zero-initialized
  // (new ... () nulls every bucket) and the fill is wrapped so a throwing
  // node copy frees the partial chains before propagating.
  static Node **deepCopy(const HashTable &o) {
    Node **arr = new Node *[o._size]();

    try {
      for (unsigned i = 0; i < o._size; i++) {
        Node *src = o._array[i];
        Node **tail = &arr[i];
        while (src != 0) {
          *tail = new Node(src->_key, src->_data);
          tail = &(*tail)->_next;
          src = src->_next;
        }
      }
    } catch (...) {
      freeChains(arr, o._size);
      delete[] arr;
      throw;
    }

    return arr;
  }

  // Bucket index for key k. The hash may return any 32-bit pattern,
  // including negative values (e.g. the test's djb2 overflows on long
  // strings); casting to unsigned BEFORE the modulo keeps the index in
  // [0, _size). Caller must ensure _size > 0.
  unsigned bucket(const K &k) const {
    return unsigned(_hash(k)) % _size;
  }

public:
  // s buckets; f computes the hash of a key.
  HashTable(const unsigned s, int (*f)(const K &k))
      : _array(new Node *[s]()), _n(0), _size(s), _hash(f) {}

  // Deep copy: same bucket count, same hash, same contents.
  HashTable(const HashTable &rhs)
      : _array(deepCopy(rhs)), _n(rhs._n), _size(rhs._size), _hash(rhs._hash) {}

  // Steal o's buckets AND hash; o left bucket-less. As with BinHeap's
  // capacity, the table's size and hash are part of the resource that
  // moved, so a moved-from table is not reusable until assigned to.
  HashTable(HashTable &&o) noexcept
      : _array(o._array), _n(o._n), _size(o._size), _hash(o._hash) {
    o._array = 0;
    o._n = 0;
    o._size = 0;
    o._hash = 0;
  }

  // Deep copy: the new table is built and filled before the old chains
  // are freed, so a failed copy leaves *this untouched.
  HashTable &operator=(const HashTable &rhs) {
    if (this == &rhs)
      return *this;

    Node **newArr = deepCopy(rhs); // may throw: *this untouched so far

    freeChains(_array, _size);
    delete[] _array;
    _array = newArr;
    _n = rhs._n;
    _size = rhs._size;
    _hash = rhs._hash;

    return *this;
  }

  // Steal o's buckets, freeing ours first (nothrow deletes).
  HashTable &operator=(HashTable &&o) noexcept {
    if (this != &o) {
      if (_array)
        freeChains(_array, _size);
      delete[] _array;
      _array = o._array;
      _n = o._n;
      _size = o._size;
      _hash = o._hash;
      o._array = 0;
      o._n = 0;
      o._size = 0;
      o._hash = 0;
    }
    return *this;
  }

  // Constant-time exchange of both tables.
  void swap(HashTable &o) {
    std::swap(_array, o._array);
    std::swap(_n, o._n);
    std::swap(_size, o._size);
    std::swap(_hash, o._hash);
  }

  // Removes every element but keeps the buckets allocated and the hash
  // function, so the table stays usable. (The old version zeroed _hash
  // and _size here, which made any later operation undefined.)
  void clear() {
    if (_array)
      freeChains(_array, _size);
    _n = 0;
  }

  unsigned size() const { return (_n); } // stored element count

  bool empty() const { return (_n == 0); }

  unsigned buckets() const { return (_size); } // bucket count

  // Returns a reference to the stored (or newly created) value for k;
  // replacing an existing key updates rather than duplicates.
  T &insert(const K &k, const T &d) {
    if (_size == 0)
      throw Exception("HashTable::insert: table has no buckets");

    Node **slot = &_array[bucket(k)];
    while (*slot != 0) {
      if ((*slot)->_key == k) {
        (*slot)->_data = d;
        return (*slot)->_data;
      }
      slot = &(*slot)->_next;
    }

    *slot = new Node(k, d);
    _n++;
    return (*slot)->_data;
  }

  // Removes k when present, reporting whether anything was erased.
  bool erase(const K &k) {
    if (_size == 0)
      return false;

    Node **slot = &_array[bucket(k)];
    while (*slot != 0) {
      if ((*slot)->_key == k) {
        Node *doomed = *slot;
        *slot = doomed->_next; // unlinks from head, middle, or tail position
        delete doomed;
        _n--;
        return true;
      }
      slot = &(*slot)->_next;
    }

    return false;
  }

  bool exists(const K &k) const {
    if (_size == 0)
      return false;

    Node *ptr = _array[bucket(k)];
    while (ptr != 0) {
      if (ptr->_key == k)
        return true;
      ptr = ptr->_next;
    }

    return false;
  }

  // Reference to the value stored under k. Throws Exception when k is
  // absent (the old code fell off the end of a non-void function here).
  T &find(const K &k) {
    if (_size == 0)
      throw Exception("HashTable::find: table has no buckets");

    Node *ptr = _array[bucket(k)];
    while (ptr != 0) {
      if (ptr->_key == k)
        return ptr->_data;
      ptr = ptr->_next;
    }

    throw Exception("HashTable::find: key not found");
  }

  const T &find(const K &k) const {
    if (_size == 0)
      throw Exception("HashTable::find: table has no buckets");

    const Node *ptr = _array[bucket(k)];
    while (ptr != 0) {
      if (ptr->_key == k)
        return ptr->_data;
      ptr = ptr->_next;
    }

    throw Exception("HashTable::find: key not found");
  }

  // Read/write access: reading an absent key inserts a default T;
  // the const overload reports an absent key by throwing.
  T &operator[](const K &k) {
    if (exists(k))
      return find(k);
    return insert(k, T());
  }

  const T &operator[](const K &k) const { return find(k); }
};
