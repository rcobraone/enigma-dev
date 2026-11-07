/** Copyright (C) 2011 Josh Ventura
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "var4.h"
#include "dynamic_args.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace std;
using namespace enigma;

#define v(argv) (*static_cast<vector<variant>*>(argv))

namespace {

bool dynamic_variant_less(const variant& lhs, const variant& rhs) {
  // Establish a deterministic ordering between variant types.
  if (lhs.type != rhs.type) {
    return lhs.type < rhs.type;
  }

  switch (lhs.type) {
    case variant::ty_string:
      return lhs.sval() < rhs.sval();

    case enigma_user::ty_pointer:
      return std::less<const void*>()(lhs.rval.p, rhs.rval.p);

    case variant::ty_real: {
      const double a = lhs.rval.d;
      const double b = rhs.rval.d;
      const bool a_nan = std::isnan(a);
      const bool b_nan = std::isnan(b);

      if (a_nan != b_nan) return !a_nan;
      if (a_nan) return false;
      return a < b;
    }

    default:
      return false;
  }
}
	
int checked_argc(const vector<variant>& arguments) {

  if (arguments.size() >
      static_cast<size_t>(std::numeric_limits<int>::max())) {
    throw std::length_error("ENIGMA: too many dynamic arguments");
  }
  return static_cast<int>(arguments.size());
 }
}

varargs& varargs::operator,(variant x) {
  auto& arguments = v(argv);

  if (arguments.size() >=
      static_cast<size_t>(std::numeric_limits<int>::max())) {
    throw std::length_error("ENIGMA: too many dynamic arguments");
  }

  arguments.push_back(x);
  argc = static_cast<int>(arguments.size());
  return *this;
}
void varargs::reverse() { std::reverse(v(argv).begin(),v(argv).end()); }
void varargs::sort() {
  auto& arguments = v(argv);
  std::sort(arguments.begin(), arguments.end(), dynamic_variant_less);
}
variant varargs::get(int i) const {
  if (i < 0)
    return variant();
const auto& arguments = v(argv);
const auto index = static_cast<size_t>(i);

if (index >= arguments.size()) return variant();

return arguments[index];
}
varargs::varargs(): argc(0), argv(new vector<variant>) {}
varargs::varargs(variant x): argc(1), argv(new vector<variant>{x}) {}
varargs::varargs(const varargs& other):
    argc(checked_argc(v(other.argv))),
    argv(new vector<variant>(v(other.argv))) {}

varargs::varargs(varargs&& other):
    argc(0),
    argv(new vector<variant>) {
  std::swap(argc, other.argc);
  std::swap(argv, other.argv);
}

varargs& varargs::operator=(const varargs& other) {
  if (this == &other)
    return *this;

  vector<variant> replacement(v(other.argv));
  const int replacement_argc = checked_argc(replacement);

  v(argv).swap(replacement);
  argc = replacement_argc;
  return *this;
}

varargs& varargs::operator=(varargs&& other) {
  if (this == &other)
    return *this;

  // Allocate the replacement before changing ownership.
  vector<variant>* empty = new vector<variant>;   
  delete static_cast<vector<variant>*>(argv);
  argv = other.argv;
  argc = other.argc;
  other.argc = 0;
  other.argv = empty;
  return *this;
}

varargs::~varargs() { delete static_cast<vector<variant>*>(argv); }

