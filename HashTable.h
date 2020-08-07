#pragma once

template <typename K, typename T> class HashTable {
  struct Node {
    Node(const K &k, const T &d) : _key(k), _data(d), _next(0) {}

    K _key;
    T _data;
    Node *_next;
  };

private:
  Node **_array;
  unsigned _n, _size;
  int (*_hash)(const K &);

  void copy(const HashTable &rhs) {
    if (this != &rhs) {
      clear();

      _n = rhs._n;
      _size = rhs._size;
      _hash = rhs._hash;
      _array = new Node *[_size];

      for (unsigned i = 0; i < _size; i++) {
        if (rhs._array[i]) {
          Node *ptr1 = rhs._array[i]->_next;
          Node *ptr2 = _array[i] =
              new Node(rhs._array[i]->_key, rhs._array[i]->_data);

          while (ptr1 != 0) {
            ptr2 = ptr2->_next = new Node(ptr1->_key, ptr1->_data);
            ptr1 = ptr1->_next;
          }
        } else {
          _array[i] = 0;
        }
      }
    }
  }

public:
  HashTable(const unsigned s, int (*f)(const K &k))
      : _array(new Node *[s]), _n(0), _size(s), _hash(f) {
    for (unsigned i = 0; i < _size; i++)
      _array[i] = 0;
  }

  HashTable(const HashTable &rhs) : _array(0), _n(0), _size(0), _hash(0) {
    copy(rhs);
  }

  ~HashTable() { clear(); }

  HashTable &operator=(const HashTable &rhs) {
    copy(rhs);
    return (*this);
  }

  void clear() {
    for (unsigned i = 0; i < _size; i++) {
      Node *ptr = _array[i];
      while (ptr != 0) {
        Node *nx = ptr->_next;
        delete ptr;
        ptr = nx;
      }
      _array[i] = 0;
    }

    delete[] _array;
    _array = 0;

    _size = _n = 0;
    _hash = 0;
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  T &insert(const K &k, const T &d) {
    int b = _hash(k) % _size;
    T *rtn;

    if (_array[b]) {
      Node *ptr = _array[b];
      do {
        if (ptr->_key == k) {
          ptr->_data = d;
          return (ptr->_data);
        }

        if (ptr->_next == 0)
          break;
        else
          ptr = ptr->_next;

      } while (ptr != 0);

      _n++;
      ptr->_next = new Node(k, d);
      rtn = &(ptr->_next->_data);
    } else {
      _n++;
      _array[b] = new Node(k, d);
      rtn = &(_array[b]->_data);
    }

    return (*rtn);
  }

  bool erase(const K &k) {
    if (exists(k)) {
      int b = _hash(k) % _size;

      Node *ptr = _array[b];
      Node *prev = 0;
      while (ptr != 0) {
        if (ptr->_key == k) {
          if (ptr == _array[b]) {
            if (ptr->_next) {
              _array[b] = ptr->_next;
            } else {
              _array[b] = 0;
            }

            delete ptr;
          } else if (ptr->_next == 0) {
            delete ptr;
            prev->_next = 0;
          } else {
            prev->_next = ptr->_next;
            delete ptr;
          }

          _n--;
          return (true);
        }

        prev = ptr;
        ptr = ptr->_next;
      }
    }

    return (false);
  }

  bool exists(const K &k) const {
    int b = _hash(k) % _size;

    Node *ptr = _array[b];
    while (ptr != 0) {
      if (ptr->_key == k)
        return (true);
      ptr = ptr->_next;
    }

    return (false);
  }

  T &find(const K &k) {
    int b = _hash(k) % _size;

    Node *ptr = _array[b];
    while (ptr != 0) {
      if (ptr->_key == k) {
        return (ptr->_data);
      }
      ptr = ptr->_next;
    }
  }

  const T &find(const K &k) const {
    int b = _hash(k) % _size;

    Node *ptr = _array[b];
    while (ptr != 0) {
      if (ptr->_key == k) {
        return (ptr->_data);
      }
      ptr = ptr->_next;
    }
  }

  T &operator[](const K &k) {
    if (exists(k))
      return (find(k));
    return (insert(k, T()));
  }

  const T &operator[](const K &k) const { return (find(k)); }
};
