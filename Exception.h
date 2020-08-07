#pragma once

#include <exception>
#include <iostream>

using namespace std;

/* C++ error handeling class for passing simple strings. */
class Exception : public exception {
private:
  string mes;

public:
  Exception(const char *s) : mes(s) {}

  virtual ~Exception() throw() {}

  virtual const char *what() const throw() { return (mes.c_str()); }
};
