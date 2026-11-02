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
  const unsigned buffer_size = GetSize();

  if (buffer_size == 0) {
    position = 0;
    return;
  }

  switch (type) {
    case enigma_user::buffer_grow:
      if (offset >= buffer_size) {
        if (offset == std::numeric_limits<unsigned>::max())
          return;
        Resize(offset + 1);
      }
      position = offset;
      return;

    case enigma_user::buffer_wrap:
      position = offset % buffer_size;
      return;

    default:
      position = std::min(offset, buffer_size - 1);
      return;
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
  if (static_cast<size_t>(id) < enigma::buffers.size()) {
    enigma::buffers[static_cast<size_t>(id)] = buffer;
  } else {
    enigma::buffers.push_back(buffer);
  };
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

  const size_t src_size = srcbuff->data.size();
  const size_t dst_size = dstbuff->data.size();
  if (size == 0 || src_size == 0)
    return;

  const bool wrap = dstbuff->type == buffer_wrap;
  const size_t available = src_size - src_offset;
  const size_t copy_size = wrap ? size : std::min<size_t>(size, available);
  if (copy_size == 0)
    return;

  // Snapshot the source first; this also handles overlapping self-copies.
  std::vector<unsigned char> temp;
  temp.reserve(copy_size);
  for (size_t i = 0; i < copy_size; ++i) {
    const size_t source_index =
        wrap ? (static_cast<size_t>(src_offset) + i) % src_size
             : static_cast<size_t>(src_offset) + i;
    temp.push_back(srcbuff->data[source_index]);
  }

  if (wrap) {
    if (dst_size == 0)
      return;
    for (size_t i = 0; i < copy_size; ++i)
      dstbuff->data[(static_cast<size_t>(dest_offset) + i) % dst_size] = temp[i];
    return;
  }

    if (dstbuff->type == buffer_grow) {
    if (copy_size > std::numeric_limits<size_t>::max() - dest_offset)
      return;

    const size_t required_size = static_cast<size_t>(dest_offset) + copy_size;
    if (required_size > dstbuff->data.max_size())
      return;
 
    if (required_size > dstbuff->data.size())
      dstbuff->data.resize(required_size, 0);

  } else {
    if (dest_offset >= dstbuff->data.size())
      return;

  }

  const size_t writable =
      std::min(copy_size, dstbuff->data.size() - dest_offset);

  std::copy_n(temp.begin(), writable, dstbuff->data.begin() + dest_offset);
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
    case buffer_wrap: {
      const size_t buffer_size = binbuff->data.size();

      if (size == 0 || buffer_size == 0 || available == 0)
        break;

      size_t remaining = size;
      size_t current_offset = offset;
      const size_t max_write = static_cast<size_t>(
          (std::numeric_limits<std::streamsize>::max)());

      while (remaining > 0) {
        const size_t chunk = std::min({
            remaining,
            buffer_size - current_offset,
            max_write
        });

        if (chunk == 0)
          break;

        myfile.write(
            reinterpret_cast<const char*>(
                binbuff->data.data() + current_offset),
            static_cast<std::streamsize>(chunk));

        if (!myfile)
          break;

        remaining -= chunk;
        current_offset = 0;
      }
      break;
   }
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
  if (file_size > 0) {
    myfile.read(reinterpret_cast<char*>(buffer->data.data()), file_size);
    if (!myfile) {
      myfile.close();
      delete buffer;
      return -1;
    }
  }    
  myfile.close();
  if (static_cast<size_t>(id) < enigma::buffers.size()) {
    enigma::buffers[id] = buffer;
  } else {
    enigma::buffers.push_back(buffer);
  }

  return id;
}

void buffer_load_ext(int buffer, string filename, unsigned offset) {
  get_buffer(binbuff, buffer);
  if (offset > binbuff->GetSize())
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
  if (file_size > 0) {
    myfile.read(data.data(), file_size);
    if (!myfile) {
      myfile.close();
      return;
    }
  }
  myfile.close();
  
  const size_t start = static_cast<size_t>(offset);
  const size_t data_size = data.size();

  if (data_size == 0)
    return;

  if (binbuff->type == buffer_grow) {
    if (data_size > std::numeric_limits<size_t>::max() - start)
      return;

    const size_t required_size = start + data_size;

    if (required_size > binbuff->data.max_size())
      return;

    if (required_size > binbuff->data.size())
      binbuff->data.resize(required_size, 0);

    std::copy(data.begin(), data.end(),
              binbuff->data.begin() + start);
    return;
   }

  const size_t buffer_size = binbuff->data.size();

  if (buffer_size == 0)
    return;

  if (binbuff->type == buffer_wrap) {
    size_t position = start % buffer_size;

    for (size_t i = 0; i < data_size; ++i) {
      binbuff->data[position] =
          static_cast<unsigned char>(data[i]);

      ++position;
      if (position == buffer_size)
        position = 0;
    }
    return;
  }

  if (start >= buffer_size)
    return;

  const size_t writable =
      std::min(data_size, buffer_size - start);

  std::copy_n(data.begin(), writable,
              binbuff->data.begin() + start);
}

void buffer_fill(int buffer, unsigned offset, int type, variant value, unsigned size) {
  get_buffer(binbuff, buffer);
  if (size > std::numeric_limits<unsigned>::max() - offset)
    return;

  const unsigned type_size = buffer_sizeof(type);
  if (type_size == 0)
    return;

  const size_t start = static_cast<size_t>(offset);
  const size_t count = static_cast<size_t>(size);

  if (binbuff->type == buffer_grow) {
    if (count > std::numeric_limits<size_t>::max() - start)
      return;

    const size_t required_size = start + count;
    if (required_size > binbuff->data.max_size())
      return;

    if (required_size > binbuff->data.size())
      binbuff->data.resize(required_size, 0);
  }

  const size_t buffer_size = binbuff->data.size();
  if (buffer_size == 0 || start >= buffer_size)
    return;

  size_t writable = count;
  if (binbuff->type == buffer_wrap) {
    for (size_t i = 0; i < writable; ++i) {
      const size_t index = (start + i) % buffer_size;
      binbuff->data[index] =
          static_cast<unsigned char>(value[i % type_size]);
    }
    return;
  }

  writable = std::min(writable, buffer_size - start);
  for (size_t i = 0; i < writable; ++i)
    binbuff->data[start + i] =
        static_cast<unsigned char>(value[i % type_size]);
}
  
void *buffer_get_address(int buffer) {
  if (!enigma::buffer_valid(buffer)) {
    #ifdef DEBUG_MODE
    DEBUG_MESSAGE("Attempting to access non-existing buffer " + toString(buffer), MESSAGE_TYPE::M_USER_ERROR);
    #endif   
    return nullptr;
  }

  enigma::BinaryBuffer *binbuff = enigma::buffers[buffer];
  if (binbuff->data.empty())
    return nullptr;

  return static_cast<void *>(binbuff->data.data());
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

  const size_t buffer_size =
      static_cast<size_t>(buffer_get_size(buffer));
  const size_t start = static_cast<size_t>(offset);

  if (start > buffer_size ||
      expected_size > buffer_size - start ||
      expected_size > std::numeric_limits<size_t>::max()) {
    DEBUG_MESSAGE("Buffer allocated with wrong length!", MESSAGE_TYPE::M_WARNING);
    return;
  }

  unsigned char* pixels =
      static_cast<unsigned char*>(buffer_get_address(buffer));

  if (!pixels)
    return;

  enigma::graphics_push_texture_pixels(tex, wid, hgt, pixels + start);  
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
