#include "BinHeap.h"
#include <iostream>

using namespace std;

struct obj {
  obj() : _n(-1) {}
  obj(const int n) : _n(n) {}
  bool operator<(struct obj &rhs) { return _n < rhs._n; }
  bool operator>(struct obj &rhs) { return _n > rhs._n; }
  bool operator==(struct obj &rhs) { return _n == rhs._n; }
  bool operator!=(struct obj &rhs) { return _n != rhs._n; }

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

int main() {
  BinHeap<obj> bh(100);

  for (int i = 0; i < 100; i++) {
    bh.insert(obj(i));
  }

  cout << bh << std::endl;

  for (int i = 0; i < 100; i++) {
    cout << bh.extractroot() << ',';
  }


  return (0);
}
