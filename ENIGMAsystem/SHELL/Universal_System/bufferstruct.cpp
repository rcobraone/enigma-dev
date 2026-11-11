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
#include <new>
#include <stdexcept>
#include <memory>

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

void BinaryBuffer::Resize(unsigned size) {
  if (static_cast<size_t>(size) > data.max_size())
    return;

  try {
    data.resize(size, 0);
  } catch (const std::bad_alloc&) {
    return;
  } catch (const std::length_error&) {
    return;
  }

  if (position > size)
    position = size;
}

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
        const unsigned required_size = offset + 1;
        if (static_cast<size_t>(required_size) > data.max_size())
          return;

        try {
          data.resize(required_size, 0);
        } catch (const std::bad_alloc&) {
          return;
        } catch (const std::length_error&) {
          return;
        }
      }
      position = offset;
      return;

    case enigma_user::buffer_wrap:
      position = offset % buffer_size;
      return;

    default:
      position = std::min(offset, buffer_size);
      return;
  }
}

unsigned char BinaryBuffer::ReadByte() {
  const size_t buffer_size = data.size();

  if (buffer_size == 0)
    return 0;

  if (type == enigma_user::buffer_wrap) {
    const size_t index = static_cast<size_t>(position) % buffer_size;
    const unsigned char byte = data[index];
    position = static_cast<unsigned>((index + 1) % buffer_size);
    return byte;
  }
  if (type == enigma_user::buffer_grow) {
    if (position == std::numeric_limits<unsigned>::max())
      return 0;

    if (static_cast<size_t>(position) >= buffer_size) {
      const size_t required_size = static_cast<size_t>(position) + 1;

      if (required_size > data.max_size() ||
          required_size > std::numeric_limits<unsigned>::max())
        return 0;

      try {
        data.resize(required_size, 0);
      } catch (const std::bad_alloc&) {
        return 0;
      } catch (const std::length_error&) {
        return 0;
      }
    }
  } else if (static_cast<size_t>(position) >= buffer_size) {
    return 0;
  }

  const unsigned char byte = data[position];

  if (position < std::numeric_limits<unsigned>::max())
    ++position;

  return byte;
}

void BinaryBuffer::WriteByte(unsigned char byte) {
  const size_t buffer_size = data.size();

  if (type == enigma_user::buffer_wrap) {
    if (buffer_size == 0)
      return;

    const size_t index = static_cast<size_t>(position) % buffer_size;
    data[index] = byte;
    position = static_cast<unsigned>((index + 1) % buffer_size);
    return;
  }

  if (type == enigma_user::buffer_grow) {
    if (position == std::numeric_limits<unsigned>::max())
      return;

    const size_t index = static_cast<size_t>(position);
    const size_t required_size = index + 1;

    if (required_size > data.max_size() ||
        required_size > std::numeric_limits<unsigned>::max())
      return;

    if (required_size > buffer_size) {
      try {
        data.resize(required_size, 0);
      } catch (const std::bad_alloc&) {
        return;
      } catch (const std::length_error&) {
        return;
      }
    }

    data[index] = byte;
    ++position;
    return;
  }

  const size_t index = static_cast<size_t>(position);
  if (index >= buffer_size)
    return;

  data[index] = byte;

  if (position < std::numeric_limits<unsigned>::max())
    ++position;
}

int get_free_buffer() {
  for (size_t i = 0; i < buffers.size(); ++i) {
    if (!buffers[i]) {
      if (i > static_cast<size_t>(
                  std::numeric_limits<int>::max()))
        return -1;
 
      return static_cast<int>(i);
    }
  }

  if (buffers.size() > static_cast<size_t>(
                           std::numeric_limits<int>::max()))
    return -1;

  return static_cast<int>(buffers.size());
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
  const int id = enigma::get_free_buffer();

  if (id < 0)
    return -1; 

  std::unique_ptr<enigma::BinaryBuffer> buffer;
  try {
    buffer.reset(new enigma::BinaryBuffer(size));
  } catch (const std::bad_alloc&) {
    return -1;
  } catch (const std::length_error&) {
    return -1;
  }
  
  buffer->type = type;
  buffer->alignment = alignment;
  const size_t index = static_cast<size_t>(id);

  if (index < enigma::buffers.size()) {
    enigma::buffers[index] = buffer.get();

  } else {
    try {
      enigma::buffers.push_back(buffer.get());
    } catch (const std::bad_alloc&) {
      return -1;
    } catch (const std::length_error&) {
      return -1;
    }
  }

  buffer.release();
  return id;

}

