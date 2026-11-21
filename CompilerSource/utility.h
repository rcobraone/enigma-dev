#ifndef ENIGMA_COMPILER_UTILITY_H
#define ENIGMA_COMPILER_UTILITY_H

#include <string>

static inline std::string toUpper(std::string x) {
  for (char& c : x)
    if (c >= 'a' && c <= 'z')
      c += 'A' - 'a';
  return x;
}

#endif  // ENIGMA_COMPILER_UTILITY_H
