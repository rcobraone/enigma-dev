#include "Audio_Systems/audio_mandatory.h"
#include "Universal_System/estring.h"
#include "Widget_Systems/widgets_mandatory.h"
#include <gme/gme.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using std::string;


typedef signed short sample_t;

struct gme_callback {
	Music_Emu *emu;
	int track;
};

size_t callback(gme_callback *userdata, void *buffer, size_t size) {
	Music_Emu *emu = userdata->emu;
	int track = userdata->track;

	if (!gme_track_ended(emu)) {
    const char *error =
        gme_play(emu, size / sizeof(sample_t), (sample_t*)buffer);
    if (error)
      DEBUG_MESSAGE(std::string("GME Error: ") + error,
                    MESSAGE_TYPE::M_ERROR);
		return size;
	}
	else {
    const char *error = gme_start_track(emu, track - 1);
    if (error)
      DEBUG_MESSAGE(std::string("GME Error: ") + error,
                    MESSAGE_TYPE::M_ERROR);
		return 0;
	}
}

void cleanup(gme_callback *userdata) {
	gme_delete(userdata->emu);
	delete userdata;
}

void seek(gme_callback *userdata, float position) {
  const char *error = gme_seek(userdata->emu, position * 1000);
  if (error)
    DEBUG_MESSAGE(std::string("GME Error: ") + error,
                  MESSAGE_TYPE::M_ERROR);;
}

namespace enigma_user {
int sound_add_from_gme(string filename, int track) {

	Music_Emu *emu = NULL;
    if (track < 0)
        return -1;

    if (track == 0)
        track = 1;

	if (!emu)
		return -1;

    const char* error = gme_start_track(emu, track - 1);
    if (error != NULL)
    {
        DEBUG_MESSAGE(
            std::string("GME Error: ") + error,
            MESSAGE_TYPE::M_ERROR
        );
        gme_delete(emu);
        return -1;
    }

	const int sid = enigma::sound_allocate();
	gme_callback *userdata = new gme_callback;
	userdata->emu = emu;
	userdata->track = track;

	return enigma::sound_add_from_stream(sid,(size_t (*)(void*, void*, size_t))callback,(void (*)(void*,float))seek,(void (*)(void*))cleanup,(void*)userdata)?-1:sid;
}
}

