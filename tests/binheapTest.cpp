#include "BinHeap.h"
#include <iostream>

using namespace std;
using namespace bhl;

struct obj {
  obj() : _n(-1) {}
  obj(const int n) : _n(n) {}
  bool operator<(struct obj &rhs) { return _n < rhs._n; }
  struct obj &operator=(const struct obj &rhs) {
    if (&rhs != this) {
      _n = rhs._n;
    }

    return *this;
  }

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

int main() {
  BinHeap<obj> bh(200);

  for (int i = 0; i < 100; i++) {
    bh.insert(obj(i - 50));
  }

  for (int i = 0; i < 100; i++) {
    cout << bh.extractroot() << ',';
  }
  cout << endl;

  return (0);
}
