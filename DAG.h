#pragma once

#include <cstddef>
#include <ostream>
#include <utility>

#include "BitVector.h"
#include "Exception.h"
#include "types.h"
#include "Vec.h"

template <typename W> class DAG;
template <typename W>
std::ostream &operator<<(std::ostream &os, const DAG<W> &g);

// DirectedAcyclicGraph<W> (DAG<W>): a directed acyclic graph with
// W-weighted edges over vertices 0..n-1.
//
// The defining invariant is ENFORCED by addEdge: an edge that would
// create a directed cycle is rejected (false) after a reachability
// probe, so the object can never stop being a DAG -- no post-hoc
// validation needed by callers. Parallel edges and self-loops are also
// rejected.
//
// Reachability is a DFS over sorted fixed adjacency; acyclicity of an
// edge (u, v) follows from v being unreachable from u before the edge
// is added.
//
// Algorithms: topologicalOrder() (Kahn's algorithm; throws on the
// impossible cycle case), reachability (reachable(u, v)), longest path
// (edge count) via topological relaxation, transitiveClosure() as a
// bitvector, and incoming/outgoing views per vertex.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// W requirements: copyable, ==-comparable (weights participate in
// equality of graphs but not in acyclicity).
template <typename W> class DAG {
private:
  // Sorted dynamic array of (target, weight); the UndirectedGraph
  // convention flipped to out-edges only.
  struct Adj {
    struct E {
      E(u32 v, const W &w) : _v(v), _w(w) {}
      E() : _v(0), _w() {}
      u32 _v;
      W _w;
    };

    E *_e; // ascending by _v; null when empty
    u64 _n, _cap;

    Adj() : _e(0), _n(0), _cap(0) {}
    ~Adj() { delete[] _e; }

    Adj(const Adj &o) : _e(0), _n(o._n), _cap(o._n) {
      if (o._n) {
        _e = new E[o._n];
        for (u64 i = 0; i < o._n; i++)
          _e[i] = o._e[i];
      }
    }

    Adj &operator=(const Adj &o) {
      if (this == &o)
        return *this;
      E *fresh = o._n ? new E[o._n] : 0;
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._e[i];
      delete[] _e;
      _e = fresh;
      _n = o._n;
      _cap = o._n;
      return *this;
    }

    void steal(Adj &o) {
      _e = o._e;
      _n = o._n;
      _cap = o._cap;
      o._e = 0;
      o._n = o._cap = 0;
    }

    u64 locate(u32 v, bool &found) const {
      found = false;
      u64 lo = 0, hi = _n;
      while (lo < hi) {
        u64 mid = lo + (hi - lo) / 2;
        if (_e[mid]._v < v)
          lo = mid + 1;
        else
          hi = mid;
      }
      u64 idx = lo;
      found = idx < _n && _e[idx]._v == v;
      return idx;
    }

    void insert(u32 v, const W &w) {
      bool found;
      u64 idx = locate(v, found);
      if (found) {
        _e[idx]._w = w;
        return;
      }
      if (_n == _cap) {
        u64 nc = _cap ? _cap * 2 : 4;
        E *fresh = new E[nc];
        for (u64 i = 0; i < _n; i++)
          fresh[i] = _e[i];
        delete[] _e;
        _e = fresh;
        _cap = nc;
      }
      for (u64 i = _n; i > idx; i--)
        _e[i] = _e[i - 1];
      _e[idx] = E(v, w);
      _n++;
    }

    bool remove(u32 v) {
      bool found;
      u64 idx = locate(v, found);
      if (!found)
        return false;
      for (u64 i = idx; i + 1 < _n; i++)
        _e[i] = _e[i + 1];
      _n--;
      return true;
    }
  };

  Adj *_adj;  // out-edges per vertex; null when _V == 0
  u32 _V;
  u64 _E;

  void validateVertex(u32 v) const {
    if (v >= _V)
      throw Exception("vertex index outside the graph");
  }

  // Iterative DFS marking everything reachable from s (no recursion,
  // so deep graphs cannot overflow the stack).
  void markReachable(u32 s, bool *reach) const {
    u32 *stk = new u32[_V ? _V : 1];
    std::unique_ptr<u32[]> sGuard(stk);
    u64 top = 0;

    stk[top++] = s;
    reach[s] = true;

    while (top > 0) {
      u32 u = stk[--top];
      for (u64 i = 0; i < _adj[u]._n; i++) {
        u32 v = _adj[u]._e[i]._v;
        if (!reach[v]) {
          reach[v] = true;
          stk[top++] = v;
        }
      }
    }
  }

public:
  typedef u64 size_type;

  DAG() : _adj(0), _V(0), _E(0) {}

  explicit DAG(u32 vertices)
      : _adj(vertices ? new Adj[vertices] : 0), _V(vertices), _E(0) {}

  // Deep copy: same edges, same weights.
  DAG(const DAG &o) : _adj(o._V ? new Adj[o._V] : 0), _V(o._V), _E(o._E) {
    for (u32 i = 0; i < _V; i++)
      _adj[i] = o._adj[i];
  }

  // Steal o's edges; o left empty and reusable.
  DAG(DAG &&o) noexcept : _adj(o._adj), _V(o._V), _E(o._E) {
    o._adj = 0;
    o._V = 0;
    o._E = 0;
  }

  ~DAG() { delete[] _adj; }

  // Deep copy: built before the old is freed (strong safety).
  DAG &operator=(const DAG &o) {
    if (this == &o)
      return *this;

    Adj *fresh = o._V ? new Adj[o._V] : 0;
    for (u32 i = 0; i < o._V; i++)
      fresh[i] = o._adj[i];

    delete[] _adj;
    _adj = fresh;
    _V = o._V;
    _E = o._E;

    return *this;
  }

  // Steal o's edges; o left empty and reusable.
  DAG &operator=(DAG &&o) noexcept {
    if (this != &o) {
      delete[] _adj;
      _adj = o._adj;
      _V = o._V;
      _E = o._E;
      o._adj = 0;
      o._V = 0;
      o._E = 0;
    }
    return *this;
  }

  // Constant-time exchange of both graphs.
  void swap(DAG &o) {
    std::swap(_adj, o._adj);
    std::swap(_V, o._V);
    std::swap(_E, o._E);
  }

  u32 vertexCount() const { return _V; }

  u64 edgeCount() const { return _E; }

  bool empty() const { return _V == 0; }

  // Adds the directed edge u -> v with weight w. False (with no
  // mutation) when u == v, either endpoint is out of range, the
  // parallel edge exists, or the edge would create a cycle (i.e. v is
  // already reachable from u).
  bool addEdge(u32 u, u32 v, const W &w = W()) {
    if (u >= _V || v >= _V)
      return false;
    if (u == v)
      return false; // self-loop is a 1-cycle

    bool found;
    _adj[u].locate(v, found);
    if (found)
      return false; // parallel edge

    // acyclicity guard: v must not reach u (v -> ... -> u would close
    // the cycle u -> v -> ... -> u). Reachability through v's
    // subtrees only -- O(V + E) worst case.
    bool *reach = new bool[_V]();
    std::unique_ptr<bool[]> rGuard(reach);
    markReachable(v, reach);
    if (reach[u])
      return false; // would close a cycle

    _adj[u].insert(v, w);
    _E++;
    return true;
  }

  // Removes the edge u -> v when present, reporting success. Edges
  // REMOVED never break acyclicity, so no invariant check needed.
  bool removeEdge(u32 u, u32 v) {
    if (u >= _V || v >= _V)
      return false;
    if (!_adj[u].remove(v))
      return false;
    _E--;
    return true;
  }

  // True when the directed edge u -> v exists.
  bool hasEdge(u32 u, u32 v) const {
    if (u >= _V || v >= _V)
      throw Exception("vertex index outside the graph");
    bool found;
    _adj[u].locate(v, found);
    return found;
  }

  // The edge weight of u -> v, or a default W when absent.
  W weight(u32 u, u32 v) const {
    if (u >= _V || v >= _V)
      throw Exception("vertex index outside the graph");
    u64 idx;
    bool found;
    idx = _adj[u].locate(v, found);
    return found ? _adj[u]._e[idx]._w : W();
  }

  // Out-degree of u (edges leaving u); bounds-checked.
  u64 outDegree(u32 u) const {
    validateVertex(u);
    return _adj[u]._n;
  }

  // In-degree of v: edges entering v (bounds-checked). O(V log deg)?
  // maintained by scanning: O(V) — fine for pedagogy.
  u64 inDegree(u32 v) const {
    validateVertex(v);
    u64 d = 0;
    for (u32 u = 0; u < _V; u++) {
      bool found;
      _adj[u].locate(v, found);
      if (found)
        d++;
    }
    return d;
  }

  // Out-neighbors ascending.
  Vec<u32> outNeighbors(u32 u) const {
    validateVertex(u);
    Vec<u32> out(_adj[u]._n);
    for (u64 i = 0; i < _adj[u]._n; i++)
      out[i] = _adj[u]._e[i]._v;
    return out;
  }

  // In-neighbors ascending by source index (two passes: count, fill).
  Vec<u32> inNeighbors(u32 v) const {
    validateVertex(v);
    u64 n = inDegree(v);
    Vec<u32> out(n);
    u64 w = 0;
    for (u32 u = 0; u < _V; u++) {
      bool found;
      _adj[u].locate(v, found);
      if (found)
        out[w++] = u;
    }
    return out;
  }

  // True when v is reachable from u via 1+ edges. For u == v: bounded
  // to the 0-length walk; this reports yes (a vertex reaches itself
  // trivially in graph theory). Use reachableStrict to exclude.
  bool reachable(u32 u, u32 v) const {
    validateVertex(u);
    validateVertex(v);
    if (u == v)
      return true;

    bool *reach = new bool[_V]();
    std::unique_ptr<bool[]> rGuard(reach);
    markReachable(u, reach);
    return reach[v];
  }

  // reachable(u, v) ignoring the trivial u == v case.
  bool reachableStrict(u32 u, u32 v) const {
    validateVertex(u);
    validateVertex(v);
    if (u == v)
      return false;

    bool *reach = new bool[_V]();
    std::unique_ptr<bool[]> rGuard(reach);
    markReachable(u, reach);
    return reach[v];
  }

  // A topological order of the vertices (Kahn's algorithm, smallest
  // index first for determinism). Throws when no order exists -- which
  // addEdge makes impossible for a well-formed DAG.
  Vec<u32> topologicalOrder() const {
    // in-degrees
    u64 *indeg = new u64[_V ? _V : 1]();
    std::unique_ptr<u64[]> iGuard(indeg);
    for (u32 u = 0; u < _V; u++)
      for (u64 i = 0; i < _adj[u]._n; i++)
        indeg[_adj[u]._e[i]._v]++;

    // ready queue over vertex ids -- smallest-first via repeated linear
    // scan (O(V^2); simple and deterministic, fine for pedagogy)
    bool *done = new bool[_V ? _V : 1]();
    std::unique_ptr<bool[]> dGuard(done);

    Vec<u32> order(_V);
    u64 w = 0;
    for (u32 emitted = 0; emitted < _V; emitted++) {
      s32 pick = -1;
      for (u32 v = 0; v < _V; v++)
        if (!done[v] && indeg[v] == 0) {
          pick = s32(v); // smallest ready vertex
          break;
        }
      if (pick < 0)
        throw Exception("graph holds a cycle; no topological order");

      done[pick] = true;
      order[w++] = u32(pick);

      for (u64 i = 0; i < _adj[pick]._n; i++)
        indeg[_adj[pick]._e[i]._v]--;
    }

    return order;
  }

  // Length (EDGE COUNT) of the longest directed path, or 0 for an
  // empty DAG (no edges). Computed by relaxing along a topological
  // order -- the classic DAG dynamic program.
  u64 longestPathEdges() const {
    if (_V == 0)
      return 0;

    Vec<u32> order = topologicalOrder();

    u64 *best = new u64[_V]();
    std::unique_ptr<u64[]> bGuard(best);
    u64 longest = 0;

    for (u32 idx = 0; idx < _V; idx++) {
      u32 u = order[idx];
      for (u64 i = 0; i < _adj[u]._n; i++) {
        u32 v = _adj[u]._e[i]._v;
        if (best[u] + 1 > best[v])
          best[v] = best[u] + 1;
        if (best[v] > longest)
          longest = best[v];
      }
    }

    return longest;
  }

  // Transitive closure as a compressed bitmap: bit (u * _V + v) is 1
  // when v is reachable from u via 1+ edges. O(V * (V + E)).
  BitVector transitiveClosure() const {
    BitVector out(_V * _V);
    for (u32 u = 0; u < _V; u++) {
      bool *reach = new bool[_V]();
      std::unique_ptr<bool[]> rGuard(reach);
      markReachable(u, reach);
      for (u32 v = 0; v < _V; v++)
        if (reach[v] && u != v)
          out.setBit(u * _V + v);
    }
    return out;
  }

  // True when u has no out-edges (a sink).
  bool isSink(u32 u) const {
    validateVertex(u);
    return _adj[u]._n == 0;
  }

  // True when u has no in-edges (a source).
  bool isSource(u32 u) const {
    validateVertex(u);
    return inDegree(u) == 0;
  }

  // Drops every edge; the vertex set is kept.
  void clearEdges() {
    for (u32 i = 0; i < _V; i++)
      delete[] _adj[i]._e, _adj[i]._e = 0, _adj[i]._n = _adj[i]._cap = 0;
    _E = 0;
  }

  // True when both DAGs have the same vertices and edge sets.
  bool operator==(const DAG &o) const {
    if (_V != o._V || _E != o._E)
      return false;
    for (u32 u = 0; u < _V; u++) {
      if (_adj[u]._n != o._adj[u]._n)
        return false;
      for (u64 i = 0; i < _adj[u]._n; i++) {
        if (_adj[u]._e[i]._v != o._adj[u]._e[i]._v)
          return false;
        if (!(_adj[u]._e[i]._w == o._adj[u]._e[i]._w))
          return false;
      }
    }
    return true;
  }

  bool operator!=(const DAG &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os, const DAG<W> &g);
};

// Prints out-adjacency ascending: "u: v1 w1, v2 w2" per line.
template <typename W>
std::ostream &operator<<(std::ostream &os, const DAG<W> &g) {
  for (u32 u = 0; u < g._V; u++) {
    os << u << ":";
    for (u64 i = 0; i < g._adj[u]._n; i++) {
      os << " " << g._adj[u]._e[i]._v;
      if (!(g._adj[u]._e[i]._w == W()))
        os << "(" << g._adj[u]._e[i]._w << ")";
    }
    os << "\n";
  }
  return os;
}

template <typename W>
void swap(DAG<W> &a, DAG<W> &b) {
  a.swap(b);
}
