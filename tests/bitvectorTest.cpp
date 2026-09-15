// Tests for BitVector. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make bitvectorTest && ./bitvectorTest

#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

#include "BitVector.h"

static int failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      cout << "ok: " #cond << endl;                                            \
    } else {                                                                   \
      cout << "FAIL: " #cond << " (line " << __LINE__ << ")" << endl;          \
      failures++;                                                              \
    }                                                                          \
  } while (0)

int main() {
  // Construction and size.
  BitVector bv(101);
  BitVector bv_copy(101);

  CHECK(bv.size() == 101);
  CHECK(bv_copy.size() == 101);
  CHECK(bv_copy.countOneBits() == 0); // starts all-zero
  CHECK(bv_copy.countZeroBits() == 101);

  // setBit/getBit round-trip, including across word boundaries.
  for (int i = 0; i < 101; i++) {
    if (i % 2)
      bv.setBit(i);
  }
  CHECK(bv.getBit(1) && !bv.getBit(0));
  CHECK(bv.getBit(31) && bv.getBit(33)); // word boundary
  CHECK(bv.getBit(99) && !bv.getBit(100));
  CHECK(bv.countOneBits() == 50); // odd bits 1..99
  CHECK(bv.countZeroBits() == 51);

  // clearBit (new API).
  bv_copy.setBit(5);
  bv_copy.clearBit(5);
  CHECK(!bv_copy.getBit(5));
  CHECK(bv_copy.countOneBits() == 0);

  // Copy construction is deep.
  BitVector cc(bv);
  cc.setBit(0);
  CHECK(cc.getBit(0));
  CHECK(!bv.getBit(0)); // original unaffected
  CHECK(cc.size() == 101);

  // Copy assignment replaces contents, any combination of sizes.
  bv_copy = bv;
  CHECK(bv_copy.size() == 101);
  CHECK(bv_copy.countOneBits() == 50);

  BitVector small(4);
  small = bv; // grow on assign
  CHECK(small.size() == 101 && small.countOneBits() == 50);

  BitVector big(500);
  big.setBit(400);
  big = bv; // shrink on assign
  CHECK(big.size() == 101 && big.countOneBits() == 50);
  CHECK(!big.getBit(0)); // even bits were clear in bv

  // Assigning from an empty BitVector empties the destination
  // (regression: used to be a silent no-op).
  BitVector emptyBv;
  BitVector stale(10);
  stale.setBit(1);
  stale = emptyBv;
  CHECK(stale.size() == 0);

  // Self-assignment is a no-op.
  BitVector &bvAlias = bv;
  bv = bvAlias;
  CHECK(bv.size() == 101 && bv.countOneBits() == 50);

  // Move construction steals; source becomes empty and reusable.
  BitVector moved(std::move(bv));
  CHECK(moved.size() == 101);
  CHECK(moved.countOneBits() == 50);
  CHECK(bv.size() == 0);

  bv.init(1); // fresh storage for the moved-from vector
  bv.setBit(0); // now reusable
  CHECK(bv.size() == 1 && bv.getBit(0));

  // Move assignment frees destination and steals the source.
  BitVector m2(7);
  m2 = std::move(moved);
  CHECK(m2.size() == 101 && m2.countOneBits() == 50);
  CHECK(moved.size() == 0);

  // swap exchanges contents.
  BitVector s1(2), s2(3);
  s1.setBit(0);
  s2.setBit(0);
  s2.setBit(1);
  s1.swap(s2);
  CHECK(s1.size() == 3 && s2.size() == 2);
  CHECK(s1.countOneBits() == 2 && s2.countOneBits() == 1);

  // init() reinitializes to a fresh zeroed vector.
  BitVector iv(10);
  iv.setBit(1);
  iv.init(200);
  CHECK(iv.size() == 200);
  CHECK(!iv.getBit(1));
  CHECK(iv.countOneBits() == 0);

  // clear() empties and the vector stays reusable.
  iv.clear();
  CHECK(iv.size() == 0);
  iv.init(64);
  iv.setBit(63);
  CHECK(iv.size() == 64 && iv.getBit(63) && iv.countOneBits() == 1);

  // operator& ANDs equal-size vectors and throws on mismatch.
  BitVector a(32), b(32);
  a.setBit(0);
  b.setBit(0);
  b.setBit(1);
  BitVector c = a & b;
  CHECK(c.size() == 32);
  CHECK(c.getBit(0) && !c.getBit(1));

  bool threw = false;
  try {
    BitVector d(5), e(6);
    d & e;
  } catch (Exception &) {
    threw = true;
  }
  CHECK(threw);

  // operator<< prints bits, comma separated, no trailing comma.
  ostringstream oss;
  BitVector p(3);
  p.setBit(1);
  oss << p;
  CHECK(oss.str() == "(0,1,0)");

  ostringstream ess;
  ess << BitVector();
  CHECK(ess.str() == "()");

  // --- Contract cases pinned from the design review ---

  // Word-boundary exactness: bits 31/32 and 63/64 land in different words.
  {
    BitVector v(100);
    v.setBit(31);
    v.setBit(32);
    v.setBit(63);
    v.setBit(64);
    CHECK(v.getBit(31) && v.getBit(32) && v.getBit(63) && v.getBit(64));
    CHECK(v.countOneBits() == 4);
    CHECK(v.countZeroBits() == 96);
  }

  // Tail-word exactness: setting the final bit of a partial word must
  // not leak into countZeroBits (underflow regression).
  {
    BitVector v(3);
    v.setBit(0);
    v.setBit(2);
    CHECK(v.countOneBits() == 2);
    CHECK(v.countZeroBits() == 1);

    v.clearBit(0);
    v.clearBit(2);
    CHECK(v.countOneBits() == 0);
    CHECK(v.countZeroBits() == 3);
  }

  // Exact full word: 32 bits set, zero clear bits.
  {
    BitVector v(32);
    for (unsigned i = 0; i < 32; i++)
      v.setBit(i);
    CHECK(v.countOneBits() == 32);
    CHECK(v.countZeroBits() == 0);
  }

  // operator& with both operands empty: sizes match, must not throw.
  {
    BitVector a, b;
    BitVector c = a & b;
    CHECK(c.size() == 0);
    CHECK(c.countOneBits() == 0);
  }

  // setBit/clearBit are idempotent.
  {
    BitVector v(8);
    v.setBit(3);
    v.setBit(3);
    CHECK(v.countOneBits() == 1);
    v.clearBit(3);
    v.clearBit(3);
    CHECK(v.countOneBits() == 0);
  }

  // Self-move is a safe no-op.
  {
    BitVector v(10);
    v.setBit(4);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
    v = std::move(v);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    CHECK(v.size() == 10);
    CHECK(v.getBit(4));
  }

  // Copy of an empty vector: empty, and reusable.
  {
    BitVector e;
    BitVector f(e);
    CHECK(f.size() == 0);
    f.init(5);
    f.setBit(2);
    CHECK(f.size() == 5 && f.getBit(2));
  }

  // Move of an empty vector: still empty, still usable.
  {
    BitVector e;
    BitVector g(std::move(e));
    CHECK(g.size() == 0);
    g.init(2);
    CHECK(g.size() == 2);
  }

  // operator<< on a size-1 vector: minimal non-empty print.
  {
    ostringstream oss1;
    BitVector one(1);
    one.setBit(0);
    oss1 << one;
    CHECK(oss1.str() == "(1)");
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
