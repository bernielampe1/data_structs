#pragma once

#include "Exception.h"

typedef unsigned ElemType;
const unsigned ELEMSIZE = sizeof(ElemType) * 8;

class BitVector {
private:
  ElemType *_bits;
  unsigned _numBits;
  unsigned _numBytes;

public:
  BitVector() : _bits(0), _numBits(0), _numBytes(0) {}

  BitVector(const unsigned &n) : _bits(0), _numBits(0), _numBytes(0) {
    this->init(n);
  }

  BitVector(const BitVector &n) : _bits(0), _numBits(0), _numBytes(0) {
    this->init(n._numBits);
    memcpy(_bits, n._bits, _numBytes);
  }

  ~BitVector() { this->clear(); }

  BitVector &operator=(const BitVector &rhs) {
    if (this != &rhs && rhs.isValid()) {
      this->clear();
      this->_numBits = rhs._numBits;
      this->_numBytes = rhs._numBytes;
      this->init(rhs._numBits);
      memcpy(this->_bits, rhs._bits, rhs._numBytes * sizeof(ElemType));
    }

    return (*this);
  }

  BitVector operator&(const BitVector &rhs) const {
    BitVector rtnVec;

    if (rhs.size() != this->size())
      throw(Exception("bit vectors are different sizes"));

    rtnVec = *this;
    for (unsigned i = 0; i < this->_numBytes; i++) {
      rtnVec._bits[i] &= rhs._bits[i];
    }

    return (rtnVec);
  }

  void init(const unsigned &n) {
    this->clear();
    this->_numBits = n;
    this->_numBytes = (n / ELEMSIZE) + ((n % ELEMSIZE) != 0);
    this->_bits = new ElemType[this->_numBytes];
    memset(this->_bits, 0, this->_numBytes * sizeof(ElemType));
  }

  void clear() {
    if (this->isValid()) {
      delete[] this->_bits;
    }
    this->_bits = 0;
    this->_numBits = 0;
    this->_numBytes = 0;
  }

  bool isValid() const { return (this->_bits != 0 && this->_numBytes > 0); }

  unsigned size() const { return (this->_numBits); }

  bool getBit(const unsigned &n) const {
    return (this->_bits[n >> 5] >> (n & 31) & 1);
  }

  void setBit(const unsigned &n) { this->_bits[n >> 5] |= (1 << (n & 31)); }

  unsigned countOneBits() const {
    unsigned count = 0;
    unsigned n, tmp;

    for (unsigned i = 0; i < this->_numBytes; i++) {
      n = this->_bits[i];
      tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
      count += ((tmp + (tmp >> 3)) & 030707070707) % 63;
    }

    return (count);
  }

  unsigned countZeroBits() const {
    return (this->_numBits - this->countOneBits());
  }

  friend ostream &operator<<(ostream &os, const BitVector &rhs);
};

ostream &operator<<(ostream &os, const BitVector &rhs) {
  os << "(";
  for (unsigned i = 0; i < rhs._numBits; i++) {
    os << unsigned(rhs.getBit(i)) << ",";
  }
  os << ")";

  return (os);
}
