// Tests for Deque<T>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:  make dequeTest && ./dequeTest

#include "Deque.h"
#include <deque>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

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

static unsigned seed = 20260915u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh deque.
  {
    Deque<int> d;
    CHECK(d.empty());
    CHECK(d.size() == 0);
    CHECK(d.capacity() == 0);
  }

  // Both-end insertions keep logical order.
  {
    Deque<int> d;
    d.push_back(2);
    d.push_back(3);
    d.push_front(1); // prepends
    d.push_back(4);

    CHECK(d.size() == 4);
    CHECK(d.front() == 1 && d.back() == 4);
    CHECK(d[0] == 1 && d[1] == 2 && d[2] == 3 && d[3] == 4);
  }

  // Both-end removals.
  {
    Deque<int> d;
    for (int i = 0; i < 6; i++)
      d.push_back(i);

    d.pop_front(); // removes 0
    d.pop_back();  // removes 5
    CHECK(d.size() == 4);
    CHECK(d.front() == 1 && d.back() == 4);

    d.pop_front();
    d.pop_front();
    d.pop_back();
    d.pop_back();
    CHECK(d.empty());

    // popping empty is a safe no-op
    d.pop_front();
    d.pop_back();
    CHECK(d.empty());
  }

  // Ring wraparound: the front recedes past slot 0 and wraps; logical
  // order is preserved through the wrap boundary.
  {
    Deque<int> d(0 == 0 ? Deque<int>() : Deque<int>());
    d.push_back(0);
    for (int i = 1; i < 12; i++)
      d.push_back(i);
    // capacity doubles as needed; front is at slot 0 now.
    // wrap the FRONT backwards: re-filling after front-pop cycles
    for (int round = 0; round < 3; round++) {
      for (int i = 0; i < 6; i++)
        d.push_back(100 * round + i);
      for (int i = 0; i < 6; i++)
        d.pop_front();
    }

    // after 3 rounds the ring holds [100..105, 200..205] (each
    // round's pops consume the previous head block)
    CHECK(d.size() == 12);
    bool inOrder = true;
    for (int i = 0; i < 6; i++)
      if (d[u64(i)] != 100 + i || d[u64(6 + i)] != 200 + i)
        inOrder = false;
    CHECK(inOrder); // logical order survived the ring arithmetic

    // front pops move _front past the wrap; pushes land behind it
    for (int i = 0; i < 10; i++) {
      d.pop_front();
      d.push_back(-i);
    }
    // from [100..105, 200..205, 300..305]: 10 pop/push rounds leave
    // [305? no: pops through the head, pushes append -i...]
    // exact: pop 100..105 (6), pop 200..201 (4 more = 10 pops);
    // remaining [202, 203, 204, 205, 300..305]; pushed -0..-9 appended
    // probe-verified: pops consumed everything through 203? no --
    // each round pops ONE and pushes ONE, so 10 rounds leave the 2
    // surviving 100s/200s plus the 10 appended: [204, 205, 0, -1..-9]
    CHECK(d.front() == 204);
    CHECK(d[1] == 205);
    CHECK(d[2] == 0);   // first appended: -0 == 0
    CHECK(d[11] == -9); // last appended
    CHECK(d.back() == -9);
    bool tailOrder = true; // the appended values run -0..-9 in order
    for (int i = 0; i < 10; i++)
      if (d[u64(2 + i)] != -i)
        tailOrder = false;
    CHECK(tailOrder);
  }

  // at() bounds checks; operator[] unchecked but correct in ring state.
  {
    Deque<int> d;
    for (int i = 0; i < 5; i++)
      d.push_back(i);
    d.push_front(-1);
    CHECK(d.at(0) == -1 && d.at(5) == 4);

    bool threw = false;
    try { d.at(6); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // clear() forgets elements, keeps capacity, reusable.
  {
    Deque<int> d;
    for (int i = 0; i < 10; i++)
      d.push_back(i);
    u64 capBefore = d.capacity();
    d.clear();
    CHECK(d.size() == 0);
    CHECK(d.capacity() == capBefore);
    d.push_front(1);
    CHECK(d.size() == 1 && d.front() == 1);
    d.clear();
    d.clear();
    CHECK(d.empty());
  }

  // Growth: many pushes from BOTH ends keep contents and order.
  {
    Deque<int> d;
    for (int i = 0; i < 500; i++) {
      if (i % 2 == 0)
        d.push_back(i);
      else
        d.push_front(i);
    }

    // odds in descending order at the front, evens ascending after;
    // logical: [499, 497, ..., 1, 0, 2, ..., 498]
    CHECK(d.size() == 500);
    CHECK(d.front() == 499);
    CHECK(d[0] == 499 && d[1] == 497 && d[2] == 495);
    CHECK(d[249] == 1);       // last of the pushed-front odds
    CHECK(d[250] == 0);       // first pushed-back even
    CHECK(d[251] == 2);
    CHECK(d.back() == 498);
  }

  // Copy construction is deep and preserves logical order even when
  // the source's ring has wrapping.
  {
    Deque<int> src;
    for (int i = 0; i < 4; i++)
      src.push_back(i);
    for (int i = 0; i < 10; i++) {
      src.pop_front();
      src.push_back(i);
    }
    // src is now logically [4, i's...] with the front wrapped deep

    Deque<int> cpy(src);
    CHECK(cpy.size() == src.size());
    bool same = true;
    for (u64 i = 0; i < src.size(); i++)
      if (cpy[i] != src[i])
        same = false;
    CHECK(same);

    // deep: the copy mutates only itself. Source recap: [0..3], then
    // 10 pop_front/push_back rounds leave [6, 7, 8, 9] (each round
    // consumes the head and appends at the tail; size stays 4)
    CHECK(src.size() == 4);
    CHECK(src.front() == 6 && src[1] == 7 && src[2] == 8 && src[3] == 9);

    cpy.pop_front();  // remove 6
    cpy.push_back(99);
    CHECK(cpy.size() == 4);
    CHECK(cpy.front() == 7 && cpy.back() == 99);
    CHECK(src.back() == 9); // the source never saw 99
  }

  // Copy assignment replaces; self-assignment no-op.
  {
    Deque<int> a;
    a.push_back(1);
    a.push_back(2);
    Deque<int> b;
    b.push_back(77);
    b = a;
    CHECK(b.size() == 2);
    CHECK(b.front() == 1 && b.back() == 2);
    CHECK(!(b.front() == 77 || b.back() == 77)); // no stale residue

    Deque<int> &alias = a;
    a = alias; // self-assignment
    CHECK(a.size() == 2);
    CHECK(a.front() == 1);
  }

  // Move construction steals; source empty and reusable.
  {
    Deque<int> a;
    for (int i = 0; i < 8; i++)
      a.push_back(i);
    Deque<int> m(std::move(a));
    CHECK(m.size() == 8);
    CHECK(m.front() == 0 && m.back() == 7);

    CHECK(a.size() == 0);
    CHECK(a.empty());
    a.push_back(5); // reusable
    CHECK(a.size() == 1 && a.back() == 5);
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    Deque<int> a;
    a.push_back(1);
    a.push_back(2);
    Deque<int> b;
    b.push_back(66);
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b.front() == 1);

    CHECK(a.empty());
    a.push_front(9); // reusable
    CHECK(a.front() == 9);

    Deque<int> s;
    s.push_back(3);
    Deque<int> &alias = s;
    s = std::move(alias);
    CHECK(s.size() == 1 && s.front() == 3);
  }

  // swap exchanges (member and free).
  {
    Deque<int> s1, s2;
    s1.push_back(1);
    s2.push_back(2);
    s2.push_back(3);
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.back() == 3 && s2.front() == 1);

    swap(s1, s2);
    CHECK(s1.size() == 1 && s2.size() == 2);
    CHECK(s1.front() == 1);
  }

  // operator<< prints front to back with no trailing separator.
  {
    Deque<int> d;
    d.push_back(1);
    d.push_back(2);
    d.push_front(0);
    ostringstream oss;
    oss << d;
    CHECK(oss.str() == "0, 1, 2");
    CHECK(d.size() == 3); // printing did not consume

    Deque<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Stack and Queue equivalence: the Deque supports both disciplines;
  // using one end only reproduces Stack, one-in-one-out reproduces
  // Queue (the pedagogical bridge this class exists to make visible).
  {
    Deque<int> asStack;
    for (int i = 0; i < 5; i++)
      asStack.push_front(i); // push
    bool lifo = true;
    for (int i = 0; i < 5; i++) {
      if (asStack.front() != 4 - i)
        lifo = false;
      asStack.pop_front(); // pop
    }
    CHECK(lifo);

    Deque<int> asQueue;
    for (int i = 0; i < 5; i++)
      asQueue.push_back(i); // enqueue
    bool fifo = true;
    for (int i = 0; i < 5; i++) {
      if (asQueue.front() != i)
        fifo = false;
      asQueue.pop_front(); // dequeue
    }
    CHECK(fifo);
  }

  // Randomized stress: Deque against std::deque as the reference
  // model, exercising every public operation with random choices.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      Deque<int> mine;
      std::deque<int> model;

      for (int op = 0; op < 500; op++) {
        int roll = int(rnd() % 5);
        int v = int(rnd() % 1000);
        if (roll == 0) {
          mine.push_back(v);
          model.push_back(v);
        } else if (roll == 1) {
          mine.push_front(v);
          model.push_front(v);
        } else if (roll == 2) {
          mine.pop_back();
          if (!model.empty())
            model.pop_back();
        } else if (roll == 3) {
          mine.pop_front();
          if (!model.empty())
            model.pop_front();
        } else {
          if (mine.empty() != model.empty())
            allOk = false;
          if (mine.size() != model.size())
            allOk = false;
          if (!model.empty()) {
            if (mine.front() != model.front() ||
                mine.back() != model.back())
              allOk = false;
          }
        }
        if (mine.size() != model.size())
          allOk = false;
      }

      // final whole-content agreement (mid-exit pops drop both sides)
      if (mine.size() == model.size()) {
        bool same = true;
        for (u64 i = 0; i < mine.size(); i++)
          if (mine[i] != model[i])
            same = false;
        if (!same)
          allOk = false;
      } else {
        allOk = false;
      }
    }
    CHECK(allOk);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
