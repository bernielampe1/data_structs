// Tests for UndirectedGraph<W>. Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make undirectedGraphTest && ./undirectedGraphTest

#include "UndirectedGraph.h"
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

template <typename A, typename B>
bool near(A a, B b, double eps = 1e-9) {
  return double(a > b ? a - b : b - a) <= eps;
}

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 2718281828u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Empty and sized graphs.
  {
    UndirectedGraph<int> g;
    CHECK(g.vertexCount() == 0);
    CHECK(g.edgeCount() == 0);
    CHECK(g.empty());

    UndirectedGraph<int> g4(4);
    CHECK(g4.vertexCount() == 4);
    CHECK(g4.edgeCount() == 0);
    CHECK(!g4.empty());
    CHECK(!g4.isConnected()); // 4 isolated vertices: not one component
    CHECK(g4.connectedComponents() == 4); // each vertex its own component
    CHECK(g4.isBipartite()); // no edges: vacuously bipartite
    CHECK(!g4.cycleExists());
  }

  // addEdge: symmetry, self-loop/parallel rejection, weights.
  {
    UndirectedGraph<int> g(3);
    CHECK(g.addEdge(0, 1, 5));
    CHECK(g.edgeCount() == 1);
    CHECK(g.hasEdge(0, 1) && g.hasEdge(1, 0)); // symmetric storage
    CHECK(g.weight(0, 1) == 5 && g.weight(1, 0) == 5);

    CHECK(!g.addEdge(1, 0)); // parallel (same edge both ways)
    CHECK(g.edgeCount() == 1);
    CHECK(!g.addEdge(2, 2)); // self-loop
    CHECK(!g.addEdge(0, 3)); // out of range

    CHECK(g.addEdge(1, 2)); // default weight
    CHECK(g.weight(1, 2) == 0 && g.weight(2, 1) == 0);

    CHECK(!g.removeEdge(0, 2)); // never existed
    CHECK(g.removeEdge(0, 1));
    CHECK(g.edgeCount() == 1);
    CHECK(!g.hasEdge(1, 0));
  }

  // degree / neighbors / neighborWeights (sorted ascending).
  {
    UndirectedGraph<int> g(5);
    g.addEdge(3, 0, 1);
    g.addEdge(3, 2, 2);
    g.addEdge(3, 1, 3);
    g.addEdge(3, 4, 4);

    CHECK(g.degree(3) == 4);
    CHECK(g.degree(0) == 1);

    Vec<u32> nb = g.neighbors(3);
    CHECK(nb.len() == 4);
    CHECK(nb[0] == 0 && nb[1] == 1 && nb[2] == 2 && nb[3] == 4); // ascending

    Vec<int> ws = g.neighborWeights(3);
    CHECK(ws[0] == 1 && ws[1] == 3 && ws[2] == 2 && ws[3] == 4);

    bool threw = false;
    try { g.degree(9); } catch (Exception &) { threw = true; }
    CHECK(threw);

    CHECK(g.isIsolated(0) == false);
    UndirectedGraph<int> h(2);
    CHECK(h.isIsolated(0)); // no edges
  }

  // BFS distances on a path graph.
  {
    UndirectedGraph<int> g(6);
    for (int i = 0; i < 5; i++)
      g.addEdge(u32(i), u32(i + 1));

    Vec<u64> d = g.breadthFirst(0);
    CHECK(d.len() == 6);
    CHECK(d[0] == 0 && d[3] == 3 && d[5] == 5);

    Vec<u64> d3 = g.breadthFirst(3); // middle: [3,2,1,0,1,2]
    CHECK(d3[0] == 3 && d3[2] == 1 && d3[3] == 0 && d3[5] == 2);

    bool threw = false;
    try { g.breadthFirst(6); } catch (Exception &) { threw = true; }
    CHECK(threw);

    // disconnected vertex keeps u64(-1)
    UndirectedGraph<int> g2(3);
    g2.addEdge(0, 1);
    Vec<u64> dd = g2.breadthFirst(0);
    CHECK(dd[2] == u64(-1));
  }

  // DFS visit order is a valid traversal (every reachable vertex).
  {
    UndirectedGraph<int> g(5);
    g.addEdge(0, 1);
    g.addEdge(0, 2);
    g.addEdge(1, 3);
    g.addEdge(2, 4);

    // visit count reaches every vertex; first visit = 0
    int order[5];
    int count = 0;
    g.depthFirst(0, [&](u32 v) { order[count++] = int(v); });
    CHECK(count == 5);
    CHECK(order[0] == 0);

    // the twice-visited smallest-neighbor property: 0's first child
    // visited after 0 is the smallest neighbor (1), not 2
    CHECK(order[1] == 1);

    bool threw = false;
    try { g.depthFirst(9, [](u32) {}); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Connected components on a 3-island graph.
  {
    UndirectedGraph<int> g(9);
    g.addEdge(0, 1); g.addEdge(1, 2);      // island {0,1,2}
    g.addEdge(3, 4);                        // island {3,4}
    // island {5,6,7,8}: a square
    g.addEdge(5, 6); g.addEdge(6, 7); g.addEdge(7, 8); g.addEdge(8, 5);

    CHECK(g.connectedComponents() == 3);
    CHECK(!g.isConnected());
    CHECK(g.pathExists(0, 2));
    CHECK(g.pathExists(5, 7));
    CHECK(!g.pathExists(0, 3));

    // square is a cycle
    CHECK(g.cycleExists());

    CHECK(g.isBipartite()); // even cycle
  }

  // Cycle detection on classic shapes.
  {
    UndirectedGraph<int> tree(5);
    tree.addEdge(0, 1); tree.addEdge(1, 2); tree.addEdge(1, 3);
    tree.addEdge(3, 4);
    CHECK(!tree.cycleExists());
    CHECK(tree.isConnected());

    tree.addEdge(2, 4); // closes a cycle
    CHECK(tree.cycleExists());

    // single edge: no cycle
    UndirectedGraph<int> one(2);
    one.addEdge(0, 1);
    CHECK(!one.cycleExists());
  }

  // Bipartite on the odd-cycle (triangle) and complete-bipartite cases.
  {
    UndirectedGraph<int> triangle(3);
    triangle.addEdge(0, 1); triangle.addEdge(1, 2); triangle.addEdge(2, 0);
    CHECK(!triangle.isBipartite());

    // K_{2,2}: bipartite with cross edges
    UndirectedGraph<int> k22(4);
    k22.addEdge(0, 2); k22.addEdge(0, 3);
    k22.addEdge(1, 2); k22.addEdge(1, 3);
    CHECK(k22.isBipartite());
    CHECK(k22.cycleExists()); // 4-cycles are cycles
  }

  // pathExists: u == v, direct edge, multi-hop, disconnected.
  {
    UndirectedGraph<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(1, 2);

    CHECK(g.pathExists(0, 0)); // trivial
    CHECK(g.pathExists(0, 1));
    CHECK(g.pathExists(0, 2)); // via 1
    CHECK(!g.pathExists(0, 3));

    bool threw = false;
    try { g.pathExists(0, 4); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // clearEdges() keeps vertices.
  {
    UndirectedGraph<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(2, 3);
    g.clearEdges();
    CHECK(g.edgeCount() == 0);
    CHECK(g.vertexCount() == 4);
    CHECK(!g.hasEdge(0, 1)); // would throw now? no: valid indexes
    CHECK(g.connectedComponents() == 4);

    g.addEdge(1, 2); // reusable
    CHECK(g.edgeCount() == 1);
  }

  // operator== / != (structure and weights).
  {
    UndirectedGraph<int> a(3), b(3);
    a.addEdge(0, 1, 7);
    b.addEdge(1, 0, 7); // reversed orientation of the same edge
    CHECK(a == b); // undirected: same edge

    b.removeEdge(0, 1);
    b.addEdge(0, 1, 8);
    CHECK(a != b); // weight differs

    UndirectedGraph<int> c(4);
    c.addEdge(0, 1, 7);
    CHECK(a != c); // different vertex counts
  }

  // Copy construction deep; assignment replaces; self-assign no-op.
  {
    UndirectedGraph<int> src(4);
    src.addEdge(0, 1, 1);
    src.addEdge(2, 3, 2);

    UndirectedGraph<int> cp(src);
    CHECK(cp.vertexCount() == 4);
    CHECK(cp.edgeCount() == 2);
    CHECK(cp.weight(0, 1) == 1);

    cp.addEdge(0, 2);
    cp.removeEdge(2, 3);
    CHECK(cp.edgeCount() == 2);
    CHECK(src.edgeCount() == 2); // source untouched
    CHECK(!src.hasEdge(0, 2));
    CHECK(src.hasEdge(2, 3));

    UndirectedGraph<int> other(10);
    other = src;
    CHECK(other.vertexCount() == 4);
    CHECK(other.edgeCount() == 2);

    UndirectedGraph<int> &alias = src;
    src = alias;
    CHECK(src.edgeCount() == 2 && src.hasEdge(0, 1));
  }

  // Move construction steals; source reusable.
  {
    UndirectedGraph<int> src(5);
    src.addEdge(0, 1);
    UndirectedGraph<int> dst(std::move(src));
    CHECK(dst.vertexCount() == 5);
    CHECK(dst.edgeCount() == 1);
    CHECK(src.vertexCount() == 0);
    CHECK(src.empty());

    CHECK(!src.addEdge(0, 1)); // 0 vertices: nothing addable
    UndirectedGraph<int> rejuvenate(3);
    src = rejuvenate; // reusable via assignment
    CHECK(src.addEdge(1, 2)); // now works
  }

  // Self-move is a no-op.
  {
    UndirectedGraph<int> m(3);
    m.addEdge(0, 1);
    UndirectedGraph<int> &alias = m;
    m = std::move(alias);
    CHECK(m.vertexCount() == 3);
    CHECK(m.hasEdge(0, 1));
  }

  // swap exchanges (member and free).
  {
    UndirectedGraph<int> s1(3), s2(5);
    s1.addEdge(0, 1);
    s2.addEdge(4, 2);
    s1.swap(s2);
    CHECK(s1.vertexCount() == 5 && s2.vertexCount() == 3);
    CHECK(s1.hasEdge(4, 2) && s2.hasEdge(0, 1));

    swap(s1, s2);
    CHECK(s1.vertexCount() == 3 && s2.vertexCount() == 5);
  }

  // operator<< prints ascending adjacency lines.
  {
    UndirectedGraph<int> g(3);
    g.addEdge(2, 0);
    g.addEdge(1, 2);
    ostringstream oss;
    oss << g;
    CHECK(oss.str().find("0: 2") != string::npos);
    CHECK(oss.str().find("1: 2") != string::npos);
    CHECK(oss.str().find("2: 0 1") != string::npos);

    UndirectedGraph<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Weighted path check via neighbor weights (a mini Dijkstra sanity).
  {
    // a weighted triangle: shortest path 0->2 is the direct edge
    UndirectedGraph<double> g(3);
    g.addEdge(0, 1, 10.0);
    g.addEdge(1, 2, 20.0);
    g.addEdge(0, 2, 5.0);

    Vec<double> w = g.neighborWeights(0);
    CHECK(w.len() == 2);
    // neighbors of 0 ascending: 1, 2 with weights 10, 5
    CHECK(w[0] == 10.0 && w[1] == 5.0);
    CHECK(g.weight(0, 2) == 5.0);
  }

  // Randomized stress: build random graphs, cross-verify connectivity,
  // component count, and edge counts against a brute-force model.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      const u32 V = 1 + rnd() % 10;
      UndirectedGraph<int> g(V);
      bool adj[10][10];
      for (u32 i = 0; i < 10; i++)
        for (u32 j = 0; j < 10; j++)
          adj[i][j] = false;
      u64 edges = 0;

      for (int op = 0; op < 40; op++) {
        u32 u = rnd() % V, v = rnd() % V;
        if (u != v && !adj[u][v] && g.addEdge(u, v)) {
          adj[u][v] = adj[v][u] = true;
          edges++;
        }
      }
      if (g.edgeCount() != edges)
        allOk = false;

      // model components via BFS on the boolean matrix
      bool seen[10];
      for (u32 i = 0; i < 10; i++)
        seen[i] = false;
      u32 comps = 0;
      for (u32 s = 0; s < V; s++) {
        if (seen[s])
          continue;
        comps++;
        u32 q[10];
        u64 head = 0, tail = 0;
        q[tail++] = s;
        seen[s] = true;
        while (head < tail) {
          u32 u = q[head++];
          for (u32 v = 0; v < V; v++) {
            if (adj[u][v] && !seen[v]) {
              seen[v] = true;
              q[tail++] = v;
            }
          }
        }
      }
      if (comps != g.connectedComponents())
        allOk = false;

      // cycle detection model: a forest iff comps + edges == V
      bool cycle = (comps + int(edges) != int(V));
      if (cycle != g.cycleExists())
        allOk = false;
    }
    CHECK(allOk);
  }

  // Dijkstra: classic weighted diamond; direct vs through paths.
  {
    UndirectedGraph<double> g(4);
    g.addEdge(0, 1, 1.0);
    g.addEdge(1, 2, 2.0);
    g.addEdge(0, 2, 10.0); // expensive direct edge
    g.addEdge(2, 3, 1.0);

    Vec<double> d = g.shortestPaths(0);
    CHECK(near(d[0], 0.0));
    CHECK(near(d[1], 1.0));
    CHECK(near(d[2], 3.0)); // 0-1-2 beats the 10 direct edge
    CHECK(near(d[3], 4.0));

    // path tree: parents form the shortest path
    Vec<u32> p = g.shortestPathTree(0);
    CHECK(p[2] == 1); // came through 1
    CHECK(p[1] == 0);
    CHECK(p[3] == 2);
    CHECK(p[0] == 0); // source's own parent

    // unreachable vertex
    UndirectedGraph<double> g2(3);
    g2.addEdge(0, 1, 1.0);
    Vec<double> d2 = g2.shortestPaths(0);
    CHECK(d2[2] == UndirectedGraph<double>::maxW()); // sentinel

    // zero-weight edges are legal
    UndirectedGraph<double> g3(2);
    g3.addEdge(0, 1, 0.0);
    CHECK(near(g3.shortestPaths(0)[1], 0.0));
  }

  // Dijkstra on the classic textbook graph (CLRS Figure 24.6 clone).
  {
    // s(0) t(1) x(2) y(3) z(4)
    UndirectedGraph<int> g(5);
    g.addEdge(0, 1, 10);
    g.addEdge(0, 3, 5);
    g.addEdge(1, 2, 1);
    g.addEdge(1, 3, 2);
    g.addEdge(2, 4, 4);
    g.addEdge(3, 1, 3);
    g.addEdge(3, 2, 9);
    g.addEdge(3, 4, 2);
    g.addEdge(4, 2, 6);
    g.addEdge(4, 0, 7);

    Vec<int> d = g.shortestPaths(0);
    // undirected distances (the figure is directed; reverse edges
    // short-circuit some routes)
    CHECK(d[0] == 0);
    CHECK(d[1] == 7);  // s-y-t (5+2 beats the 10 direct)
    CHECK(d[2] == 8);  // s-y-t-x (5+2+1 beats the 9 direct)
    CHECK(d[3] == 5);  // s-y
    CHECK(d[4] == 7);  // s-y-z
  }

  // Randomized stress: Dijkstra vs Bellman-Ford-style brute relaxer.
  {
    bool allOk = true;
    for (int trial = 0; trial < 40; trial++) {
      const u32 V = 1 + rnd() % 8;
      UndirectedGraph<int> g(V);
      int W[8][8];
      for (u32 i = 0; i < 8; i++)
        for (u32 j = 0; j < 8; j++)
          W[i][j] = 0;

      for (int op = 0; op < 20; op++) {
        u32 u = rnd() % V, v = rnd() % V;
        if (u == v)
          continue;
        int w = 1 + int(rnd() % 20);
        if (g.addEdge(u, v, w)) {
          W[u][v] = W[v][u] = w;
        }
      }

      // model: Bellman-Ford from 0
      const int INF = 1 << 30;
      int dist[8];
      for (u32 i = 0; i < 8; i++)
        dist[i] = INF;
      dist[0] = 0;
      for (u32 it2 = 0; it2 < V; it2++)
        for (u32 u = 0; u < V; u++)
          for (u32 v = 0; v < V; v++)
            if (W[u][v] && dist[u] < INF && dist[u] + W[u][v] < dist[v])
              dist[v] = dist[u] + W[u][v];

      Vec<int> mine = g.shortestPaths(0);
      for (u32 v = 0; v < V; v++) {
        if (dist[v] >= INF) {
          if (mine[v] != UndirectedGraph<int>::maxW())
            allOk = false;
        } else if (mine[v] != dist[v]) {
          allOk = false;
        }
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
