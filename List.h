#ifndef _BHL_LIST_H_
#define _BHL_LIST_H_

namespace bhl {
template <typename T> class List {
  struct Node {
    Node(Node *n, Node *p) : _next(n), _prev(p) {}
    Node(const T &d, Node *n, Node *p) : _data(d), _next(n), _prev(p) {}

    T _data;
    Node *_next, *_prev;
  };

private:
  Node *_head, *_tail;
  unsigned _n;

  void copy(const List<T> &rhs) {
    if (this != &rhs) {
      clear();

      _n = rhs._n;
      Node *ptr1 = rhs._head->_next;
      Node *ptr2 = _head;

      for (unsigned i = 0; i < _n; i++) {
        ptr2 = ptr2->_next = new Node(ptr1->_data, 0, ptr1);
        ptr1 = ptr1->_next;
      }

      _tail = ptr2;
    }
  }

public:
  class iterator {
  private:
    Node *_ptr;
    unsigned _p;

  public:
    iterator(Node *ptr, unsigned p) : _ptr(ptr), _p(p) {}

    bool operator==(const iterator &rhs) const { return (_p == rhs._p); }
    bool operator!=(const iterator &rhs) const { return (_p != rhs._p); }
    bool operator<(const iterator &rhs) const { return (_p < rhs._p); }
    bool operator>(const iterator &rhs) const { return (_p > rhs._p); }
    iterator &operator++() {
      _p++;
      _ptr = _ptr->_next;
      return (*this);
    }
    iterator operator++(int) {
      iterator it = *this;
      _p++;
      _ptr = _ptr->_next;
      return (it);
    }
    iterator &operator--() {
      _p--;
      _ptr = _ptr->_prev;
      return (*this);
    }
    iterator operator--(int) {
      iterator it = *this;
      _p--;
      _ptr = _ptr->_prev;
      return (it);
    }
    T &operator*() const { return (_ptr->_data); }
    T *operator->() const { return (&(operator*())); };

    friend class List<T>;
  };

  class const_iterator {
  private:
    const Node *_ptr;
    unsigned _p;

  public:
    const_iterator(Node *ptr, unsigned p) : _ptr(ptr), _p(p) {}

    bool operator==(const const_iterator &rhs) const { return (_p == rhs._p); }
    bool operator!=(const const_iterator &rhs) const { return (_p != rhs._p); }
    bool operator<(const const_iterator &rhs) const { return (_p < rhs._p); }
    bool operator>(const const_iterator &rhs) const { return (_p > rhs._p); }
    const_iterator &operator++() {
      _p++;
      _ptr = _ptr->_next;
      return (*this);
    }
    const_iterator operator++(int) {
      const_iterator it = *this;
      _p++;
      _ptr = _ptr->_next;
      return (it);
    }
    const_iterator &operator--() {
      _p--;
      _ptr = _ptr->_prev;
      return (*this);
    }
    const_iterator operator--(int) {
      const_iterator it = *this;
      _p--;
      _ptr = _ptr->_prev;
      return (it);
    }
    const T &operator*() const { return (_ptr->_data); }
    const T *operator->() const { return (&(operator*())); };

    friend class List<T>;
  };

public:
  List() : _head(new Node(0, 0)), _tail(_head), _n(0) {}

  List(const unsigned n) : _head(new Node(0, 0)), _tail(_head), _n(n) {
    Node *ptr = _head;
    for (unsigned i = 0; i < _n; i++) {
      ptr = ptr->_next = new Node(0, ptr);
    }
    _tail = ptr;
  }

  List(const T &o, const unsigned n)
      : _head(new Node(0, 0)), _tail(_head), _n(n) {
    Node *ptr = _head;
    for (unsigned i = 0; i < _n; i++) {
      ptr = ptr->_next = new Node(o, 0, ptr);
    }
    _tail = ptr;
  }

  List(const List &rhs) : _head(new Node(0, 0)), _tail(_head), _n(rhs._n) {
    copy(rhs);
  }

  ~List() {
    clear();
    delete _head;
  }

  List &operator=(const List &rhs) {
    copy(rhs);
    return (*this);
  }

  unsigned size() const { return (_n); }

  bool empty() const { return (_n == 0); }

  void resize(const unsigned n) {
    if (n > _n) {
      for (unsigned i = 0; i < n - _n; i++) {
        _tail = _tail->_next = new Node(0, _tail);
      }
      _n = n;
    } else if (n < _n) {
      for (unsigned i = 0; i < _n - n; i++) {
        Node *ptr = _tail;
        _tail = _tail->_prev;
        delete ptr;
      }
      _tail->_next = 0;
      _n = n;
    }
  }

  void clear() {
    Node *tmp, *ptr = _head->_next;
    while (ptr != 0) {
      tmp = ptr;
      ptr = ptr->_next;
      delete tmp;
    }
    _tail = _head;
    _n = 0;
  }

  void push_back(const T &d) {
    _tail = _tail->_next = new Node(d, 0, _tail);
    _n++;
  }

  void pop_back() {
    if (!empty()) {
      Node *tmp = _tail;
      _tail = _tail->_prev;
      delete tmp;

      _tail->_next = 0;
      _n--;
    }
  }

  const T &back() const { return (_tail->_data); }

  void push_front(const T &d) {
    Node *tmp = _head->_next;
    tmp->_prev = _head->_next = new Node(d, tmp, _head);
    _n++;
  }

  void pop_front() {
    if (!empty()) {
      Node *tmp = _head->_next;
      _head->_next = tmp->_next;
      if (tmp->_next)
        tmp->_next->_prev = _head;
      delete tmp;
      _n--;
    }
  }

  const T &front() const {
    if (!empty())
      return (_head->_next->_data);
    return (_head->_data);
  }

  iterator insert(const iterator &loc, const T &d) {
    Node *tmp = loc._ptr->_next;
    loc._ptr->_next = new Node(d, tmp, loc._ptr);
    if (tmp)
      tmp->_prev = loc._ptr->_next;

    if (loc._ptr == _tail) {
      _tail = loc._ptr->_next;
    }
    _n++;

    return (iterator(loc._ptr->_next, loc._p + 1));
  }

  iterator erase(const iterator &loc) {
    if (!empty()) {
      Node *tmp = loc._ptr;
      Node *before = loc._ptr->_prev;
      Node *after = loc._ptr->_next;

      if (before)
        before->_next = after;
      if (after)
        after->_prev = before;

      delete tmp;
      _n--;

      return (iterator(before, loc._p > 0 ? loc._p - 1 : 0));
    }

    return (iterator(_head, 0));
  }

  iterator begin() { return (iterator(_head->_next, 0)); }
  const_iterator begin() const { return (const_iterator(_head->_next, 0)); }

  iterator end() { return (iterator(_tail, _n)); }
  const_iterator end() const { return (const_iterator(_tail, _n)); }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const List<T> &rhs) {
  for (typename List<T>::const_iterator it = rhs.begin(); it != rhs.end();
       it++) {
    os << *it << ", ";
  }

  return (os);
}
} // namespace bhl

#endif // _BHL_LIST_H_
