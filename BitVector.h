#ifndef _BITVECTOR_H_
#define _BITVECTOR_H_

#include <math.h>

using namespace std;

#include "P5Exception.h"

typedef unsigned ElemType;
const unsigned ELEMSIZE = sizeof(ElemType) * 8;

class BitVector {
private:
  ElemType *bits;
  unsigned numBits;
  unsigned numElems;

public:
  BitVector() : bits(0), numBits(0), numElems(0) {}

  BitVector(const unsigned &n) : bits(0), numBits(0), numElems(0) {
    this->init(n);
  }

  BitVector(const BitVector &rhs) : bits(0), numBits(0), numElems(0) {
    *this = rhs;
  }

  ~BitVector() { this->clear(); }

  BitVector &operator=(const BitVector &rhs) {
    if (this != &rhs && rhs.isValid()) {
      this->clear();
      this->numBits = rhs.numBits;
      this->numElems = rhs.numElems;
      this->init(rhs.numBits);
      memcpy(this->bits, rhs.bits, rhs.numElems * sizeof(ElemType));
    }

    return (*this);
  }

  BitVector operator&(const BitVector &rhs) const {
    BitVector rtnVec;

    if (rhs.size() != this->size())
      throw(P5Exception("bit vectors are different sizes"));

    rtnVec = *this;
    for (unsigned i = 0; i < this->numElems; i++) {
      rtnVec.bits[i] &= rhs.bits[i];
    }

    return (rtnVec);
  }

  void init(const unsigned &n) {
    this->clear();
    this->numElems = unsigned(ceil(double(n) / double(ELEMSIZE)));
    this->numBits = n;
    this->bits = new ElemType[this->numElems];
    memset(this->bits, 0, this->numElems * sizeof(ElemType));
  }

  void clear() {
    if (this->isValid()) {
      delete[] this->bits;
    }
    this->bits = 0;
    this->numBits = 0;
    this->numElems = 0;
  }

  bool isValid() const { return (this->bits != 0 && this->numElems > 0); }

  unsigned size() const { return (this->numBits); }

  bool getBit(const unsigned &n) const {
    return (this->bits[n >> 5] >> (n & 31) & 1);
  }

  void setBit(const unsigned &n) { this->bits[n >> 5] |= (1 << (n & 31)); }

  unsigned countOneBits() const {
    register unsigned count = 0;
    register unsigned n, tmp;

    for (unsigned i = 0; i < this->numElems; i++) {
      n = this->bits[i];
      tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
      count += ((tmp + (tmp >> 3)) & 030707070707) % 63;
    }

    return (count);
  }

  unsigned countZeroBits() const {
    return (this->numBits - this->countOneBits());
  }

  friend ostream &operator<<(ostream &os, const BitVector &rhs);
};

#endif /* _BITVECTOR_H_ */