void buffer_delete(int buffer) {
  get_buffer(binbuff, buffer);

  delete binbuff;
  enigma::buffers[static_cast<size_t>(buffer)] = nullptr;
}

bool buffer_exists(int buffer) {
  return (buffer >= 0 && (size_t)buffer < enigma::buffers.size() && enigma::buffers[buffer] != nullptr);
}

void buffer_copy(int src_buffer, unsigned src_offset, unsigned size,
                 int dest_buffer, unsigned dest_offset) {

  get_buffer(srcbuff, src_buffer);
  get_buffer(dstbuff, dest_buffer);
 
  const size_t src_size = srcbuff->data.size();
  const size_t dst_size = dstbuff->data.size();
 
  if (size == 0 || src_size == 0)
    return; 

  const bool src_wrap = srcbuff->type == buffer_wrap;
  const bool dst_wrap = dstbuff->type == buffer_wrap;
  const bool dst_grow = dstbuff->type == buffer_grow;

  const size_t source_offset = static_cast<size_t>(src_offset);
  const size_t destination_offset = static_cast<size_t>(dest_offset);

  // A non-wrapping source cannot be read beyond its end.
  if (!src_wrap && source_offset > src_size)
    return;
 

  // Empty wrapping destination cannot accept data.
  if (dst_wrap && dst_size == 0)
    return;

  // A non-wrapping destination cannot start beyond its end,
  // unless it is allowed to grow.
  if (!dst_wrap && !dst_grow && destination_offset > dst_size)
    return;

  const size_t available = src_wrap
      ? static_cast<size_t>(size)
      : src_size - source_offset;

  const size_t copy_size =
      std::min(static_cast<size_t>(size), available); 

  if (copy_size == 0)
    return;

  // Check arithmetic before allocating or modifying the destination.
  size_t required_size = 0;
  if (dst_grow) {
    if (copy_size >
        std::numeric_limits<size_t>::max() - destination_offset)       
      return;

    required_size = destination_offset + copy_size;

    if (required_size > dstbuff->data.max_size() ||
        required_size > std::numeric_limits<unsigned>::max())
      return;
  }

  // Snapshot protects overlapping copies, including self-copy.
  std::vector<unsigned char> temp;
  if (copy_size > temp.max_size())
    return;

  try {
    temp.resize(copy_size);
  } catch (const std::bad_alloc&) {
    return;
  } catch (const std::length_error&) {
    return;
  }

  const size_t source_start = src_wrap
      ? source_offset % src_size
      : source_offset;

  size_t source_index = source_start;
  for (size_t i = 0; i < copy_size; ++i) {
    temp[i] = srcbuff->data[source_index];

    if (src_wrap) {
      ++source_index;
      if (source_index == src_size)
        source_index = 0;
    } else {
      ++source_index;
    }
  }

  if (dst_wrap) {
   size_t destination_index = destination_offset % dst_size;
    
    for (size_t i = 0; i < copy_size; ++i) {
      dstbuff->data[destination_index] = temp[i];
      ++destination_index;
      if (destination_index == dst_size)
        destination_index = 0;
    }

    return;
  }

  if (dst_grow) {
    if (required_size > dstbuff->data.size()) {
      try {
        dstbuff->data.resize(required_size, 0);
      } catch (const std::bad_alloc&) {
        return;
      } catch (const std::length_error&) {
        return;
      }
    }
  }
  
  const size_t writable = std::min(
      copy_size,
      dstbuff->data.size() - destination_offset
  );
   
  std::copy_n(
      temp.begin(),
      writable,
      dstbuff->data.begin() + destination_offset
  );
}
void buffer_save(int buffer, string filename) {
  get_buffer(binbuff, buffer);

  const size_t data_size = binbuff->data.size();

  if (data_size >
      static_cast<size_t>(
          (std::numeric_limits<std::streamsize>::max)())) {
    DEBUG_MESSAGE("Buffer is too large to save!", MESSAGE_TYPE::M_ERROR);
    return;
  }

  std::ofstream myfile(filename.c_str(), std::ios::binary);

  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return;
  }

 

  if (data_size > 0) {
    myfile.write(
        reinterpret_cast<const char*>(binbuff->data.data()),
        static_cast<std::streamsize>(data_size));
  }

  if (!myfile) {
    DEBUG_MESSAGE("Unable to write buffer to file " + filename,
                  MESSAGE_TYPE::M_ERROR);
    myfile.close();
    return;
  }

  myfile.close();

  if (!myfile) {
    DEBUG_MESSAGE("Unable to close file " + filename,
                  MESSAGE_TYPE::M_ERROR);
  }

}

