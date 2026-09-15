#pragma once

#include <cstring>
#include <ostream>
#include <utility>

#include "Exception.h"

typedef unsigned ElemType;
const unsigned ELEMSIZE = sizeof(ElemType) * 8; // bits per element

// BitVector: a fixed-length sequence of bits packed into ElemType words.
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// Bit indexing is unchecked (same contract as std::vector's operator[]):
// getBit/setBit past size() is undefined behavior. Bits within the
// final partial word but past size() are kept zero so the counting
// helpers stay exact for any bits set through the public API.

class BitVector {
private:
  ElemType *_bits;    // word buffer; null when empty
  unsigned _numBits;   // logical bit count
  unsigned _numWords;  // words allocated (never zero when _bits != null)

  // Allocates n zeroed words under an RAII holder, so a later failure
  // leaks nothing.
  static ElemType *allocateWords(unsigned words) {
    return new ElemType[words]();
  }

public:
  BitVector() : _bits(0), _numBits(0), _numWords(0) {}

  explicit BitVector(unsigned n) : _bits(0), _numBits(0), _numWords(0) {
    init(n);
  }

  // Deep copy: same bit count, same contents.
  BitVector(const BitVector &o)
      : _bits(0), _numBits(0), _numWords(0) {
    if (o._bits) {
      _bits = allocateWords(o._numWords);
      memcpy(_bits, o._bits, o._numWords * sizeof(ElemType));
      _numBits = o._numBits;
      _numWords = o._numWords;
    }
  }

  // Steal o's words; o left empty.
  BitVector(BitVector &&o) noexcept
      : _bits(o._bits), _numBits(o._numBits), _numWords(o._numWords) {
    o._bits = 0;
    o._numBits = 0;
    o._numWords = 0;
  }

  ~BitVector() { clear(); }

  // Deep copy: build and fill the new buffer before releasing the old
  // one, so a failed allocation leaves *this untouched.
  BitVector &operator=(const BitVector &o) {
    if (this == &o)
      return *this;

    ElemType *newBits = 0;
    if (o._bits) {
      newBits = allocateWords(o._numWords);
      memcpy(newBits, o._bits, o._numWords * sizeof(ElemType));
    }

    delete[] _bits;
    _bits = newBits;
    _numBits = o._numBits;
    _numWords = o._numWords;

    return *this;
  }

  // Steal o's words (freeing ours, unconditionally -- delete[] on null
  // is a no-op).
  BitVector &operator=(BitVector &&o) noexcept {
    if (this != &o) {
      delete[] _bits;
      _bits = o._bits;
      _numBits = o._numBits;
      _numWords = o._numWords;
      o._bits = 0;
      o._numBits = 0;
      o._numWords = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(BitVector &o) {
    std::swap(_bits, o._bits);
    std::swap(_numBits, o._numBits);
    std::swap(_numWords, o._numWords);
  }

  // Reinitializes to n zeroed bits, releasing any previous storage.
  // The new buffer is allocated and zeroed before the old is freed, so
  // a failed allocation leaves *this untouched.
  void init(unsigned n) {
    ElemType *newBits = n ? allocateWords((n / ELEMSIZE) + ((n % ELEMSIZE) != 0))
                          : 0;

    delete[] _bits;
    _bits = newBits;
    _numBits = n;
    _numWords = n ? (n / ELEMSIZE) + ((n % ELEMSIZE) != 0) : 0;
  }

  // Releases all memory; the vector becomes empty and reusable.
  void clear() {
    delete[] _bits;
    _bits = 0;
    _numBits = 0;
    _numWords = 0;
  }

  unsigned size() const { return _numBits; } // logical bit count

  bool getBit(unsigned n) const { // bit n (UB if n >= size())
    return (_bits[n / ELEMSIZE] >> (n % ELEMSIZE)) & 1;
  }

  void setBit(unsigned n) { // set bit n to one (UB if n >= size())
    _bits[n / ELEMSIZE] |= ElemType(1) << (n % ELEMSIZE);
  }

  void clearBit(unsigned n) { // set bit n to zero (UB if n >= size())
    _bits[n / ELEMSIZE] &= ~(ElemType(1) << (n % ELEMSIZE));
  }

  // Bitwise AND; throws Exception when the sizes differ.
  BitVector operator&(const BitVector &rhs) const {
    if (rhs.size() != size())
      throw(Exception("bit vectors are different sizes"));

    BitVector rtnVec(*this);
    for (unsigned i = 0; i < _numWords; i++)
      rtnVec._bits[i] &= rhs._bits[i];

    return (rtnVec); // maskTail unnecessary: both inputs are masked
  }

  unsigned countOneBits() const { // number of set bits
    unsigned count = 0, n, tmp;

    for (unsigned i = 0; i < _numWords; i++) {
      n = _bits[i];
      tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
      count += ((tmp + (tmp >> 3)) & 030707070707) % 63;
    }

    return count;
  }

  unsigned countZeroBits() const { // number of clear bits
    return _numBits - countOneBits();
  }

  friend ostream &operator<<(ostream &os, const BitVector &rhs);
};

ostream &operator<<(ostream &os, const BitVector &rhs) {
  os << "(";
  for (unsigned i = 0; i < rhs._numBits; i++) {
    if (i > 0)
      os << ",";
    os << unsigned(rhs.getBit(i));
  }
  os << ")";

  return os;
}
