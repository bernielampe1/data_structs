#include "HashTable.h"
#include <iostream>

using namespace std;

int hashFunc(const string &s) {
  unsigned long v = 5381;
  for (unsigned i = 0; i < s.size(); i++)
    v = ((v << 5) + v) + s[i];
  return (v);
}

int main() {
  HashTable<string, float> h(100, hashFunc);

  h.insert("jan", 1);
  h.insert("feb", 2);
  h.insert("mar", 3);
  h.insert("apr", 4);
  h.insert("may", 5);
  h["jun"] = 6;
  h["jul"] = 7;
  h["aug"] = 8;
  h["sep"] = 9;
  h["oct"] = 10;
  h["nov"] = 11;
  h["dec"] = 12;

  HashTable<string, float> hcopy0(h);
  HashTable<string, float> hcopy1 = h;

  cout << "size of hash table h = " << h.size() << endl;
  cout << "size of hash table hcopy0 = " << hcopy0.size() << endl;
  cout << "size of hash table hcopy1 = " << hcopy1.size() << endl;

  hcopy1.clear();
  cout << "size of hash table hcopy1 = " << hcopy1.size() << endl;

  h.erase("jan");
  h.erase("nov");
  h.erase("dec");
  cout << "erase size of hash table h = " << h.size() << endl;
  cout << "exists jan keyword in h = " << h.exists("jan") << endl;
  cout << "exists nov keyword in h = " << h.exists("nov") << endl;
  cout << "exists dec keyword in h = " << h.exists("dec") << endl;

  cout << "find of keyword feb in h = " << h.find("feb") << endl;
  cout << "find of keyword mar in h = " << h.find("mar") << endl;

  cout << "operator[] of keyword feb in h = " << h["feb"] << endl;
  cout << "operator[] of keyword mar in h = " << h["mar"] << endl;

  h.erase("feb");
  h.erase("mar");
  h.erase("apr");
  h.erase("may");
  h.erase("jun");
  h.erase("jul");
  h.erase("aug");
  h.erase("sep");
  h.erase("oct");

  cout << "erase size of hash table h = " << h.size() << endl;

  h.insert("jan", 1);
  h.insert("feb", 2);
  h.insert("mar", 3);
  h.insert("apr", 4);
  h.insert("may", 5);
  h["jun"] = 6;
  h["jul"] = 7;
  h["aug"] = 8;
  h["sep"] = 9;
  h["oct"] = 10;
  h["nov"] = 11;
  h["dec"] = 12;

  cout << "insert size of hash table h = " << h.size() << endl;

  h.erase("feb");
  h.erase("mar");
  h.erase("jan");
  h.erase("may");
  h.erase("apr");
  h.erase("jun");
  h.erase("dec");
  h.erase("aug");
  h.erase("jul");
  h.erase("oct");
  h.erase("sep");
  h.erase("nov");
  h.erase("nov");
  h.erase("nov");
  h.erase("nov");

  cout << "erase size of hash table h = " << h.size() << endl;

  cout << "exists jan keyword in h = " << h.exists("jan") << endl;
  cout << "exists feb keyword in h = " << h.exists("feb") << endl;
  cout << "exists mar keyword in h = " << h.exists("mar") << endl;
  cout << "exists apr keyword in h = " << h.exists("apr") << endl;
  cout << "exists may keyword in h = " << h.exists("may") << endl;
  cout << "exists jun keyword in h = " << h.exists("jun") << endl;
  cout << "exists jul keyword in h = " << h.exists("jul") << endl;
  cout << "exists aug keyword in h = " << h.exists("aug") << endl;
  cout << "exists sep keyword in h = " << h.exists("sep") << endl;
  cout << "exists oct keyword in h = " << h.exists("oct") << endl;
  cout << "exists nov keyword in h = " << h.exists("nov") << endl;
  cout << "exists dec keyword in h = " << h.exists("dec") << endl;

  return (0);
}
