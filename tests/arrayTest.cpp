#include<iostream>
#include"Array.h"

using namespace std;
using namespace bhl;

struct obj
{
    obj(): _n(0) { }
    obj(const int n): _n(n) { }

    int _n;
};

ostream& operator<<(ostream &os, const obj &o)
{
  os << o._n;
  return(os);
}

int main()
{
  Array<obj> a0;
  Array<obj> a1(a0);
  Array<obj> a2(100);
  Array<obj> a3(a2);
  Array<obj> a4(obj(100), 100);
  Array<obj> a5 = a4;

  cout << "size of a0 = " << a0.size() << endl;
  cout << "size of a1 = " << a1.size() << endl;
  cout << "size of a2 = " << a2.size() << endl;
  cout << "size of a3 = " << a3.size() << endl;
  cout << "size of a4 = " << a4.size() << endl;
  cout << "size of a5 = " << a5.size() << endl;

  a0.clear();
  a1.clear();
  a2.clear();
  a3.clear();
  a4.clear();
  a5.clear();

  cout << "size of a0 = " << a0.size() << endl;
  cout << "size of a1 = " << a1.size() << endl;
  cout << "size of a2 = " << a2.size() << endl;
  cout << "size of a3 = " << a3.size() << endl;
  cout << "size of a4 = " << a4.size() << endl;
  cout << "size of a5 = " << a5.size() << endl;

  a0.resize(999);
  cout << "resize of a0 = " << a0.size() << endl;

  a0.resize(99);
  cout << "resize of a0 = " << a0.size() << endl;


  for(int i = 0; i < 99; i++) { a0[i] = i; }

  cout << "a0 contains: " << a0 << endl;

  a0.clear();

  cout << "a0 contains: " << a0 << endl;

  a0.resize(50);
  cout << "resize of a0 = " << a0.size() << endl;
  cout << "a0 contains: " << a0 << endl;

  return(0);
}