void buffer_save_ext(int buffer, string filename, unsigned offset,

                     unsigned size) {
  get_buffer(binbuff, buffer);

  const size_t buffer_size = binbuff->data.size();
  const size_t start = static_cast<size_t>(offset);

  if (start > buffer_size)
    return;
  const size_t max_write = static_cast<size_t>(
      (std::numeric_limits<std::streamsize>::max)());

  if (max_write == 0)
    return;

  std::ofstream myfile(filename.c_str(), std::ios::binary);

  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename,
                  MESSAGE_TYPE::M_ERROR);
    return;
  }

  if (binbuff->type == buffer_wrap) {
    if (size > 0 && buffer_size > 0) {
      size_t remaining = static_cast<size_t>(size);
      size_t position = start % buffer_size;

      while (remaining > 0) {
        const size_t chunk = std::min({
            remaining,
            buffer_size - position,
            max_write
        });

        if (chunk == 0)
          break;

        myfile.write(
            reinterpret_cast<const char*>(
                binbuff->data.data() + position),
            static_cast<std::streamsize>(chunk));

        if (!myfile)
          break;

        remaining -= chunk;
        position = 0;
      }
    }

  } else {
    const size_t available = buffer_size - start;
    const size_t writable = std::min(
        static_cast<size_t>(size),
        available);

    size_t written = 0;

    while (written < writable) {
      const size_t chunk = std::min(

          writable - written,
          max_write);

      myfile.write(
          reinterpret_cast<const char*>(
              binbuff->data.data() + start + written),
          static_cast<std::streamsize>(chunk));

      if (!myfile)
        break;

      written += chunk;
    }
  }

  if (!myfile) {
    DEBUG_MESSAGE("Unable to write buffer to file " + filename,
                  MESSAGE_TYPE::M_ERROR);
    myfile.close();
    return;
  }

  myfile.close();

  if (!myfile) {
    DEBUG_MESSAGE("Unable to close file " + filename,
                  MESSAGE_TYPE::M_ERROR);
  }

}

int buffer_load(string filename) {
  const int id = enigma::get_free_buffer();
  if (id < 0)
    return -1;

  std::unique_ptr<enigma::BinaryBuffer> buffer;
  try {
    buffer.reset(new enigma::BinaryBuffer(0));
  } catch (const std::bad_alloc&) {
    return -1;
  } catch (const std::length_error&) {
    return -1;
  }

  buffer->type = buffer_grow;
  buffer->alignment = 1;

  std::ifstream myfile(filename.c_str(), std::ios::binary); 
  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return -1;
  }
  myfile.seekg(0, std::ios::end);
  if (!myfile) {
    myfile.close();
    return -1;
  }

  const std::streampos file_position = myfile.tellg();
  if (file_position == std::streampos(-1)) {
    myfile.close();
    return -1;
  }

  const std::streamoff file_size =
      static_cast<std::streamoff>(file_position);

  if (file_size < 0 ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<unsigned>::max()) ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<std::streamsize>::max())) {
    myfile.close();
    return -1;
  }

  const size_t size = static_cast<size_t>(file_size);

  if (size > buffer->data.max_size()) {
    myfile.close();
    return -1;
  }

  try {
    buffer->data.resize(size);
  } catch (const std::bad_alloc&) {
    myfile.close();
    return -1;
  } catch (const std::length_error&) {
    myfile.close();
    return -1;
  }

   myfile.seekg(0, std::ios::beg);
  if (!myfile) {
    myfile.close();
    return -1;
  }

  if (file_size > 0) {
    myfile.read(
        reinterpret_cast<char*>(buffer->data.data()),
        static_cast<std::streamsize>(file_size));
    if (!myfile) {
      myfile.close();
      return -1;
    }
  }    
  myfile.close();
  if (static_cast<size_t>(id) < enigma::buffers.size()) {
    enigma::buffers[static_cast<size_t>(id)] = buffer.get();
  } else {	  
    try {
      enigma::buffers.push_back(buffer.get());
    } catch (const std::bad_alloc&) {
      return -1;
    } catch (const std::length_error&) {
      return -1;
    }
  }
  buffer.release();
  return id;
}

