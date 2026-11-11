/** Copyright (C) 2008-2013 Josh Ventura
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

#include "strings_util.h"

#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <vector>
#include "var4.h"
#include "estring.h"

#ifdef DEBUG_MODE
#include "libEGMstd.h"
#include "Widget_Systems/widgets_mandatory.h"
#endif

using std::string;

#include "../../../CompilerSource/OS_Switchboard.h"

#if CURRENT_PLATFORM_ID == OS_WINDOWS

#define byte __windows_byte_workaround
#include <windows.h>
#undef byte

using std::vector;

tstring widen(const string &str) {
  if (str.empty())
    return L"";

  if (str.size() > static_cast<size_t>(INT_MAX))
    return L"";

  const int input_size = static_cast<int>(str.size());

  const int required = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS,
      str.data(), input_size, nullptr, 0);

  if (required <= 0)
    return L"";

  vector<wchar_t> buf(static_cast<size_t>(required));

  const int converted = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS,
      str.data(), input_size, buf.data(), required);

  if (converted != required)
    return L"";

  return tstring(buf.data(), static_cast<size_t>(converted));
}

string shorten(tstring str) {
  if (str.empty())
    return "";

  if (str.size() > static_cast<size_t>(INT_MAX))
    return "";

  const int input_size = static_cast<int>(str.size());

  const int required = WideCharToMultiByte(
      CP_UTF8, WC_ERR_INVALID_CHARS,
      str.data(), input_size,
      nullptr, 0, nullptr, nullptr);

  if (required <= 0)
    return "";

  vector<char> buf(static_cast<size_t>(required));

  const int converted = WideCharToMultiByte(
      CP_UTF8, WC_ERR_INVALID_CHARS,
      str.data(), input_size,
      buf.data(), required, nullptr, nullptr);

  if (converted != required)
    return "";

  return string(buf.data(), static_cast<size_t>(converted));
}

#endif

static const char ldgrs[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  4,4,4,4,4,4,4,4,4,4,0,0,0,0,0,0,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0
};

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

namespace enigma_user {

bool is_base64(unsigned char c) {
  return (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') ||
         c == '+' ||
         c == '/';
}

string base64_encode(string const& str) {
  string ret;
  const size_t groups = str.size() / 3;
  const size_t remainder = str.size() % 3;

  if (groups > (ret.max_size() - (remainder ? 4 : 0)) / 4)
    return "";

  ret.reserve(groups * 4 + (remainder ? 4 : 0));

  size_t i = 0;
  while (i + 3 <= str.size()) {
    const unsigned char a = static_cast<unsigned char>(str[i]);
    const unsigned char b = static_cast<unsigned char>(str[i + 1]);
    const unsigned char c = static_cast<unsigned char>(str[i + 2]);

    ret += base64_chars[a >> 2];
    ret += base64_chars[((a & 0x03) << 4) | (b >> 4)];
    ret += base64_chars[((b & 0x0f) << 2) | (c >> 6)];
    ret += base64_chars[c & 0x3f];
    i += 3;
  }

  if (remainder == 1) {
    const unsigned char a = static_cast<unsigned char>(str[i]);
    ret += base64_chars[a >> 2];
    ret += base64_chars[(a & 0x03) << 4];
    ret += "==";
  } else if (remainder == 2) {
    const unsigned char a = static_cast<unsigned char>(str[i]);
    const unsigned char b = static_cast<unsigned char>(str[i + 1]);
    ret += base64_chars[a >> 2];
    ret += base64_chars[((a & 0x03) << 4) | (b >> 4)];
    ret += base64_chars[(b & 0x0f) << 2];
    ret += '=';
  }
  return ret;
}

string base64_decode(string const& str) {
  string ret;
  if (str.empty())
    return ret;

  if (str.size() % 4 != 0)
    return "";

  const size_t groups = str.size() / 4;
  if (groups > ret.max_size() / 3)
    return "";

  ret.reserve(groups * 3);

  for (size_t i = 0; i < str.size(); i += 4) {
    const bool last = (i + 4 == str.size());
    const bool pad2 = str[i + 2] == '=';
    const bool pad1 = str[i + 3] == '=';

    if (str[i] == '=' || str[i + 1] == '=')
      return "";

    if ((pad2 && !pad1) || ((pad1 || pad2) && !last))
      return "";

    const size_t a = base64_chars.find(str[i]);
    const size_t b = base64_chars.find(str[i + 1]);
    const size_t c = pad2 ? 0 : base64_chars.find(str[i + 2]);
    const size_t d = pad1 ? 0 : base64_chars.find(str[i + 3]);

    if (a == string::npos || b == string::npos ||
        (!pad2 && c == string::npos) ||
        (!pad1 && d == string::npos))
      return "";

    if ((pad2 && (b & 0x0f) != 0) ||
        (pad1 && !pad2 && (c & 0x03) != 0))
      return "";

    const unsigned int value =
        (static_cast<unsigned int>(a) << 18) |
        (static_cast<unsigned int>(b) << 12) |
        (static_cast<unsigned int>(c) << 6) |
        static_cast<unsigned int>(d);

    ret += static_cast<char>((value >> 16) & 0xff);

    if (!pad2)
      ret += static_cast<char>((value >> 8) & 0xff);

    if (!pad1)
      ret += static_cast<char>(value & 0xff);
  }
  return ret;
}

double real(variant str) { return str.type ? atof(((string)str).c_str()) : (double) str; }

string ansi_char(char byte) { return string(1,byte); }
string chr(char val) { return string(1,val); }
int ord(string str) {
  return str.empty() ? 0 : static_cast<unsigned char>(str[0]);
}

size_t string_length(string str) { return str.length(); }
size_t string_length(const char* str) {
  return str ? strlen(str) : 0;
}

static bool count_utf8_codepoints(
    const char* data,
    size_t length,
    size_t& result
) {
  result = 0;
  size_t i = 0;

  while (i < length) {
    const unsigned char c =
        static_cast<unsigned char>(data[i]);

    if (c <= 0x7F) {
      ++i;
    } else if (c >= 0xC2 && c <= 0xDF) {
      if (length - i < 2)
        return false;

      const unsigned char c1 =
          static_cast<unsigned char>(data[i + 1]);

      if (c1 < 0x80 || c1 > 0xBF)
        return false;

      i += 2;
    } else if (c >= 0xE0 && c <= 0xEF) {
      if (length - i < 3)
        return false;

      const unsigned char c1 =
          static_cast<unsigned char>(data[i + 1]);
      const unsigned char c2 =
          static_cast<unsigned char>(data[i + 2]);

      if (c2 < 0x80 || c2 > 0xBF)
        return false;

      if (c == 0xE0) {
        if (c1 < 0xA0 || c1 > 0xBF)
          return false;
      } else if (c == 0xED) {
        if (c1 < 0x80 || c1 > 0x9F)
          return false;
      } else if (c1 < 0x80 || c1 > 0xBF) {
        return false;
      }

      i += 3;
    } else if (c >= 0xF0 && c <= 0xF4) {
      if (length - i < 4)
        return false;

      const unsigned char c1 =
          static_cast<unsigned char>(data[i + 1]);
      const unsigned char c2 =
          static_cast<unsigned char>(data[i + 2]);
      const unsigned char c3 =
          static_cast<unsigned char>(data[i + 3]);

      if (c2 < 0x80 || c2 > 0xBF ||
          c3 < 0x80 || c3 > 0xBF)
        return false;

      if (c == 0xF0) {
        if (c1 < 0x90 || c1 > 0xBF)
          return false;
      } else if (c == 0xF4) {
        if (c1 < 0x80 || c1 > 0x8F)
          return false;
      } else if (c1 < 0x80 || c1 > 0xBF) {
        return false;
      }

      i += 4;
    } else {
      return false;
    }

    ++result;
  }

  return true;
}

size_t string_length_utf8(string str) {
    
  size_t result = 0;
  if (!count_utf8_codepoints(str.data(), str.size(), result))
    return 0;
  return result;
}

size_t string_length_utf8(const char* str) {
	
  if (str == nullptr)
    return 0;

  size_t result = 0;
  if (!count_utf8_codepoints(str, strlen(str), result))
    return 0;
  return result;
}

size_t string_pos(string substr,string str) {
  const size_t pos = str.find(substr);
  if (pos == string::npos)
   return 0;
  return pos + 1;
}

string string_format(double val, unsigned tot, unsigned dec) {
  // Prevent excessive allocations caused by untrusted formatting arguments.
  constexpr size_t max_output_size = 1024 * 1024;

  if (tot > max_output_size || dec > max_output_size)
    return "";

  const int width = static_cast<int>(tot);
  const int precision = static_cast<int>(dec);

  const int needed = std::snprintf(
      nullptr, 0, "%0*.*f", width, precision, val);

  if (needed < 0)
    return "";

  const size_t output_size = static_cast<size_t>(needed);
  if (output_size > max_output_size)
    return "";
  std::vector<char> buffer(output_size + 1);
  const int written = std::snprintf(
      buffer.data(), buffer.size(), "%0*.*f", width, precision, val);

  if (written < 0 || written != needed)
    return "";

  return string(buffer.data(), output_size);
}

string string_copy(string str, int index, int count) {
  index = index < 0 ? 0 : index;
  return (size_t)index > str.length()? "": str.substr(index < 2? 0: index-1, count < 1? 0: count);
}

string string_set_byte_at(string str, int index, char byte) {
  if (index < 1)
    return str;

  const size_t pos = static_cast<size_t>(index - 1);

  if (pos >= str.size())
    return str;

  str[pos] = byte;
  return str;
}

char string_byte_at(string str, int index) {
if (index < 1)
    return 0;
  const size_t pos = static_cast<size_t>(index - 1);

  if (pos >= str.size())
    return 0;

  return str[pos];
}

string string_char_at(string str,int index) {
  unsigned int n = index <= 1 ? 0 : (unsigned int)(index - 1);
   if (n >= str.length())
    return "";
  return string(1, str[n]);
}

string string_delete(string str,int index,int count) {
  if (index < 1 || count < 1)
    return str;

  const size_t pos = static_cast<size_t>(index - 1);

  if (pos >= str.size())
    return str;

  const size_t length = static_cast<size_t>(count);

  str.erase(pos, length);
  return str;
}

string string_insert(string substr,string str,int index) {
  if (index <= 1)
    return substr + str;

  const size_t pos = static_cast<size_t>(index - 1);

  if (pos > str.size())
    return str + substr;

  str.insert(pos, substr);
  return str;
}

string string_replace(string str,string substr,string newstr) {
  if (substr.empty())
    return str;

  const size_t pos = str.find(substr);

  if (pos == string::npos)
    return str;

  str.replace(pos, substr.size(), newstr);
  return str;
}

string string_replace_all(string str,string substr,string newstr) {
  if (substr.empty())
    return str;

  return ::string_replace_all(str, substr, newstr);
}

size_t string_count(string substr,string str) {
  if (substr.empty())
   return 0;
  size_t pos = 0, occ = 0;
  const size_t sublen = substr.length();
  while((pos=str.find(substr,pos)) != string::npos)
    occ++, pos += sublen;
  return occ;
}

string string_lower(string str) {
  const size_t len = str.length();
  for (size_t i = 0; i < len; ++i)
    if (ldgrs[(int)(unsigned char)str[i]] & 2)
      str[i] += 32;
  return str;
}

string string_upper(string str) {
  const size_t len = str.length();
  for (size_t i = 0; i < len; ++i)
    if (ldgrs[(unsigned char)str[i]] & 1)
      str[i] -= 32;
  return str;
}

string string_repeat(string str,int count) {
  if (count <= 0 || str.empty())
   return "";
  string ret;
  const size_t repetitions = static_cast<size_t>(count);
  
  if (str.length() > ret.max_size() / repetitions)
    return "";

  const size_t total_size = str.length() * repetitions;
  ret.reserve(total_size);

  for (size_t i = 0; i < repetitions; ++i)
    ret.append(str);

  return ret;
}

string string_letters(string str) {
  string ret;
  for(const char*c=str.c_str();*c;c++)
    if(ldgrs[(unsigned char)*c]&3) ret+=*c;
  return ret;
}

string string_digits(string str) {
  string ret;
  for(const char*c=str.c_str();*c;c++)
    if(ldgrs[(unsigned char)*c]&4) ret += *c;
  return ret;
}

string string_lettersdigits(string str) {
  string ret;
  for(const char*c=str.c_str();*c;c++)
    if(ldgrs[(unsigned char)*c]) ret += *c;
  return ret;
}

bool string_isletters(string str) {
  for(const char*c = str.c_str(); *c; c++)
    if(!(ldgrs[(unsigned char)*c] & 3))
      return false;
  return true;
}

bool string_isdigits(string str) {
  for(const char*c = str.c_str(); *c; c++)
    if(!(ldgrs[(unsigned char)*c] & 4))
      return false;
  return true;
}

bool string_islettersdigits(string str) {
  for(const char*c=str.c_str(); *c; c++)
    if(!ldgrs[(unsigned char)*c])
      return false;
  return true;
}

//filename fucntions place here as they are just string based

string filename_name(string fname)
{
  size_t fp = fname.find_last_of("/\\");
  return fname.substr(fp+1);
}

string filename_path(string fname)
{
  size_t fp = fname.find_last_of("/\\");
  return fname.substr(0,fp+1);
}

string filename_dir(string fname)
{
  if (fname.size() < 2)
    return "";

  const size_t fp = fname.find_last_of("/\\");
  if (fp == string::npos)
    return "";
   
  return fname.substr(0, fp);
}

string filename_drive(string fname)
{
  size_t fp = fname.find_first_of("/\\");
  if (!fp || fp == string::npos || fname[fp-1] != ':')
    return "";

  const unsigned char drive =
      static_cast<unsigned char>(fname[0]);

  if (!((drive >= 'A' && drive <= 'Z') ||
        (drive >= 'a' && drive <= 'z')) ||
      fname[1] != ':')
    return "";

  return fname.substr(0, 2);
}

string filename_ext(string fname)
{
  fname = filename_name(fname);
  size_t fp = fname.find_last_of(".");
  if (fp == string::npos)
    return "";
  return fname.substr(fp);
}

string filename_change_ext(string fname, string newext)
{
  const size_t separator = fname.find_last_of("/\\");
  const size_t dot = fname.find_last_of('.');

  // A dot in a directory name is not a file extension.
  if (dot == string::npos ||
      (separator != string::npos && dot < separator))
    return fname + newext;

  return fname.replace(dot, fname.length() - dot, newext);
}

var string_split(const std::string &str, const std::string &delim,
                 bool skip_empty) {
  var res;
  if (delim.empty()) {
    res[0] = str;
    return res;
  }
  size_t last = 0, next, found = 0;
  while ((next = str.find(delim, last)) != std::string::npos) {
    if (!skip_empty || next > last)
      res[found++] = str.substr(last, next - last);
    last = next + delim.length();
  }
  if (!skip_empty || last < str.length())
    res[found++] = str.substr(last);
  return res;
}

}
