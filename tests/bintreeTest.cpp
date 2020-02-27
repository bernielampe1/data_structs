#include "BinTree.h"
#include <iostream>

using namespace std;

int main() {
  BinTree<int> b;
  BinTree<int> c;

  b.insert(10);
  b.insert(9);
  b.insert(8);
  b.insert(11);

  cout << b.exists(10) << endl;
  cout << b.exists(9) << endl;
  cout << b.exists(8) << endl;
  cout << b.exists(11) << endl;
  cout << b.exists(1) << endl;

  c = b;

  cout << c.exists(10) << endl;
  cout << c.exists(9) << endl;
  cout << c.exists(8) << endl;
  cout << c.exists(11) << endl;
  cout << c.exists(1) << endl;

  return (0);
}
