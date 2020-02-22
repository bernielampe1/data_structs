#include "qsort.h"
#include "Array.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

int main() {
  Array<float> a(100);
  for (int i = 0; i < 100; ++i) {
    a[i] = float(rand()) / float(rand());
  }

  cout
      << "*********************************************************************"
      << endl;
  cout << "DATA = " << a << endl << endl;

  qsort(a, 0, a.size() - 1);

  cout
      << "*********************************************************************"
      << endl;
  cout << "DATA = " << a << endl << endl;

  return (0);
}
