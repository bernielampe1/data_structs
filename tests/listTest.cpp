#include<iostream>
#include"List.h"

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
  List<obj> a0;
  List<obj> a1(a0);
  List<obj> a2(100);
  List<obj> a3(a2);
  List<obj> a4(obj(100), 100);
  List<obj> a5 = a4;

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


  for(int i = 0; i < 99; i++) { a0.push_back(i); }

  cout << "a0 contains: " << a0 << endl;

  a0.clear();

  cout << "a0 contains: " << a0 << endl;
  
  a0.resize(50);
  cout << "resize of a0 = " << a0.size() << endl;
  cout << "a0 contains: " << a0 << endl;


  for(int i = 0; i < 25; i++) { a0.pop_back(); }
  cout << "pop_back of a0 = " << a0.size() << endl;

  a0.push_back(10000);
  cout << "a0 on back is = " << a0.back() << endl;
  cout << "push_back of a0 = " << a0.size() << endl;

  for(int i = 0; i < 25; i++) { a0.push_front(i); }
  cout << "push_front of a0 contains: " << a0 << endl;
  cout << "push_front of a0 = " << a0.size() << endl;
  
  
  for(int i = 0; i < 25; i++) { a0.pop_front(); }
  cout << "pop_front of a0 contains: " << a0 << endl;
  cout << "pop_front of a0 = " << a0.size() << endl;

  a0.push_front(10000);
  cout << "a0 on front is = " << a0.front() << endl;
  cout << "push_front of a0 = " << a0.size() << endl;

  List<obj>::iterator it = a0.begin();

  for(int i = 0; i < 15; i++) it++;
  a0.insert(it, 5555);
  cout << "insert of a0 contains: " << a0 << endl;
  cout << "insert of a0 = " << a0.size() << endl;

  a0.erase(++it);
  cout << "erase of a0 contains: " << a0 << endl;
  cout << "erase of a0 = " << a0.size() << endl;

  return(0);
}

