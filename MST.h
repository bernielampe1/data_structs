#pragma once

// Minimum spanning trees on UndirectedGraph<W>.
//
// Two classic algorithms sharing one interface: Kruskal (sort edges,
// grow with union-find by cheapest joining edge) and Prim (grow from a
// seed by the cheapest frontier edge, O(V^2) linear-scan pedagogy).
// Both return a BitVector edge mask -- bit (min(u,v) * V + max(u,v)) is
// 1 when edge {u, v} joins the tree -- and an out-param total weight.
// On a connected graph the mask describes a spanning tree with equal
// totals from both algorithms; on a disconnected graph a spanning
// FOREST (one tree per component, totals still equal).
//
// Counts: the number of chosen edges is V - (number of components),
// computable from the mask for verification.
//
// W requirements: arithmetic and <-comparable (ints and floats work).
#include <limits>

#include "BitVector.h"
#include "Exception.h"
#include "UndirectedGraph.h"
#include "types.h"

namespace mst_detail {

// Union-find with path halving (same shape as DisjointSet's public
// algorithm, local because MST.h owns no graph state).
inline u32 ufFind(u32 *p, u32 x) {
  while (p[x] != x) {
    p[x] = p[p[x]];
    x = p[x];
  }
  return x;
}

inline u64 maskCount(const BitVector &m, u32 V) {
  u64 c = 0;
  for (u32 u = 0; u < V; u++)
    for (u32 v = u + 1; v < V; v++)
      if (m.getBit(u64(u) * V + v))
        c++;
  return c;
}

} // namespace mst_detail

// Kruskal: collect every edge once (smaller index first), sort by
// weight (insertion sort: O(E^2) worst, simple and small), then scan,
// unioning the endpoints of every edge that joins two different trees.
template <typename W>
BitVector kruskalMST(const UndirectedGraph<W> &g, W &totalOut) {
  const u32 V = g.vertexCount();
  BitVector mask(u64(V) * V);
  totalOut = W();

  if (V < 2 || g.edgeCount() == 0)
    return mask;

  struct Edge {
    u32 _u, _v;
    W _w;
  };
  Edge *edges = new Edge[g.edgeCount()];
  std::unique_ptr<Edge[]> eGuard(edges);
  u64 nEdges = 0;

  for (u32 u = 0; u < V; u++) {
    Vec<u32> nb = g.neighbors(u);
    Vec<W> ws = g.neighborWeights(u);
    for (u64 i = 0; i < nb.len(); i++) {
      u32 v = nb[i];
      if (v < u)
        continue; // each edge once, (min, max) orientation
      edges[nEdges]._u = u;
      edges[nEdges]._v = v;
      edges[nEdges]._w = ws[i];
      nEdges++;
    }
  }

  for (u64 i = 1; i < nEdges; i++) { // insertion sort by weight
    Edge e = edges[i];
    u64 j = i;
    while (j > 0 && edges[j - 1]._w > e._w) {
      edges[j] = edges[j - 1];
      j--;
    }
    edges[j] = e;
  }

  u32 *parent = new u32[V];
  std::unique_ptr<u32[]> pGuard(parent);
  for (u32 i = 0; i < V; i++)
    parent[i] = i;

  u64 taken = 0;
  for (u64 i = 0; i < nEdges; i++) {
    u32 ru = mst_detail::ufFind(parent, edges[i]._u);
    u32 rv = mst_detail::ufFind(parent, edges[i]._v);
    if (ru == rv)
      continue; // would close a cycle

    parent[ru] = rv;
    u32 lo = edges[i]._u, hi = edges[i]._v;
    if (hi < lo) {
      u32 t = lo;
      lo = hi;
      hi = t;
    }
    mask.setBit(u64(lo) * V + hi);
    totalOut = totalOut + edges[i]._w;
    taken++;

    // spanning forest completes at V - components edges; taking more
    // is impossible (every join edge already taken), no early exit
    // needed for correctness -- but skipping the scan is free:
    if (V > 0 && taken == V - 1)
      break; // tree case fully spanned
  }

  return mask;
}

// Prim: grow one tree from seed by repeatedly adding the cheapest edge
// with exactly one endpoint inside. Over connected components, call
// per remaining component seed; the mask covers the whole graph
// (vertices unreachable from the seed are skipped: that is a FOREST,
// not an error).
template <typename W>
BitVector primMST(const UndirectedGraph<W> &g, u32 seed, W &totalOut) {
  const u32 V = g.vertexCount();
  BitVector mask(u64(V) * V);
  totalOut = W();

  if (V == 0 || seed >= V)
    throw Exception("Prim: seed outside the graph");

  bool *inTree = new bool[V]();
  std::unique_ptr<bool[]> tGuard(inTree);
  inTree[seed] = true;

  for (;;) {
    // scan every tree-boundary edge for the global cheapest crossing
    u32 bestU = V, bestV = V;
    W bestW = std::numeric_limits<W>::max();

    for (u32 u = 0; u < V; u++) {
      if (!inTree[u])
        continue;
      Vec<u32> nb = g.neighbors(u);
      Vec<W> ws = g.neighborWeights(u);
      for (u64 i = 0; i < nb.len(); i++) {
        u32 v = nb[i];
        if (inTree[v])
          continue;
        if (ws[i] < bestW) {
          bestW = ws[i];
          bestU = u;
          bestV = v;
        }
      }
    }

    if (bestU == V)
      break; // no crossing edge left: this tree is maximal

    inTree[bestV] = true;
    u32 lo = bestU, hi = bestV;
    if (hi < lo) {
      u32 t = lo;
      lo = hi;
      hi = t;
    }
    mask.setBit(u64(lo) * V + hi);
    totalOut = totalOut + bestW;
  }

  return mask;
}

// Prim over ALL components: sweeps seeds so the mask spans every
// component (the Kruskal contract). Totals match kruskalMST's exactly.
template <typename W>
BitVector primForestMST(const UndirectedGraph<W> &g, W &totalOut) {
  const u32 V = g.vertexCount();
  BitVector out(u64(V) * V);
  totalOut = W();

  if (V == 0)
    return out;

  bool *done = new bool[V]();
  std::unique_ptr<bool[]> dGuard(done);

  for (u32 s = 0; s < V; s++) {
    if (done[s])
      continue;

    // grow this component's tree from s, recording into the shared
    // structures so subsequent seeds skip covered vertices
    done[s] = true;
    for (;;) {
      u32 bestU = V, bestV = V;
      W bestW = std::numeric_limits<W>::max();
      for (u32 u = 0; u < V; u++) {
        if (!done[u])
          continue;
        Vec<u32> nb = g.neighbors(u);
        Vec<W> ws = g.neighborWeights(u);
        for (u64 i = 0; i < nb.len(); i++) {
          u32 v = nb[i];
          if (done[v])
            continue;
          if (ws[i] < bestW) {
            bestW = ws[i];
            bestU = u;
            bestV = v;
          }
        }
      }
      if (bestU == V)
        break; // component complete

      done[bestV] = true;
      u32 lo = bestU, hi = bestV;
      if (hi < lo) {
        u32 t = lo;
        lo = hi;
        hi = t;
      }
      out.setBit(u64(lo) * V + hi);
      totalOut = totalOut + bestW;
    }
  }

  return out;
}

// Convenience: count of chosen edges in a mask.
inline u64 mstEdgeCount(const BitVector &mask, u32 V) {
  return mst_detail::maskCount(mask, V);
}
