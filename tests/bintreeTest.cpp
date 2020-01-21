#include<iostream>
#include"BinTree.h"

using namespace std;
using namespace bhl;

int main()
{
  BinTree<int> b;

  b.insert(10);
  b.insert(9);
  b.insert(8);
  b.insert(11);

  cout << b.exists(10) << endl;
  cout << b.exists(9) << endl;
  cout << b.exists(8) << endl;
  cout << b.exists(11) << endl;
  cout << b.exists(1) << endl;

  return(0);
}
