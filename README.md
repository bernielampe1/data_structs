Summary:
    This repository is a collection of data structures authored in C++. They are largely pedagogical, minimally tested and not optimizied.

To be implemented:
  Set, Map, HashMap, Trie, Skip Lists, Sparse Matrix, N-dimensional Array, Undirected Graph, Directed Acyclic Graph  

Rule-of-five-default:
    If you define, =default or =delete any of dtor, copy ctor, assignment op, move ctor, move assignment op then you must define, =default or =delete them all.

Rule-of-zero:
    Use value semantics of stl and smart pointers to ensure you don't have to define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor.

Rule-of-five:
    If you define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor, you must to all five.

Rule-of-three:
    If you define any of the copy ctor, assignment op, or dtor, you must to all three.
