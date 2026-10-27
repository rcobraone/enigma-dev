/** Copyright (C) 2010-2011 Josh Ventura
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

#include "Audio_Systems/audio_mandatory.h"
#include "Platforms/platforms_mandatory.h"
#include "Widget_Systems/widgets_mandatory.h"
#include "libEGMstd.h"
#include "resinit.h"
#include "Universal_System/zlib.h"
#include "Platforms/General/fileio.h"

#include <cstring>
#include <new>
#include <vector>

namespace enigma_user {
  void sound_play(int sound);
}

namespace enigma
{
  void sound_safety_override()
  {

  }

  void exe_loadsounds(FILE_t *exe)
  {
    char header[4];

    if (fread_wrapper(header,1,sizeof(header),exe) != sizeof(header))
      return;
    if (memcmp(header, "SND ", sizeof(header)) != 0)
      return;

    // Determine how many sprites we have
    int sndcount;
    if (!fread_wrapper(&sndcount,4,1,exe)) return;

    // Fetch the highest ID we will be using
    int snd_highid;
    if (!fread_wrapper(&snd_highid,4,1,exe)) return;
    if (sndcount < 0 || snd_highid < 0) return;
    if (sndcount == 0) return;    

    for (int i = 0; i < sndcount; i++)
    {
      int id;
      if (!fread_wrapper(&id,4,1,exe)) return;
      if (id < 0 || id > snd_highid) return;

      unsigned size;
      if (!fread_wrapper(&size,4,1,exe)) return;

      std::vector<char> fdata;
      try {
        fdata.resize(size);
      } catch (const std::bad_alloc&) {
        DEBUG_MESSAGE("Failed to load sound: Cannot allocate enough memory",
                      MESSAGE_TYPE::M_ERROR);
        return;
      } catch (const std::length_error&) {
        DEBUG_MESSAGE("Failed to load sound: Invalid data size",
                      MESSAGE_TYPE::M_ERROR);
        return;
      }


      const size_t read = fread_wrapper(fdata.data(),1,size,exe);
      if (read != size) {
        DEBUG_MESSAGE("Failed to load sound: Data is truncated",
                      MESSAGE_TYPE::M_ERROR);
        return;
      }

      int e = sound_add_from_buffer(id, fdata.data(), size);
      if (e) DEBUG_MESSAGE("Failed to load sound " + std::to_string(i) + " error " + std::to_string(e), MESSAGE_TYPE::M_ERROR);

    }
  }
}
