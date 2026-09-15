#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <math.h>
#include <string.h>
#include <vector>

#include "Exception.h"

// Histogram<T>: bin counts over a data sample, with the number of bins
// estimated by the Freedman-Diaconis rule.
//
// A rule-of-five implementation (see README.md): the destructor, copy
// and move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// set(v, n) computes min/max and the FD bin count, allocates the bins,
// and tallies. Calling set() again re-initializes (the earlier bins are
// released first; the old version leaked them). n <= 0 throws Exception.
// Degenerate samples (identical values, or zero interquartile range)
// fall back to unit-width bins sized to the value range.
//
// operator[] is bounds-checked and throws Exception on a bad bin; access
// patterns mimic the read-only count view. Data T must be copyable into
// a std::vector and support operator< (std::sort).
template<typename T> class Histogram
{
  private:
    int *_hist;          // per-bin counts; null when no sample is set
    int _numBins;        // number of bins (== 0 until set())
    double _binWidth;    // width of one bin
    double _minVal, _maxVal; // sample range

  public:
    Histogram() : _hist(0), _numBins(0), _binWidth(0), _minVal(0), _maxVal(0) {}

    // Deep copy: same bin layout, same counts.
    Histogram(const Histogram &o)
        : _hist(o._hist ? new int[o._numBins] : 0), _numBins(o._numBins),
          _binWidth(o._binWidth), _minVal(o._minVal), _maxVal(o._maxVal)
    {
      for (int i = 0; i < _numBins; ++i)
        _hist[i] = o._hist[i];
    }

    // Steal o's bin array; o left empty and reusable.
    Histogram(Histogram &&o) noexcept
        : _hist(o._hist), _numBins(o._numBins), _binWidth(o._binWidth),
          _minVal(o._minVal), _maxVal(o._maxVal)
    {
      o._hist = 0;
      o._numBins = 0;
      o._binWidth = 0;
      o._minVal = 0;
      o._maxVal = 0;
    }

    ~Histogram() { delete[] _hist; }

    // Deep copy: allocated and filled before the old array is released,
    // so a failed allocation leaves *this untouched.
    Histogram &operator=(const Histogram &o)
    {
      if (this == &o)
        return(*this);

      int *newHist = o._hist ? new int[o._numBins] : 0;
      for (int i = 0; i < o._numBins; ++i)
        newHist[i] = o._hist[i];

      delete[] _hist;
      _hist = newHist;
      _numBins = o._numBins;
      _binWidth = o._binWidth;
      _minVal = o._minVal;
      _maxVal = o._maxVal;

      return(*this);
    }

    // Steal o's bin array (freeing ours, unconditionally).
    Histogram &operator=(Histogram &&o) noexcept
    {
      if (this != &o)
      {
        delete[] _hist;
        _hist = o._hist;
        _numBins = o._numBins;
        _binWidth = o._binWidth;
        _minVal = o._minVal;
        _maxVal = o._maxVal;
        o._hist = 0;
        o._numBins = 0;
        o._binWidth = 0;
        o._minVal = 0;
        o._maxVal = 0;
      }
      return(*this);
    }

    // Constant-time exchange of both bin arrays.
    void swap(Histogram &o)
    {
      std::swap(_hist, o._hist);
      std::swap(_numBins, o._numBins);
      std::swap(_binWidth, o._binWidth);
      std::swap(_minVal, o._minVal);
      std::swap(_maxVal, o._maxVal);
    }

    // Releases the bins; the histogram becomes empty and reusable.
    void clear()
    {
      delete[] _hist;
      _hist = 0;
      _numBins = 0;
      _binWidth = 0;
      _minVal = 0;
      _maxVal = 0;
    }

    // accessors
    int getNumBins() const { return(_numBins); }

    double getMin() const { return(_minVal); }

    double getMax() const { return(_maxVal); }

    double getBinWidth() const { return(_binWidth); }

    // count in bin i; throws Exception when i is not a bin index.
    int operator[](const int i) const
    {
      if (i < 0 || i >= _numBins || _hist == 0)
        throw(Exception("Histogram: bin index out of range"));
      return(_hist[i]);
    }

    // used freedman-diaconis rule for estimating optimal bin width
    void set(const T *v, const int n)
    {
      if (n <= 0)
        throw(Exception("Histogram::set: sample size must be positive"));
      if (v == 0)
        throw(Exception("Histogram::set: null sample"));

      // release any previous bins before recomputing (the old version
      // leaked them on a second set()).
      delete[] _hist;
      _hist = 0;

      // copy values into vector and sort
      vector<T> v_t;
      v_t.assign(v, v + n);
      sort(v_t.begin(), v_t.end());
      _minVal = v_t[0];
      _maxVal = *v_t.rbegin();

      // get the quartiles, with the indexes clamped to the sample: the
      // old code read v_t[3*qw-1] unclamped, out of bounds for samples
      // smaller than four.
      int qw = int(n / 4.0 + 0.5);
      if (qw < 1)
        qw = 1;
      T q1 = v_t[qw - 1];
      T q3 = v_t[3 * qw - 1 < n ? 3 * qw - 1 : n - 1];

      double n13 = pow(double(n), 1.0 / 3.0);
      double r = _maxVal - _minVal;
      double d = 2.0 * (q3 - q1);

      if (d > 0 && r > 0)
      {
        _numBins = int(ceil(n13 * r / d));
        _binWidth = r / _numBins;
      }
      else
      {
        // degenerate sample: near-unit-width bins across the range
        _numBins = int(r) + 1;
        _binWidth = 1.0;
      }

      if (_numBins < 1)
        _numBins = 1; // belt and braces: ceil of a positive is >= 1

      // allocate (value-initialized: zeroed, replacing the old
      // memset(hist, 0, numBins) which zeroed numBins BYTES, leaving
      // most bins holding garbage)
      _hist = new int[_numBins]();

      // assign; round-to-nearest bin with a two-sided clamp (the value
      // at _maxVal must land in the last bin, not one past it)
      for (int i = 0; i < n; ++i)
      {
        int b = int((v[i] - _minVal) / _binWidth);
        if (b >= _numBins)
          b = _numBins - 1;
        if (b < 0)
          b = 0;
        _hist[b]++;
      }
    }

    ostream& print(ostream &os) const
    {
      os << "* Histogram:\n* Num Bins:" << _numBins << "\n";
      for(int i = 0; i < _numBins; ++i)
      {
        double v = _minVal + i * _binWidth;
        os << " " << i << " " << v << " " << _hist[i] << endl;
      }

      return(os);
    }
};

template< typename T >
ostream& operator<<(ostream &os, const Histogram<T> &h) { return(h.print(os)); }
