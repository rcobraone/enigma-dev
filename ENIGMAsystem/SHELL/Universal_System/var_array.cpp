/********************************************************************************\
**                                                                              **
**  Copyright (C) 2014 Seth N. Hetu                                             **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/

#include "var_array.h"
#include "var4.h"
#include <limits>

namespace enigma_user {

var array_create(size_t size, variant value) { return var(value, size); }
var array_create_2d(size_t length, size_t height, variant value) { return var(value, length, height); }
bool array_equals(const var& arr1, const var& arr2) {
  const int length = arr1.array_len();

  if (length != arr2.array_len())
    return false;

  for (int i = 0; i < length; ++i) {
    if (arr1[i] != arr2[i])
      return false;
  }
  return true;
}
void array_copy(var& dest, size_t dest_index, const var& src, size_t src_index, size_t length) {

 if (length == 0)
    return;


 // var::operator[] accepts int, not size_t.
 // Reject indices that cannot be represented safely.
 constexpr size_t max_index =
     static_cast<size_t>(std::numeric_limits<int>::max());

 if (dest_index > max_index || src_index > max_index)
   return;

 if (length - 1 > max_index - dest_index ||
     length - 1 > max_index - src_index)
   return;

 // Self-copy must preserve the original source values.
 // Copy backwards when the destination overlaps to the right.
 if (&dest == &src) {
   if (dest_index == src_index)
     return;

   if (dest_index > src_index) {
     for (size_t i = length; i > 0; --i) {
       const size_t offset = i - 1;
       dest[static_cast<int>(dest_index + offset)] =
           src[static_cast<int>(src_index + offset)];
     }
   } else {
     for (size_t i = 0; i < length; ++i) {
       dest[static_cast<int>(dest_index + i)] =
           src[static_cast<int>(src_index + i)];
     }
   }
  }
   for (size_t i = 0; i < length; ++i) { 
   dest[static_cast<int>(dest_index + i)] =
       src[static_cast<int>(src_index + i)];
  }
}

int array_length_1d(const var& v) { return v.array_len(); }
int array_length_2d(const var& v, int n) {
  return n < 0 ? 0 : v.array_len(n);
}
int array_height_2d(const var& v) { return v.array_height(); }
void array_set(var& v, int pos, variant value) {
  if (pos < 0) return;
  v[pos] = value;
}
bool is_array(const var& v) {
 return v.array_height() > 0 || v.array_len() > 0;
}

}  // namespace enigma_user
