#pragma once

#include <cstddef>
#include <ostream>
#include <utility>

#include "BitVector.h"
#include "Exception.h"
#include "types.h"
#include "Vec.h"

template <typename W> class DirectedGraph;
template <typename W>
std::ostream &operator<<(std::ostream &os, const DirectedGraph<W> &g);

// DirectedGraph<W>: a general digraph (cycles allowed) over vertices
// 0..n-1 with W-weighted edges. This is the missing family member
// between UndirectedGraph (bidirectional, undirected walks) and DAG
// (permanently acyclic): here cycles exist and are DETECTED rather
// than rejected at insertion.
//
// Structural: parallel edges and self-loops are rejected by addEdge
// (self-loops are real - and they are cycles); direction is kept in
// out-edge lists sorted ascending.
//
// Algorithms: hasCycle() (iterative DFS with 3-color marking),
// stronglyConnectedComponents() (Tarjan, iterative, component ids per
// vertex), reachable() (DFS), cycleThrough(u, v) membership,
// transitiveClosure() BitVector, and the topologicalOrder() contract
// DIFFERENT from DAG's: here it THROWS when cyclic (report the fact;
// the DAG class instead makes cycles impossible). isDAG() tells the
// caller which contract applies.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// W requirements: copyable, ==-comparable.
template <typename W> class DirectedGraph {
private:
  struct Adj {
    struct E {
      E(u32 v, const W &w) : _v(v), _w(w) {}
      E() : _v(0), _w() {}
      u32 _v;
      W _w;
    };

    E *_e;
    u64 _n = 0, _cap = 0;
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
      if (found)
        return; // caller-checked parallel
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

  Adj *_adj;
  u32 _V;
  u64 _E;

  void validateVertex(u32 v) const {
    if (v >= _V)
      throw Exception("vertex index outside the graph");
  }

  // Iterative DFS marking everything reachable from s.
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

  DirectedGraph() : _adj(0), _V(0), _E(0) {}

  explicit DirectedGraph(u32 vertices)
      : _adj(vertices ? new Adj[vertices] : 0), _V(vertices), _E(0) {}

  DirectedGraph(const DirectedGraph &o)
      : _adj(o._V ? new Adj[o._V] : 0), _V(o._V), _E(o._E) {
    for (u32 i = 0; i < _V; i++)
      _adj[i] = o._adj[i];
  }

  DirectedGraph(DirectedGraph &&o) noexcept
      : _adj(o._adj), _V(o._V), _E(o._E) {
    o._adj = 0;
    o._V = 0;
    o._E = 0;
  }

  ~DirectedGraph() { delete[] _adj; }

  DirectedGraph &operator=(const DirectedGraph &o) {
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

  DirectedGraph &operator=(DirectedGraph &&o) noexcept {
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

  void swap(DirectedGraph &o) {
    std::swap(_adj, o._adj);
    std::swap(_V, o._V);
    std::swap(_E, o._E);
  }

  u32 vertexCount() const { return _V; }

  u64 edgeCount() const { return _E; }

  bool empty() const { return _V == 0; }

  // Adds the directed edge u -> v with weight w. False (no mutation)
  // for out-of-range endpoints, self-loops, or parallel edges; cycles
  // are ACCEPTED here (that is what distinguishes this class from DAG).
  bool addEdge(u32 u, u32 v, const W &w = W()) {
    if (u >= _V || v >= _V)
      return false;
    if (u == v)
      return false;

    bool found;
    _adj[u].locate(v, found);
    if (found)
      return false;

    _adj[u].insert(v, w);
    _E++;
    return true;
  }

  // Reverse edge lookup: hasEdge(v, u) -- the directed complement.
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

  u64 outDegree(u32 u) const {
    validateVertex(u);
    return _adj[u]._n;
  }

  bool hasEdge(u32 u, u32 v) const {
    if (u >= _V || v >= _V)
      throw Exception("vertex index outside the graph");
    bool found;
    _adj[u].locate(v, found);
    return found;
  }

  W weight(u32 u, u32 v) const {
    if (u >= _V || v >= _V)
      throw Exception("vertex index outside the graph");
    u64 idx;
    bool found;
    idx = _adj[u].locate(v, found);
    return found ? _adj[u]._e[idx]._w : W();
  }

  bool removeEdge(u32 u, u32 v) {
    if (u >= _V || v >= _V)
      return false;
    if (!_adj[u].remove(v))
      return false;
    _E--;
    return true;
  }

  Vec<u32> outNeighbors(u32 u) const {
    validateVertex(u);
    Vec<u32> out(_adj[u]._n);
    for (u64 i = 0; i < _adj[u]._n; i++)
      out[i] = _adj[u]._e[i]._v;
    return out;
  }

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

  // True when a directed cycle exists (3-color iterative DFS: WHITE =
  // unvisited, GRAY = on the exploration stack, BLACK = finished; a GRAY
  // revisit is a back edge, i.e. a cycle).
  bool hasCycle() const {
    // 0 white, 1 gray, 2 black
    u8 *color = new u8[_V ? _V : 1]();
    std::unique_ptr<u8[]> cGuard(color);

    // explicit DFS with (vertex, out-edge cursor) stack frames
    u64 *frames = new u64[(_V ? _V : 1) * 2];
    std::unique_ptr<u64[]> fGuard(frames);
    u64 top = 0;

    for (u32 s = 0; s < _V; s++) {
      if (color[s] != 0)
        continue;

      color[s] = 1;
      frames[2 * top] = s;
      frames[2 * top + 1] = 0;
      top++;

      while (top > 0) {
        u32 u = u32(frames[2 * (top - 1)]);
        u64 &ci = frames[2 * (top - 1) + 1];

        if (ci < _adj[u]._n) {
          u32 v = _adj[u]._e[ci]._v;
          ci++;

          if (color[v] == 1)
            return true; // back edge to a GRAY vertex: cycle
          if (color[v] == 0) {
            color[v] = 1;
            frames[2 * top] = v;
            frames[2 * top + 1] = 0;
            top++;
          }
        } else {
          color[u] = 2; // finished: black
          top--;
        }
      }
    }

    return false;
  }

  // convenience: the DAG-compatibility report
  bool isDAG() const { return !hasCycle(); }

  // Tarjan's strongly connected components (iterative, component ids
  // 0..k-1 assigned in reverse topological order; components[v] names
  // v's component). O(V + E).
  Vec<u32> stronglyConnectedComponents() const {
    const u32 NONE = u32(-1);
    Vec<u32> components(_V);
    for (u32 i = 0; i < _V; i++)
      components[i] = NONE;

    // iterative Tarjan state
    u32 *index = new u32[_V ? _V : 1]();
    u32 *low = new u32[_V ? _V : 1]();
    bool *onStack = new bool[_V ? _V : 1]();
    u32 *stack = new u32[_V ? _V : 1]();
    u64 stackTop = 0;

    // per-thread DFS frames: (vertex, cursor)
    u64 *frames = new u64[(_V ? _V : 1) * 2];
    u64 frameTop = 0;

    std::unique_ptr<u32[]> iGuard(index), lGuard(low), sGuard(stack);
    std::unique_ptr<bool[]> oGuard(onStack);
    std::unique_ptr<u64[]> fGuard(frames);

    u32 nextIndex = 0;
    u32 nextComponent = 0;

    for (u32 s = 0; s < _V; s++) {
      if (components[s] != NONE)
        continue;

      index[s] = low[s] = nextIndex++;
      stack[stackTop++] = s;
      onStack[s] = true;
      frames[2 * frameTop] = s;
      frames[2 * frameTop + 1] = 0;
      frameTop++;

      while (frameTop > 0) {
        u32 u = u32(frames[2 * (frameTop - 1)]);
        u64 &ci = frames[2 * (frameTop - 1) + 1];

        if (ci < _adj[u]._n) {
          u32 v = _adj[u]._e[ci]._v;
          ci++;

          if (!onStack[v] && components[v] == NONE) {
            // white (unvisited beyond this DFS): descend
            index[v] = low[v] = nextIndex++;
            stack[stackTop++] = v;
            onStack[v] = true;
            frames[2 * frameTop] = v;
            frames[2 * frameTop + 1] = 0;
            frameTop++;
          } else if (onStack[v]) {
            // back or cross edge into a node on the Tarjan stack
            if (index[v] < low[u])
              low[u] = index[v];
          }
        } else {
          // leaving u: fold low into parent, then pop the component
          frameTop--;
          if (frameTop > 0) {
            u32 parent = u32(frames[2 * (frameTop - 1)]);
            if (low[u] < low[parent])
              low[parent] = low[u];
          } else {
            // popped the DFS root: inherits nothing
          }

          if (low[u] == index[u]) { // u is an SCC root
            for (;;) {
              u32 v = stack[--stackTop];
              onStack[v] = false;
              components[v] = nextComponent;
              if (v == u)
                break;
            }
            nextComponent++;
          }
        }
      }
    }

    return components;
  }

  // True when u and v end up in the same SCC (mutually reachable,
  // including the trivial single-vertex case).
  bool stronglyConnected(u32 u, u32 v) const {
    validateVertex(u);
    validateVertex(v);
    Vec<u32> comps = stronglyConnectedComponents();
    return comps[u] == comps[v];
  }

  // Topological order: throws when the graph is cyclic (the caller
  // negotiates with isDAG first); Kahn's smallest-first otherwise.
  Vec<u32> topologicalOrder() const {
    if (hasCycle())
      throw Exception("graph holds a cycle; no topological order");

    u64 *indeg = new u64[_V ? _V : 1]();
    std::unique_ptr<u64[]> iGuard(indeg);
    for (u32 u = 0; u < _V; u++)
      for (u64 i = 0; i < _adj[u]._n; i++)
        indeg[_adj[u]._e[i]._v]++;

    bool *done = new bool[_V ? _V : 1]();
    std::unique_ptr<bool[]> dGuard(done);

    Vec<u32> order(_V);
    u64 w = 0;
    for (u32 emitted = 0; emitted < _V; emitted++) {
      s32 pick = -1;
      for (u32 v = 0; v < _V; v++)
        if (!done[v] && indeg[v] == 0) {
          pick = s32(v);
          break;
        }
      if (pick < 0)
        throw Exception("graph holds a cycle; no topological order");

      done[pick] = true;
      order[w++] = u32(pick);

      for (u64 i = 0; i < _adj[u32(pick)]._n; i++)
        indeg[_adj[u32(pick)]._e[i]._v]--;
    }

    return order;
  }

  // Compressed-closure reachability bitvector (i). Transitive closure
  // including the diagonal (u reaches itself trivially recorded as 0:
  // strict reachability only, matching DAG's closure).
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

  // True when u -> v is an edge AND v -> u exists too (2-cycles use
  // the pair form).
  bool isBidirectionalPair(u32 u, u32 v) const {
    validateVertex(u);
    validateVertex(v);
    return hasEdge(u, v) && hasEdge(v, u);
  }

  void clearEdges() {
    for (u32 i = 0; i < _V; i++)
      delete[] _adj[i]._e, _adj[i]._e = 0, _adj[i]._n = _adj[i]._cap = 0;
    _E = 0;
  }

  bool operator==(const DirectedGraph &o) const {
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

  bool operator!=(const DirectedGraph &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os,
                                    const DirectedGraph<W> &g);
};

// Prints out-adjacency ascending: "u: v1 w1, v2 w2" per line.
template <typename W>
std::ostream &operator<<(std::ostream &os, const DirectedGraph<W> &g) {
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
void swap(DirectedGraph<W> &a, DirectedGraph<W> &b) {
  a.swap(b);
}
