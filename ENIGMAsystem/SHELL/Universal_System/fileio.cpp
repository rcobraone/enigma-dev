/** Copyright (C) 2008-2011 Josh Ventura
*** Copyright (C) 2014 Robert B. Colton
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

// Simple, intuitive, integer based file I/O

#include "Platforms/General/fileio.h"
#include "Resources/AssetArray.h"
#include "Widget_Systems/widgets_mandatory.h"

#include <fstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <limits>

#ifdef DEBUG_MODE
#define try_io_and_print(f) print_and_clear_fs_status(__FUNCTION__, f);
#else
#define try_io_and_print(f)
#endif

namespace enigma {
	
  static inline bool valid_file_handle(int fileid) {
    return files.exists(fileid);
  }
  
  struct file {
    file() {}
    file(const std::string& fName, std::ios_base::openmode mode) : fn(fName) { fs.open(fName, mode); }
    file(file&& other) : fn(other.fn) { fs.swap(other.fs); }
    std::string fn;
    std::fstream fs;
    // AssArray mandatory
    static const char* getAssetTypeName() { return "FileHandle"; }
    bool isDestroyed() const { return false; }
    void destroy() { fs.close(); }
  };
  
  AssetArray<file> files;
  
  static void print_and_clear_fs_status(const std::string& operation, file& f) {
    if (!f.fs.good()) {
      DEBUG_MESSAGE("Operation: " + operation + " failed on: " + f.fn + " Failbit: " + std::to_string(f.fs.fail())
        + " Badbit: " +  std::to_string(f.fs.bad()) + " EOF: " + std::to_string(f.fs.eof()), MESSAGE_TYPE::M_USER_ERROR);
    }
    bool eof = f.fs.eof();
    f.fs.clear();
    if (eof) f.fs.setstate(std::ios::eofbit);
  }
  
  static inline int file_open(const std::string& fname, std::ios_base::openmode mode) {
    file f(fname, mode);
    
    try_io_and_print(f)
    
    if (f.fs.is_open()) {
      files.add(std::move(f));
      return files.size()-1;
    } else return -1;
  }
} // NAMESPACE enigma


namespace enigma_user {

// Opens the file with the indicated name for reading. The function returns the id of the file that must be used in the other functions. You can open multiple files at the same time (32 max). Don't forget to close them once you are finished with them.
int file_text_open_read(const std::string& fname) {
  return enigma::file_open(fname, std::ios::in);
}

// Opens the indicated file for writing, creating it if it does not exist. The function returns the id of the file that must be used in the other functions.
int file_text_open_write(const std::string& fname) {
  return enigma::file_open(fname, std::ios::out);
}

// Opens the indicated file for appending data at the end, creating it if it does not exist. The function returns the id of the file that must be used in the other functions.
int file_text_open_append(const std::string& fname) {
  return enigma::file_open(fname, std::ios::out | std::ios::app);
}

// Closes the file with the given file id
void file_text_close(int fileid) {
  if (enigma::valid_file_handle(fileid)) {
    enigma::files.get(fileid).fs.close();
  } else DEBUG_MESSAGE("Cannot close an unopened file: " + std::to_string(fileid), MESSAGE_TYPE::M_USER_ERROR);
}

// Writes the std::string to the file with the given file id.
void file_text_write_string(int fileid, const std::string& str) {
  if (!enigma::valid_file_handle(fileid)) return;	
  enigma::files.get(fileid).fs << str;
  try_io_and_print(enigma::files.get(fileid))
}

// Write the real value to the file with the given file id.
void file_text_write_real(int fileid, double x) {
  if (!enigma::valid_file_handle(fileid)) return;
  enigma::files.get(fileid).fs << " " << x << std::setprecision(16);
  try_io_and_print(enigma::files.get(fileid))
}

// Write a newline character to the file.
void file_text_writeln(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return;
  enigma::files.get(fileid).fs << '\n';
  try_io_and_print(enigma::files.get(fileid))
}

// Write a string and newline character to the file.
void file_text_writeln(int fileid, const std::string& str) {
  file_text_write_string(fileid, str);
  file_text_writeln(fileid);
}

// Reads a string from the file with the given file id and returns this string. A string ends at the end of line.
std::string file_text_read_string(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return {};
  auto& fs = enigma::files.get(fileid).fs;
  std::string line;
  while (true) {
  const auto next = fs.peek();	  
  if (next == std::char_traits<char>::eof() || next == '\n') {
    break;
  }
    line.push_back(static_cast<char>(fs.get()));
  }
  return line;
}

std::string file_text_read_all(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return {};
  std::string all, line;
  bool first_line = true;
  while(std::getline(enigma::files.get(fileid).fs, line)) {
    try_io_and_print(enigma::files.get(fileid))
  if (!first_line) {
    all += '\n';
    }
    all += line;
    first_line = false;
  }
  return all;
}

bool file_text_eoln(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return true;
auto& fs = enigma::files.get(fileid).fs;
const auto next = fs.peek();

return next == '\n' ||
       next == std::char_traits<char>::eof();
}

double file_text_read_real(int fileid) { // Reads a real value from the file and returns this value.
  if (!enigma::valid_file_handle(fileid)) return 0.0;
  double x = 0;
  enigma::files.get(fileid).fs >> x;
  try_io_and_print(enigma::files.get(fileid))
  return x;
}

// Skips the rest of the line in the file and starts at the start of the next line.
std::string file_text_readln(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return {};
  std::string line;
  std::getline(enigma::files.get(fileid).fs, line);
  return line;
}

bool file_text_eof(int fileid) { // Returns whether we reached the end of the file.
  if (!enigma::valid_file_handle(fileid)) return true;
  return (enigma::files.get(fileid).fs.eof());
}

void load_info(const std::string& fname) {
	int file = file_text_open_read(fname);
	if (file < 0)
     return;
	enigma::gameInfoText = file_text_read_all(file);
	file_text_close(file);
}

// Opens the file with the indicated name. The mode indicates what can be done with the file: 0 = reading, 1 = writing, 2 = both reading and writing). When the file does not exist it is created. The function returns the id of the file that must be used in the other functions.
int file_bin_open(const std::string& fname, int mode) {
  // TODO: add other modes like trunc / append?
  switch (mode) {
    case 0: return enigma::file_open(fname, std::ios::in  | std::ios::binary);
    case 1: return enigma::file_open(fname, std::ios::out | std::ios::binary);
    case 2: return enigma::file_open(fname, std::ios::in  | std::ios::out | std::ios::binary);
    default: return -1;
  }
}

// Rewrites the file with the given file id, that is, clears it and starts writing at the start.
bool file_bin_rewrite(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return false;
  enigma::files.get(fileid).fs.close();
  enigma::files.get(fileid).fs.open(enigma::files.get(fileid).fn, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
  try_io_and_print(enigma::files.get(fileid))
  return enigma::files.get(fileid).fs.good();
}

// Closes the file with the given file id.
void file_bin_close(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return;
  enigma::files.get(fileid).fs.close();
}

// Returns the size (in bytes) of the file with the given file id.
size_t file_bin_size(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return 0;
  auto& fs = enigma::files.get(fileid).fs;
  if (!fs.is_open()) return 0;

fs.clear();
std::streampos currPos = fs.tellg();

bool use_output = false;
if (currPos == std::streampos(-1)) {
  fs.clear();
  currPos = fs.tellp();
  use_output = true;
}
if (currPos == std::streampos(-1)) {
  fs.clear();
  return 0;
}
fs.clear();
if (use_output) {
  fs.seekp(0, std::ios::end);
} else {
  fs.seekg(0, std::ios::end);
}
	  
if (!fs) {	  
  fs.clear();
  if (use_output) fs.seekp(currPos);
  else fs.seekg(currPos);
   return 0;
 }

const std::streampos endPos =
    use_output ? fs.tellp() : fs.tellg();

  fs.clear();
if (use_output) fs.seekp(currPos);

else fs.seekg(currPos);
if (endPos == std::streampos(-1) || !fs) {
  fs.clear();
  return 0;
}
const std::streamoff length = endPos;

if (length < 0 ||
    static_cast<std::uintmax_t>(length) >
        static_cast<std::uintmax_t>(
            std::numeric_limits<size_t>::max())) {
  return 0;
}

try_io_and_print(enigma::files.get(fileid))

return static_cast<size_t>(length);
}

// Returns the current position (in bytes; 0 is the first position) of the file with the given file id.
size_t file_bin_position(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return 0;
  const std::streampos position = enigma::files.get(fileid).fs.tellg();
  if (position < 0) {
    try_io_and_print(enigma::files.get(fileid))
    return 0;
  }
  return static_cast<size_t>(position);
}

// Moves the current position of the file to the indicated position. To append to a file move the position to the size of the file before writing.
void file_bin_seek(int fileid, size_t pos) {
  if (!enigma::valid_file_handle(fileid)) return;	
  enigma::files.get(fileid).fs.seekg(pos);
  enigma::files.get(fileid).fs.seekp(pos);
  try_io_and_print(enigma::files.get(fileid))
}

// Writes a byte of data to the file with the given file id.
void file_bin_write_byte(int fileid, unsigned char byte) {
  if (!enigma::valid_file_handle(fileid)) return;	
  enigma::files.get(fileid).fs.put(static_cast<char>(byte));
  try_io_and_print(enigma::files.get(fileid))
}

// Reads a byte of data from the file and returns this
int file_bin_read_byte(int fileid) {
  if (!enigma::valid_file_handle(fileid)) return -1;
  const int value = enigma::files.get(fileid).fs.get();
  bool good = enigma::files.get(fileid).fs.good();
  try_io_and_print(enigma::files.get(fileid))
  return (!good) ? -1 : value;
}

} // NAMESPACE enigma_user
