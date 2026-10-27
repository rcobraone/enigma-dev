/** Copyright (C) 2008-2011 Josh Ventura
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

#include "libEGMstd.h"
#include "Widget_Systems/widgets_mandatory.h"
#include "zlib.h"

#include <string>
#include <memory>
#include <zlib.h>

namespace enigma {

unsigned char* zlib_compress(unsigned char* inbuffer,int actualsize)
{
if (actualsize < 0)
return nullptr;

if (!inbuffer && actualsize > 0)
return nullptr;

 Bytef empty_input = 0;
 const Bytef* input = inbuffer
     ? reinterpret_cast<const Bytef*>(inbuffer)
     : &empty_input;

 uLongf outsize = compressBound(static_cast<uLong>(actualsize));

 std::unique_ptr<Bytef[]> outbytef(new Bytef[outsize]);

    int res=compress(outbytef.get(),&outsize,
                     reinterpret_cast<const Bytef*>(inbuffer),
                     static_cast<uLong>(actualsize));

    if (res != Z_OK)
    {
     #if DEBUG_MODE
     if (res==Z_MEM_ERROR)
     DEBUG_MESSAGE("Zlib failed to compress the buffer. Out of memory.", MESSAGE_TYPE::M_ERROR);
     if (res==Z_BUF_ERROR)
     DEBUG_MESSAGE("Zlib failed to compress the buffer. Output size greater than allotted.", MESSAGE_TYPE::M_ERROR);
     #endif
     delete[] outbytef;
     return nullptr;     
    }

    return reinterpret_cast<unsigned char*>(outbytef.release());
}

int zlib_decompress(unsigned char* inbuffer, int insize, int uncompresssize,unsigned char* outbytef)
{
if (insize < 0 || uncompresssize < 0)
return -4;

if (!inbuffer && insize > 0)
return -4;

if (!outbytef && uncompresssize > 0)
return -4;

 Bytef empty_input = 0;
 Bytef empty_output = 0;
 const Bytef* input = inbuffer
     ? reinterpret_cast<const Bytef*>(inbuffer)
     : &empty_input;
 Bytef* output = outbytef ? reinterpret_cast<Bytef*>(outbytef)
                          : &empty_output;

uLongf outused=static_cast<uLongf>(uncompresssize);

    switch(uncompress(output,&outused,input,

    static_cast<uLong>(insize))){
	case Z_OK:return outused;
	case Z_MEM_ERROR:
		#if DEBUG_MODE
			DEBUG_MESSAGE("Zerror: Memory out", MESSAGE_TYPE::M_ERROR);
		#endif
		return -1;
	case Z_BUF_ERROR:
		#if DEBUG_MODE
			DEBUG_MESSAGE("Zerror: Output of " + toString(outused) + " above allotted " + toString(uncompresssize), MESSAGE_TYPE::M_ERROR);
		#endif
		return -2;
	case Z_DATA_ERROR:
		#if DEBUG_MODE
			DEBUG_MESSAGE("Zerror: Invalid data", MESSAGE_TYPE::M_ERROR);
		#endif
		return -3;
	default:return -4;
	}
}

}  //namespace enigma
