#ifndef __PAIR_H__
#define __PAIR_H__

template<typename S, typename T>
class Pair
{
  private:
    S _first;
    T _second;

  public:
    Pair& make_pair(const S &f, const T &s) {
      _first = f;
      _second = s;
    }

    S &first() const { return _first; }

    T &second() const { return _second; }

    bool operator==(const Pair &p) const {
        return (this->_first == p._first && this->_second == p._second);
    }

    bool operator!=(const Pair &p) const {
        return (this->_first != p._first || this->_second != p._second);
    }
};

#endif // __PAIR_H__
