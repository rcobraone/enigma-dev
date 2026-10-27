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
#include <vector>
using namespace std;
using namespace enigma;

#define v(argv) (*(vector<variant>*)argv)

varargs& varargs::operator,(variant x) { v(argv).push_back(x); argc++; return *this; }
void varargs::reverse() { std::reverse(v(argv).begin(),v(argv).end()); }
void varargs::sort() { std::sort(v(argv).begin(),v(argv).end()); }
variant varargs::get(int i) const {
  if (i < 0 || static_cast<size_t>(i) >= v(argv).size())
    return variant();
  return v(argv)[static_cast<size_t>(i)];
}
varargs::varargs(): argc(0), argv(new vector<variant>) {}
varargs::varargs(variant x): argc(1), argv(new vector<variant>) { v(argv).push_back(x); }
varargs::varargs(const varargs& other):

    argc(other.argc),
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

  v(argv) = v(other.argv);
  argc = other.argc;
  return *this;
}

varargs& varargs::operator=(varargs&& other) {
  if (this == &other)
    return *this;

  // Allocate the replacement before changing ownership.
  vector<variant>* empty = new vector<variant>;   
  delete (vector<variant>*)argv;
  argv = other.argv;
  argc = other.argc;
  other.argc = 0;
  other.argv = empty;
  return *this;
}

varargs::~varargs() { delete (vector<variant>*)argv; }

