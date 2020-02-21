#include <iostream>

using namespace std;

#include "BitVector.h"

ostream &operator<<(ostream &os, const BitVector &rhs) {
  os << "(";
  for (unsigned i = 0; i < rhs.numBits; i++) {
    os << unsigned(rhs.getBit(i)) << ",";
  }
  os << ")";

  return (os);
}
