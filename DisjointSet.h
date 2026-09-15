#pragma once

#include <unordered_map>
#include <utility>

#include "Exception.h"

// DisjointSet<T>: union-find over elements of a hashable type T.
//
// A rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined. With a
// data member container there is no resource to release, so the destructor
// is defaulted; moves transfer the element map and leave the source empty
// and reusable.
//
// All operations throw Exception for an element that was never passed to
// make_set(): the set membership is explicit rather than silently growing.
// make_set() on an existing element is a no-op. join() of two elements
// in the same set is also a no-op (self-union neither errors nor
// decrements the set count), matching standard union-find semantics.
//
// find() compresses paths, so it is non-const. join() unions by rank;
// sizes are maintained per set for size().
template <typename T> class DisjointSet {
  private:
    class elem
    {
      public:
        T parent;
        int rank;
        int size;

        // Only the initializing constructor is used by make_set(); the
        // default is kept zero-initialized so a default elem carries no
        // garbage.
        elem() : rank(0), size(0) {}

        elem(const T &p, const int r, const int s): parent(p), rank(r), size(s)
        { ; }
    };

    std::unordered_map< T, elem > elts;
    int num;

  public:
    DisjointSet() : num(0) {}

    // Copy is a deep copy of the element map; the map owns only values.
    DisjointSet(const DisjointSet &o) = default;
    ~DisjointSet() = default;

    // Steal o's element map; o is left empty and reusable.
    DisjointSet(DisjointSet &&o) : elts(std::move(o.elts)), num(o.num)
    {
      o.num = 0;
    }

    // Deep copy: unordered_map's assignment copies every element, so a
    // failed allocation leaves *this untouched.
    DisjointSet &operator=(const DisjointSet &o) = default;

    // Steal o's element map, freeing ours automatically. Self-move is
    // guarded because container move assignment from itself is unspecified.
    DisjointSet &operator=(DisjointSet &&o)
    {
      if (this != &o)
      {
        elts = std::move(o.elts);
        num = o.num;
        o.num = 0;
      }
      return(*this);
    }

    void make_set(const T &x);
    void join(const T &x, const T &y);
    T find(const T &x);
    int size(const T &x) const;
    int numSets() const { return(num); }
};

template< typename T > void DisjointSet<T>::make_set(const T &x)
{
  // insert() does nothing (and reports it) when x is already present, so
  // a repeat make_set never overwrites an existing membership or size.
  pair<typename unordered_map<T, elem>::iterator, bool> result =
elts.insert(typename unordered_map<T, elem>::value_type(x, elem(x, 0, 1)));
  if (result.second) num++;
}

template< typename T > int DisjointSet<T>::size(const T &x) const
{
  // Size lives on the set's representative; asking for x's own slot
  // would return a stale value once x has been attached under another
  // element.
  typename unordered_map< T, elem >::const_iterator it = elts.find(x);
  if (it == elts.end())
    throw(Exception("DisjointSet::size: element not found"));

  typename unordered_map< T, elem >::const_iterator root = it;
  while (root->second.parent != root->first)
    root = elts.find(root->second.parent);

  return(root->second.size);
}

template< typename T > void DisjointSet<T>::join(const T &x, const T &y)
{
  // Union by rank operates on the ROOTS. Comparing the elements the
  // caller passed would attach non-representatives and corrupt both the
  // structure and the sizes.
  const T xroot = find(x);
  const T yroot = find(y);

  if (xroot == yroot)
    return; // already in the same set: self-union is a no-op

  typename unordered_map< T, elem >::iterator xit = elts.find(xroot);
  typename unordered_map< T, elem >::iterator yit = elts.find(yroot);

  if (xit->second.rank > yit->second.rank)
  {
    yit->second.parent = xroot;
    xit->second.size += yit->second.size;
  }
  else
  {
    xit->second.parent = yroot;
    yit->second.size += xit->second.size;

    if (xit->second.rank == yit->second.rank)
    {
      // Equal ranks: y becomes the new representative and gains a level.
      yit->second.rank++;
    }
  }
  num--;
}

template< typename T > T DisjointSet<T>::find(const T &x)
{
  typename unordered_map< T, elem >::iterator it = elts.find(x);
  if (it == elts.end())
    throw(Exception("DisjointSet::find: element not found"));

  if (it->second.parent == x)
  {
    return(it->second.parent);
  }
  else
  {
    // Path compression: re-root x at its true representative so later
    // finds get shorter chains.
    it->second.parent = find(it->second.parent);
    return(it->second.parent);
  }
}
