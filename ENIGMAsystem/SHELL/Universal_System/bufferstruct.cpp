/** Copyright (C) 2013 Robert B. Colton
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

#include "buffers.h"
#include "buffers_internal.h"
#include "libEGMstd.h"

#include "Resources/AssetArray.h" // TODO: start actually using for this resource
#include "Graphics_Systems/graphics_mandatory.h"
#include "Graphics_Systems/General/GSsurface.h"
#include "Widget_Systems/widgets_mandatory.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <cstdint>
#include <algorithm>

using std::string;

namespace enigma {
std::vector<BinaryBuffer*> buffers(0);

BinaryBuffer::BinaryBuffer(unsigned size) {
  data.resize(size, 0);
  position = 0;
  alignment = 1;
  type = 0;
}

unsigned BinaryBuffer::GetSize() { return data.size(); }

void BinaryBuffer::Resize(unsigned size) { data.resize(size, 0); }

void BinaryBuffer::Seek(unsigned offset) {
  if (GetSize() == 0) {
    position = 0;
    return;
  }

  position = offset;
  while (position >= GetSize()) {
    switch (type) {
      case enigma_user::buffer_grow:
        if (position == std::numeric_limits<unsigned>::max())
          return;      
        Resize(position + 1);
        return;
      case enigma_user::buffer_wrap:
        position %= GetSize();
        return;
      default:
        position = GetSize() - (position - GetSize());
        return;
    }
  }
}

unsigned char BinaryBuffer::ReadByte() {
  if (GetSize() == 0)
    return 0;

  Seek(position);
  if (position >= GetSize())
    return 0;

  unsigned char byte = data[position];
  if (position < std::numeric_limits<unsigned>::max())
    Seek(position + 1);
  return byte;
}

void BinaryBuffer::WriteByte(unsigned char byte) {
  if (GetSize() == 0)
    return;

  Seek(position);
  if (position >= GetSize())
    return;

  data[position] = byte;
  if (position < std::numeric_limits<unsigned>::max())
    Seek(position + 1);
}

int get_free_buffer() {
  for (unsigned i = 0; i < buffers.size(); i++) {
    if (!buffers[i]) {
      return i;
    }
  }
  return buffers.size();
}

std::vector<unsigned char> valToBytes(variant value, unsigned count) {
  std::vector<unsigned char> result(0);
  for (unsigned i = 0; i < count; i++) {
    result.push_back(value >> ((i)*8));
  }
  return result;
}
}  // namespace enigma

namespace enigma_user {

int buffer_create(unsigned size, int type, unsigned alignment) {
  enigma::BinaryBuffer* buffer = new enigma::BinaryBuffer(size);
  buffer->type = type;
  buffer->alignment = alignment;
  int id = enigma::get_free_buffer();
  enigma::buffers.insert(enigma::buffers.begin() + id, buffer);
  return id;
}

void buffer_delete(int buffer) {
  get_buffer(binbuff, buffer);
 delete enigma::buffers[buffer];
 enigma::buffers[buffer] = nullptr;
}

bool buffer_exists(int buffer) {
  return (buffer >= 0 && (size_t)buffer < enigma::buffers.size() && enigma::buffers[buffer] != nullptr);
}

void buffer_copy(int src_buffer, unsigned src_offset, unsigned size, int dest_buffer, unsigned dest_offset) {
  get_buffer(srcbuff, src_buffer);
  get_buffer(dstbuff, dest_buffer);
  if (src_offset > srcbuff->GetSize())
    return;
  if (dest_offset > dstbuff->GetSize())
    return;

  const unsigned src_size = srcbuff->GetSize();
  const unsigned available = src_size - src_offset;
  const unsigned over = size > available ? size - available : 0;
  switch (dstbuff->type) {
    case buffer_wrap:
      if (srcbuff == dstbuff) {
        const unsigned copy_size = size - over;
        std::vector<unsigned char> temp(
            srcbuff->data.begin() + src_offset,
            srcbuff->data.begin() + src_offset + copy_size);
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             temp.begin(), temp.end());
      } else {
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             srcbuff->data.begin() + src_offset,
                             srcbuff->data.begin() + src_offset + size - over);
      }
      dstbuff->data.insert(dstbuff->data.begin() + dest_offset, srcbuff->data.begin(), srcbuff->data.begin() + over);
      break;
    case buffer_grow:
      if (srcbuff == dstbuff) {
        const unsigned copy_size = size - over;
        std::vector<unsigned char> temp(
            srcbuff->data.begin() + src_offset,
            srcbuff->data.begin() + src_offset + copy_size);
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             temp.begin(), temp.end());
      } else {
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             srcbuff->data.begin() + src_offset,
                             srcbuff->data.begin() + src_offset + size - over);
      }
      break;
    default:
      if (srcbuff == dstbuff) {
        const unsigned copy_size = size - over;
        std::vector<unsigned char> temp(
            srcbuff->data.begin() + src_offset,
            srcbuff->data.begin() + src_offset + copy_size);
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             temp.begin(), temp.end());
      } else {
        dstbuff->data.insert(dstbuff->data.begin() + dest_offset,
                             srcbuff->data.begin() + src_offset,
                             srcbuff->data.begin() + src_offset + size - over);
      }
      break;
  }
}

void buffer_save(int buffer, string filename) {
  get_buffer(binbuff, buffer);
  std::ofstream myfile(filename.c_str(), std::ios::binary);
  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return;
  }
  if (!binbuff->data.empty())
    myfile.write(reinterpret_cast<const char*>(binbuff->data.data()),
                 static_cast<std::streamsize>(binbuff->data.size()));
  myfile.close();
}

void buffer_save_ext(int buffer, string filename, unsigned offset, unsigned size) {
  get_buffer(binbuff, buffer);
  if (offset > binbuff->GetSize())
    return;

  std::ofstream myfile(filename.c_str(), std::ios::binary);
  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return;
  }

  const unsigned available = binbuff->GetSize() - offset;
  if (binbuff->type != buffer_wrap && size > available)
    size = available;

  const unsigned over =
      binbuff->type == buffer_wrap && size > available
          ? size - available
          : 0;

  switch (binbuff->type) {
    case buffer_wrap:
      if (size > 0 && available > 0) {
        const unsigned first = std::min(size, available);
        myfile.write(
            reinterpret_cast<const char*>(binbuff->data.data() + offset),
            static_cast<std::streamsize>(first));

        if (size > first && !binbuff->data.empty()) {
          const unsigned second = size - first;
          myfile.write(
              reinterpret_cast<const char*>(binbuff->data.data()),
              static_cast<std::streamsize>(second));
        }
      }
      break;
    case buffer_grow:
      //TODO: Might need to use min(size, binbuff->GetSize()); for the last parameter.
      //Depends on whether Stupido will write 0's to fill in the entire size you gave it even though the data isn't that big.
      myfile.write(reinterpret_cast<const char*>(binbuff->data.data() + offset),
                   std::min(size, available));
      break;
    default:
    if (size > binbuff->GetSize() - offset)
      size = binbuff->GetSize() - offset;
    if (size > 0)
      myfile.write(
          reinterpret_cast<const char*>(binbuff->data.data() + offset),
          static_cast<std::streamsize>(size));
      break;
  }

  myfile.close();
}

int buffer_load(string filename) {
  enigma::BinaryBuffer* buffer = new enigma::BinaryBuffer(0);
  buffer->type = buffer_grow;
  buffer->alignment = 1;
  int id = enigma::get_free_buffer();
  enigma::buffers.insert(enigma::buffers.begin() + id, buffer);
  std::ifstream myfile(filename.c_str(), std::ios::binary);
  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    delete buffer;
    return -1;
  }
  myfile.seekg(0, std::ios::end);
  const std::streampos file_size = myfile.tellg();
  if (file_size < 0 ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(std::numeric_limits<unsigned>::max())) {
    myfile.close();
    delete buffer;
    return -1;
  }

  buffer->data.resize(static_cast<size_t>(file_size));
  myfile.seekg(0, std::ios::beg);
  if (file_size > 0)
    myfile.read(reinterpret_cast<char*>(buffer->data.data()), file_size);
  myfile.close();
  enigma::buffers.insert(enigma::buffers.begin() + id, buffer);

  return id;
}

void buffer_load_ext(int buffer, string filename, unsigned offset) {
  get_buffer(binbuff, buffer);
  if (offset > binbuff->GetSize() &&
      binbuff->type != buffer_grow)
    return;

  std::ifstream myfile(filename.c_str(), std::ios::binary);
  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return;
  }
  std::vector<char> data;
  myfile.seekg(0, std::ios::end);
  const std::streampos file_size = myfile.tellg();
  if (file_size < 0 ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(std::numeric_limits<unsigned>::max())) {
    myfile.close();
    return;
  }

  data.resize(static_cast<size_t>(file_size));
  myfile.seekg(0, std::ios::beg);
  if (file_size > 0)
    myfile.read(data.data(), file_size);
  const size_t buffer_size = binbuff->GetSize();
  const size_t data_size = data.size();
  const size_t available = buffer_size - offset;
  const size_t over =
      data_size > available ? data_size - available : 0;
  switch (binbuff->type) {
    case buffer_wrap:
      binbuff->data.insert(binbuff->data.begin() + offset, data.begin(), data.end() - over);
      binbuff->data.insert(binbuff->data.begin(), data.begin(), data.begin() + over);
      break;
    case buffer_grow:
      binbuff->data.insert(binbuff->data.begin() + offset, data.begin(), data.end());
      break;
    default:
      binbuff->data.insert(binbuff->data.begin() + offset, data.begin(), data.end() - over);
      break;
  }

  myfile.close();
}

void buffer_fill(int buffer, unsigned offset, int type, variant value, unsigned size) {
  get_buffer(binbuff, buffer);
  if (size > std::numeric_limits<unsigned>::max() - offset)
    return;

  unsigned nsize = offset + size;
  
  if (binbuff->GetSize() < nsize && binbuff->type == buffer_grow) {
    binbuff->data.resize(nsize);
  }
  unsigned pos = offset;
  const unsigned type_size = buffer_sizeof(type);
  for (unsigned i = 0; i < type_size; i++) {
    if (pos >= binbuff->GetSize())
      break;

	binbuff[pos] = value[i];  
    ++pos;
  }
}
  
void *buffer_get_address(int buffer) {
  if (buffer < 0 ||
      static_cast<size_t>(buffer) >= enigma::buffers.size() ||
      !enigma::buffers[buffer]) {
    #ifdef DEBUG_MODE
    DEBUG_MESSAGE("Attempting to access non-existing buffer " + toString(buffer), MESSAGE_TYPE::M_USER_ERROR);
    #endif   
    return nullptr;
  }

  enigma::BinaryBuffer *binbuff = enigma::buffers[buffer];
  return reinterpret_cast<void *>(binbuff->data.data());
}

unsigned buffer_get_size(int buffer) {
  get_bufferr(binbuff, buffer, -1);
  return binbuff->GetSize();
}

unsigned buffer_get_alignment(int buffer) {
  get_bufferr(binbuff, buffer, -1);
  return binbuff->alignment;
}

int buffer_get_type(int buffer) {
  get_bufferr(binbuff, buffer, -1);
  return binbuff->type;
}

void buffer_get_surface(int buffer, int surface, int mode, unsigned offset, int modulo) {
  //get_buffer(binbuff, buffer);
  //TODO: Write this function
  DEBUG_MESSAGE("Function unimplemented: buffer_get_surface", MESSAGE_TYPE::M_WARNING);
}

void buffer_set_surface(int buffer, int surface, int mode, unsigned offset, int modulo) {
  int tex = surface_get_texture(surface);
  int wid = surface_get_width(surface);
  int hgt = surface_get_height(surface);

  if (wid <= 0 || hgt <= 0) {
    DEBUG_MESSAGE("Surface has invalid dimensions!", MESSAGE_TYPE::M_WARNING);
    return;
  }

  const std::uint64_t expected_size =
      static_cast<std::uint64_t>(buffer_sizeof(buffer_u64)) *
      static_cast<std::uint64_t>(wid) *
      static_cast<std::uint64_t>(hgt);

  if (buffer_get_size(buffer) == expected_size) {
    enigma::graphics_push_texture_pixels(tex, wid, hgt, (unsigned char *)buffer_get_address(buffer));
  } else { // execution can not continue safely with wrong buffer size
    DEBUG_MESSAGE("Buffer allocated with wrong length!", MESSAGE_TYPE::M_WARNING);
  }
}

void buffer_resize(int buffer, unsigned size) {
  get_buffer(binbuff, buffer);
  binbuff->Resize(size);
}

void buffer_seek(int buffer, int base, unsigned offset) {
  get_buffer(binbuff, buffer);
  switch (base) {
    case buffer_seek_start:
      binbuff->Seek(offset);
      break;
    case buffer_seek_end:
      if (offset > std::numeric_limits<unsigned>::max() - binbuff->GetSize())
        return;
      binbuff->Seek(binbuff->GetSize() + offset);
      break;
    case buffer_seek_relative:
      if (offset > std::numeric_limits<unsigned>::max() - binbuff->position)
        return;   
      binbuff->Seek(binbuff->position + offset);
      break;
  }
}

unsigned buffer_sizeof(int type) {
  switch (type) {
    case buffer_u8: case buffer_s8: case buffer_bool:
      return 1;
    case buffer_u16: case buffer_s16: case buffer_f16:
      return 2;
    case buffer_u32: case buffer_s32: case buffer_f32:
      return 4;
    case buffer_u64: case buffer_f64:
      return 8;
    case buffer_string: case buffer_text: default:
      break;
  }
  return 0;
}

int buffer_tell(int buffer) {
  get_bufferr(binbuff, buffer, -1);
  return binbuff->position;
}

variant buffer_peek(int buffer, unsigned offset, int type) {
  get_bufferr(binbuff, buffer, -1);
  const unsigned saved_position = binbuff->position;
  binbuff->Seek(offset);
  if (type != buffer_string) {
    //unsigned dsize = buffer_sizeof(type) + binbuff->alignment - 1;
    //NOTE: These buffers most likely need a little more code added to take care of endianess on different architectures.
    //TODO: Fix floating point precision.
    std::uint64_t res = 0;
    for (unsigned i = 0; i < buffer_sizeof(type); ++i) {
      res |= static_cast<std::uint64_t>(binbuff->ReadByte()) << (i * 8);
    }
    binbuff->position = saved_position;
    return res;
  } else {
    char byte = '1';
    std::vector<char> data;
    while (byte != 0x00 && binbuff->position < binbuff->GetSize()) {
      byte = binbuff->ReadByte();
      if (byte != 0x00)
        data.push_back(byte);
    }
    variant result = data.empty() ? variant("") : variant(&data[0]);
    binbuff->position = saved_position;
    return result;
  }
}

variant buffer_read(int buffer, int type) {
  get_bufferr(binbuff, buffer, -1);
  return buffer_peek(buffer, binbuff->position, type);
}

void buffer_poke(int buffer, unsigned offset, int type, variant value) {
  get_buffer(binbuff, buffer);
  binbuff->Seek(offset);
  if (type != buffer_string) {
    //TODO: Implement buffer alignment.
    //unsigned dsize = buffer_sizeof(type); //+ binbuff->alignment - 1;
    std::vector<unsigned char> data = enigma::valToBytes(value, buffer_sizeof(type));
    for (unsigned i = 0; i < data.size(); i++) {
      binbuff->WriteByte(data[i]);
    }
  } else {
    unsigned pos = 0;
    const unsigned length = static_cast<unsigned>(value.string_length());
    while (pos <= length) {
      const char byte = value.char_at(pos);
      ++pos;
      binbuff->WriteByte(byte);
      if (byte == 0x00)
        break;      
    }
    if (binbuff->alignment > pos) {
      for (unsigned i = 0; i < binbuff->alignment - pos; i++) {
        binbuff->WriteByte(0);
      }
    }
  }
}

void buffer_write(int buffer, int type, variant value) {
  get_buffer(binbuff, buffer);
  buffer_poke(buffer, binbuff->position, type, value);
}

string buffer_md5(int buffer, unsigned offset, unsigned size) {
  //get_bufferr(binbuff, buffer, 0);
  //TODO: Write this function
  return NULL;
}

string buffer_sha1(int buffer, unsigned offset, unsigned size) {
  //get_bufferr(binbuff, buffer, 0);
  //TODO: Write this function
  return NULL;
}

int buffer_base64_decode(string str) {
  enigma::BinaryBuffer* buffer = new enigma::BinaryBuffer(0);
  buffer->type = buffer_grow;
  buffer->alignment = 1;
  int id = enigma::get_free_buffer();
  enigma::register_buffer(id, buffer);
  //TODO: Write this function
  return id;
}

int buffer_base64_decode_ext(int buffer, string str, unsigned offset) {
  //get_bufferr(binbuff, buffer, -1);
  //TODO: Write this function
  return 0;
}

string buffer_base64_encode(int buffer, unsigned offset, unsigned size) {
  //get_bufferr(binbuff, buffer, 0);
  //TODO: Write this function
  return NULL;
}

void game_save_buffer(int buffer) {
  //get_buffer(binbuff, buffer);
  //TODO: Write this function
}

void game_load_buffer(int buffer) {
  //get_buffer(binbuff, buffer);
  //TODO: Write this function
}

}  // namespace enigma_user