void buffer_load_ext(int buffer, string filename, unsigned offset) {
  get_buffer(binbuff, buffer);

  std::ifstream myfile(filename.c_str(), std::ios::binary);

  if (!myfile.is_open()) {
    DEBUG_MESSAGE("Unable to open file " + filename, MESSAGE_TYPE::M_ERROR);
    return;
  }

  myfile.seekg(0, std::ios::end);

  if (!myfile) {
    myfile.close();
    return;
  }

  const std::streampos file_position = myfile.tellg();

  if (file_position == std::streampos(-1)) {
    myfile.close();
    return;
  }

  const std::streamoff file_size =
      static_cast<std::streamoff>(file_position);

  if (file_size < 0 ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<unsigned>::max()) ||
      static_cast<std::uintmax_t>(file_size) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<std::streamsize>::max())) {
    myfile.close();
    return;
  }

  const size_t data_size = static_cast<size_t>(file_size);

  std::vector<unsigned char> data;

  if (data_size > data.max_size()) {
    myfile.close();
    return;
  }

  try {
    data.resize(data_size);
  } catch (const std::bad_alloc&) {
    myfile.close();
    return;
  } catch (const std::length_error&) {
    myfile.close();
    return;
  }

  myfile.seekg(0, std::ios::beg);
  if (!myfile) {
    myfile.close();
    return;
  }

  if (!data.empty()) {
    myfile.read(
        reinterpret_cast<char*>(data.data()),
        static_cast<std::streamsize>(file_size));

    if (!myfile) {
      myfile.close();
      return;
    }
  }

  myfile.close();
  if (data.empty())
    return;

  const size_t start = static_cast<size_t>(offset);

  if (binbuff->type == buffer_grow) {
    if (data_size >
        static_cast<size_t>(std::numeric_limits<unsigned>::max()) - start)
      return;

    const size_t required_size = start + data_size;

    if (required_size > binbuff->data.max_size())
      return;

    try {
      if (required_size > binbuff->data.size())
        binbuff->data.resize(required_size, 0);
    } catch (const std::bad_alloc&) {
      return;
    } catch (const std::length_error&) {
      return;
    }

    std::copy(
        data.begin(),
        data.end(),
        binbuff->data.begin() + start);
    return;
  }

  const size_t buffer_size = binbuff->data.size();

  if (buffer_size == 0)
    return;

  if (binbuff->type == buffer_wrap) {
    size_t position = start % buffer_size;

    for (const unsigned char byte : data) {
      binbuff->data[position] = byte;
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

  std::copy_n(
      data.begin(),
      writable,
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
    if (required_size > binbuff->data.max_size() ||
        required_size > std::numeric_limits<unsigned>::max())
      return;

    if (required_size > binbuff->data.size()) {
      try {
        binbuff->data.resize(required_size, 0);
      } catch (const std::bad_alloc&) {
        return;
      } catch (const std::length_error&) {
        return;
      }
    }
  }

  const size_t buffer_size = binbuff->data.size();
  if (buffer_size == 0)
    return;

  size_t writable = count;
  
  if (binbuff->type == buffer_wrap) {
	   size_t index = start % buffer_size;
	   
    for (size_t i = 0; i < writable; ++i) {
      binbuff->data[index] =
          static_cast<unsigned char>(value[i % type_size]);
          
      ++index;
      if (index == buffer_size)
        index = 0;      
    }
    
    return;
  }
  
  if (start >= buffer_size)
    return;
    
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

  // The returned pointer is invalidated when the buffer is deleted
  // or an operation reallocates its underlying vector storage.
  return static_cast<void *>(binbuff->data.data());
}

unsigned buffer_get_size(int buffer) {
  get_bufferr(binbuff, buffer, 0);
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

  const std::uint64_t bytes_per_pixel =
      static_cast<std::uint64_t>(buffer_sizeof(buffer_u64));
  const std::uint64_t width = static_cast<std::uint64_t>(wid);
  const std::uint64_t height = static_cast<std::uint64_t>(hgt);
  const std::uint64_t uint64_max =
      std::numeric_limits<std::uint64_t>::max();

  if (bytes_per_pixel == 0 || width == 0 ||
      height > uint64_max / width) {
    DEBUG_MESSAGE("Surface dimensions overflow!",
                  MESSAGE_TYPE::M_WARNING);
    return;
  }

  const std::uint64_t pixel_count = width * height;

  if (pixel_count > uint64_max / bytes_per_pixel) {
    DEBUG_MESSAGE("Surface buffer size overflow!",
                  MESSAGE_TYPE::M_WARNING);
    return;
  }

  const std::uint64_t expected_size =
      pixel_count * bytes_per_pixel;

  if (expected_size >
      static_cast<std::uint64_t>(
          std::numeric_limits<size_t>::max())) {
    DEBUG_MESSAGE("Surface buffer size is not representable!",
                  MESSAGE_TYPE::M_WARNING);
    return;
  }

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

  const unsigned current_size = binbuff->GetSize();

  switch (base) {
    case buffer_seek_start:
      binbuff->Seek(offset);
      break;
      
    case buffer_seek_end:
      if (offset > std::numeric_limits<unsigned>::max() - current_size)
        return;
      binbuff->Seek(current_size + offset);
      break;
      
    case buffer_seek_relative:
      if (offset > std::numeric_limits<unsigned>::max() - binbuff->position)
        return;   
      binbuff->Seek(binbuff->position + offset);
      break;

    default:
      return;      
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

  const size_t buffer_size = binbuff->data.size();
  const size_t start = static_cast<size_t>(offset);
  const unsigned saved_position = binbuff->position;
  if (buffer_size == 0)
    return type == buffer_string ? variant("") : variant(0); 

  const bool wrap = binbuff->type == buffer_wrap;

  if (!wrap && start >= buffer_size)
    return type == buffer_string ? variant("") : variant(0);

  if (type != buffer_string) {
    const unsigned type_size = buffer_sizeof(type); 

    if (type_size == 0)
      return 0;

    if (!wrap && type_size > buffer_size - start)
      return 0;

    std::uint64_t result = 0;
    size_t position = wrap ? start % buffer_size : start; 

    for (unsigned i = 0; i < type_size; ++i) {
      result |= static_cast<std::uint64_t>(
          binbuff->data[position]) << (i * 8);

      ++position;
      if (position == buffer_size)
        position = 0;
    }
 
    return result;
  }
 
  std::string data;
  size_t position = wrap ? start % buffer_size : start; 

  const size_t max_read = wrap
      ? buffer_size
      : buffer_size - start;

  try {
    for (size_t i = 0; i < max_read; ++i) {
      const unsigned char byte = binbuff->data[position]; 

    if (byte == 0)
      break; 

      data.push_back(static_cast<char>(byte));
      ++position;
      if (position == buffer_size)
        position = 0;
    }
  } catch (const std::bad_alloc&) {
    return variant("");
  } catch (const std::length_error&) {
    return variant("");
  }
  
  return variant(data);
}

variant buffer_read(int buffer, int type) {
  get_bufferr(binbuff, buffer, -1);

  const size_t buffer_size = binbuff->data.size();
  const size_t start = static_cast<size_t>(binbuff->position); 

  if (buffer_size == 0)
    return type == buffer_string ? variant("") : variant(0);

  const bool wrap = binbuff->type == buffer_wrap;
  const bool is_string = type == buffer_string;

  if (!is_string && buffer_sizeof(type) == 0)
    return 0;

  if (!wrap && start >= buffer_size)

    return is_string ? variant("") : variant(0); 

  const size_t position = wrap ? start % buffer_size : start;

  size_t consumed = 0;
  if (is_string) {
    const size_t limit = wrap
        ? buffer_size
        : buffer_size - position;

    size_t index = position;
    while (consumed < limit) {
      ++consumed;

      if (binbuff->data[index] == 0)
        break;
        
      ++index;
      if (index == buffer_size)
        index = 0;
    }

  } else {
    consumed = buffer_sizeof(type); 

    if (!wrap && consumed > buffer_size - position)
      return 0;
  }

  variant result = buffer_peek(
      buffer,
      static_cast<unsigned>(position),
      type
  );

  if (wrap) {
    const size_t advance = consumed % buffer_size;
    const size_t remaining = buffer_size - position;
    
    binbuff->position = static_cast<unsigned>(
        advance >= remaining
            ? advance - remaining
            : position + advance
    );

  } else {
    binbuff->position = static_cast<unsigned>(
        position + consumed
    );

  }

  return result;

}

void buffer_poke(int buffer, unsigned offset, int type, variant value) {
  get_buffer(binbuff, buffer);
  const unsigned type_size = buffer_sizeof(type);

  if (type != buffer_string && type_size == 0)
    return;

  if (binbuff->data.empty() &&
      binbuff->type != buffer_grow)
    return;

  size_t position = static_cast<size_t>(offset);
  const size_t max_size = binbuff->data.max_size();

  // Do not allow an offset that cannot be represented by the
  // buffer's public unsigned position.
  if (position > std::numeric_limits<unsigned>::max())
    return;

  const auto write_byte = [&](unsigned char byte) -> bool {
    const size_t buffer_size = binbuff->data.size();

    if (binbuff->type == buffer_wrap) {
      if (buffer_size == 0)
        return false;

      const size_t index = position % buffer_size;
      binbuff->data[index] = byte;
      position = (index + 1) % buffer_size;
      return true;
    }

    if (binbuff->type == buffer_grow) {
      if (position >= std::numeric_limits<unsigned>::max())
        return false;

      if (position >= max_size)
        return false;

      const size_t required_size = position + 1;
      if (required_size > binbuff->data.size()) {
        if (required_size > std::numeric_limits<unsigned>::max())
          return false;

        try {
          binbuff->data.resize(required_size, 0);
        } catch (const std::bad_alloc&) {
          return false;
        } catch (const std::length_error&) {
          return false;
        }
      }


      binbuff->data[position] = byte;
      ++position;
      return true;
    }

    if (position >= buffer_size)
      return false;

    binbuff->data[position] = byte;
    ++position;
    return true;
  };

  if (type == buffer_string) {
    const unsigned length =
        static_cast<unsigned>(value.string_length());

    for (unsigned i = 0; i <= length; ++i) {
      const unsigned char byte =
          static_cast<unsigned char>(value.char_at(i));

      if (!write_byte(byte))
        break;

      if (byte == 0)
        break;
    }

    const size_t alignment = binbuff->alignment;
    const size_t padding =
        alignment > 0 ? (alignment - ((length + 1) % alignment)) % alignment : 0;

    for (size_t i = 0; i < padding; ++i) {
      if (!write_byte(0))
        break;
    }
  } else {
    const std::vector<unsigned char> data =
        enigma::valToBytes(value, type_size);

    for (const unsigned char byte : data) {
      if (!write_byte(byte))
        break;
    }
  }

  binbuff->position = static_cast<unsigned>(position);
}

void buffer_write(int buffer, int type, variant value) {
  get_buffer(binbuff, buffer);
  buffer_poke(buffer, binbuff->position, type, value);
}

string buffer_md5(int buffer, unsigned offset, unsigned size) {
  //get_bufferr(binbuff, buffer, 0);
  //TODO: Write this function
  return {};
}

string buffer_sha1(int buffer, unsigned offset, unsigned size) {
  //get_bufferr(binbuff, buffer, 0);
  //TODO: Write this function
  return {};
}

namespace {

 

bool decode_base64(const std::string& input,
                   std::vector<unsigned char>& output) {
  static const char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  if (input.size() % 4 != 0)
    return false;

  const size_t groups = input.size() / 4;

  if (groups > output.max_size() / 3)
    return false;

  output.clear();

  try {
    output.reserve(groups * 3);

    for (size_t i = 0; i < input.size(); i += 4) {
      unsigned int values[4] = {};
      unsigned int padding = 0;

      for (unsigned int j = 0; j < 4; ++j) {
        const char c = input[i + j];

        if (c == '=') {
          if (j < 2)
            return false;

          ++padding;
          continue;
        }

        if (padding != 0)
          return false;

        const char* found = std::strchr(alphabet, c);

        if (!found)
          return false;

        values[j] =
            static_cast<unsigned int>(found - alphabet);
      }

      if (padding > 2)
        return false;

      const bool last_group = i + 4 == input.size();

      if (padding != 0 && !last_group)
        return false;

      if (padding == 1 && (values[2] & 0x03) != 0)
        return false;

      if (padding == 2 && (values[1] & 0x0F) != 0)
        return false;

      const unsigned int block =
          (values[0] << 18) |
          (values[1] << 12) |
          (values[2] << 6) |
          values[3];

      output.push_back(
          static_cast<unsigned char>((block >> 16) & 0xFF));

      if (padding < 2) {
        output.push_back(
            static_cast<unsigned char>((block >> 8) & 0xFF));
      }

      if (padding == 0) {
        output.push_back(
            static_cast<unsigned char>(block & 0xFF));
      }
    }
  } catch (const std::bad_alloc&) {
    output.clear();
    return false;

  } catch (const std::length_error&) {
    output.clear();
    return false;
  }

  return true;
}

}  // namespace

 

int buffer_base64_decode(string str) {
  std::vector<unsigned char> decoded;

  if (!decode_base64(str, decoded))
    return -1;

  const int id = enigma::get_free_buffer();

  if (id < 0)
    return -1;

  std::unique_ptr<enigma::BinaryBuffer> buffer;

  try {
    buffer.reset(new enigma::BinaryBuffer(0));
    buffer->type = buffer_grow;
    buffer->alignment = 1;
    buffer->data = std::move(decoded);

    const size_t index = static_cast<size_t>(id);

    if (index < enigma::buffers.size()) {
      enigma::buffers[index] = buffer.get();
    } else {
      enigma::buffers.push_back(buffer.get());
    }
  } catch (const std::bad_alloc&) {
    return -1;
  } catch (const std::length_error&) {
    return -1;
  }
  
  buffer.release();
  return id;
}

int buffer_base64_decode_ext(int buffer, string str, unsigned offset) {
  //get_bufferr(binbuff, buffer, -1);
  //TODO: Write this function
  return 0;
}

string buffer_base64_encode(int buffer, unsigned offset, unsigned size) {
  if (!enigma::buffer_valid(buffer))
    return {};

  const enigma::BinaryBuffer* binbuff =
      enigma::buffers[static_cast<size_t>(buffer)];
  const size_t buffer_size = binbuff->data.size();
  const size_t start = static_cast<size_t>(offset);

  if (start > buffer_size)
    return {};

  const size_t available = buffer_size - start;
  const size_t count = std::min(
      static_cast<size_t>(size), available);

  if (count > (std::string().max_size() - 2) / 4 * 3)
    return {};

  static const char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  std::string result;

  try {
    result.reserve(((count + 2) / 3) * 4);

    for (size_t i = 0; i < count; i += 3) {
      const unsigned int a = binbuff->data[start + i];
      const unsigned int b =
          i + 1 < count ? binbuff->data[start + i + 1] : 0;

      const unsigned int c =
          i + 2 < count ? binbuff->data[start + i + 2] : 0;

      const unsigned int block =
          (a << 16) | (b << 8) | c;

      result.push_back(alphabet[(block >> 18) & 0x3F]);
      result.push_back(alphabet[(block >> 12) & 0x3F]);

      result.push_back(
          i + 1 < count ? alphabet[(block >> 6) & 0x3F] : '=');

      result.push_back(
          i + 2 < count ? alphabet[block & 0x3F] : '=');
    }
  } catch (const std::bad_alloc&) {
    return {};
  } catch (const std::length_error&) {
    return {};
  }

  return result;
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
