#include "Queue.h"
#include <iostream>

using namespace std;

struct obj {
  obj() : _n(0) {}
  obj(const int n) : _n(n) {}

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

int main() {
  Queue<obj> a0;
  Queue<obj> a1(a0);

  for (int i = 0; i < 25; i++) {
    a0.push(obj(i));
  }

  Queue<obj> a2 = a0;

  cout << "size of a0 = " << a0.size() << endl;
  cout << "size of a1 = " << a1.size() << endl;
  cout << "size of a2 = " << a2.size() << endl;

  a0.clear();
  a1.clear();
  a2.clear();

  cout << "size of a0 = " << a0.size() << endl;
  cout << "size of a1 = " << a1.size() << endl;
  cout << "size of a2 = " << a2.size() << endl;

  for (int i = 0; i < 25; i++) {
    a0.push(obj(i));
  }

  cout << "size of a0 = " << a0.size() << endl;
  cout << "a0 contains: " << a0 << endl;

  for (int i = 0; i < 10; i++) {
    a0.pop();
  }

  cout << "size of a0 = " << a0.size() << endl;
  cout << "a0 contains: " << a0 << endl;

  cout << "a0 front is = " << a0.front() << endl;
  cout << "a0 back is = " << a0.back() << endl;

  for (int i = 0; i < 1000; i++) {
    a0.pop();
  }

  cout << "size of a0 = " << a0.size() << endl;
  cout << "a0 contains: " << a0 << endl;

  return (0);
}
