This is a pedagogical set of data structs, and some algorithms in C++. We concentrated on the rule-of-five implementations. The rule-of-zero is a better way to do this so you don't have to re-accomplish work and inevitably introduce bugs. These are used for learning and teaching.

## Rules

**Rule-of-five-default:**
If you define, `=default` or `=delete` any of dtor, copy ctor, assignment op, move ctor, move assignment op then you must define, `=default` or `=delete` them all.

**Rule-of-zero:**
Use value semantics of stl and smart pointers to ensure you don't have to define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor.

**Rule-of-five:**
If you define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor, you must to all five.

**Rule-of-three:**
If you define any of the copy ctor, assignment op, or dtor, you must to all three.

## Containers

All are rule-of-five, each with a test in `tests/`.

| Header | Contents |
|---|---|
| `Array.h` | Fixed-length contiguous array; STL random-access iterators |
| `Vec.h` / `Vec.inl` | Growable length with element-wise arithmetic |
| `Matrix.h` / `Matrix.inl` | Dense 2-D; matmul, LUP decompose, det(2), inverse(2), solve(2) |
| `SparseMatrix.h` | Sorted (row, col, value) triples; Matrix.h feature parity, zero-normalizing arithmetic, `toDense`/`fromDense` bridges |
| `ArrayN.h` | N-dimensional dense array over row-major strides |
| `BitVector.h` | Packed bit sequence with bit tricks (population count) |
| `List.h` | Doubly-linked ring around a sentinel; bidirectional iterators |
| `Stack.h` | LIFO singly-linked chain; forward iterators |
| `Queue.h` | FIFO doubly-linked chain; forward iterators |
| `Deque.h` | Double-ended queue over a circular buffer; O(1) amortized both ends; the Stack/Queue generalization |
| `Pair.h` | Two-element value type |
| `Set.h` | Ordered set over a doubling sorted array (binary search) |
| `Map.h` | Ordered map from K to V over the same sorted array |
| `HashMap.h` | std::hash keyed map, open addressing with tombstones |
| `HashTable.h` | Client-hash keyed map, separate chaining |
| `RBTree.h` | Red-black tree (std::set contract: insert/erase/exists/find, `checkInvariants()` test hook) |
| `BinTree.h` | Unbalanced BST (insert/exists/find) |
| `BinHeap.h` | Fixed-capacity binary heap (MAXHEAP by default) |
| `PriorityQueue.h` | Priority-queue ADT over BinHeap: push/popTop/changePriority |
| `SkipList.h` | Probabilistic ordered map with tower levels |
| `Trie.h` | String-keyed prefix tree; `hasPrefix`/`prefix_each` |
| `DisjointSet.h` | Union-find with rank + path compression |
| `Image.h` / `Image.inl` | 2-D pixel buffer; PGM/PPM file I/O; convolution |
| `UndirectedGraph.h` | Weighted undirected simple graph; BFS, DFS, components, cycle detect, bipartite, Dijkstra `shortestPaths` |
| `DAG.h` | Directed acyclic graph with enforced acyclicity; topological order, reachability, transitive closure, weighted longest path |
| `DirectedGraph.h` | General digraph (cycles allowed); `hasCycle` (3-color DFS), Tarjan strongly connected components, topological order (throws when cyclic), transitive closure |
| `MST.h` | `kruskalMST` / `primMST` / `primForestMST`: minimum spanning tree over UndirectedGraph by union-find growth or seeded growth |

## Algorithms

Each is cross-checked against `std::sort` / reference models.

| Header | Contents |
|---|---|
| `qsort.h` | Quicksort over `Array<T>` (l..r range) |
| `msort.h` | Merge sort over `Array<T>` (stable) |

## Build and run every test

```
cd tests && make check
```

Individual test:

```
cd tests && make <name>Test && ./<name>Test
```

Every test prints one line per check (`ok: ...` / `FAIL: ...`) and exits nonzero on the first failure.
