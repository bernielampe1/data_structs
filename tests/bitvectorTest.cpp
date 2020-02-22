#include <iostream>

using namespace std;

#include "BitVector.h"

int main() {
  BitVector bv(101);
  BitVector bv_copy(101);

  for(int i = 0; i < 101; i++) {
    if (i % 2) bv.setBit(i);
  }

  cout << bv << std::endl;
  cout << bv_copy << std::endl;
  bv_copy = bv;
  cout << bv_copy << std::endl;
  cout << bv.size() << std::endl;
  cout << bv.countZeroBits() << std::endl;
  cout << bv.countOneBits() << std::endl;

  return (0);
}
