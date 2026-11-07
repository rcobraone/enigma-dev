#include "include.h"

#include "Platforms/xlib/XLIBwindow.h"

#include <X11/extensions/Xrandr.h> // for display_set_size

namespace {

void query_size_id(int w, int h, SizeID &compatible_size_id) {
  compatible_size_id = -1;
  int num_sizes;
  XRRScreenSize *xrrs;
  xrrs = XRRSizes(enigma::x11::disp, 0, &num_sizes);
 if (!xrrs || num_sizes <= 0)
    return;  
  
  for (int i = 0; i < num_sizes; i++) {
    if (xrrs[i].width == w && xrrs[i].height == h) {
      compatible_size_id = i;
      return;
    }
  }
}

bool test_size_id(int &w, int &h, SizeID &compatible_size_id, SizeID original_size_id) {
  if (w == -1 && h == -1) {
    compatible_size_id = original_size_id;
  } else {
    if (w == -1) {
      w = XWidthOfScreen(enigma::x11::screen);
    }
    if (h == -1) {
      h = XHeightOfScreen(enigma::x11::screen);
    }
    query_size_id(w, h, compatible_size_id);
    if (compatible_size_id == -1)
      return false;
  }
  return true;
}

} // namespace anonymous

namespace enigma_user {

int display_get_frequency() {
  XRRScreenConfiguration *conf = XRRGetScreenInfo(enigma::x11::disp, enigma::x11::win);
  if (!conf)
    return -1;
  const int frequency = XRRConfigCurrentRate(conf);
  XRRFreeScreenConfigInfo(conf);
  return frequency;
}

bool display_set_size(int w, int h) {
  XRRScreenConfiguration *conf = XRRGetScreenInfo(enigma::x11::disp, enigma::x11::win);
  if (!conf)
    return false;
  Rotation original_rotation;
  SizeID original_size_id = XRRConfigCurrentConfiguration(conf, &original_rotation);
  SizeID compatible_size_id = -1;

  query_size_id(w, h, compatible_size_id);
  if (compatible_size_id == -1) {
    XRRFreeScreenConfigInfo(conf);
     return false;
  }
    return false;

  const Status status = XRRSetScreenConfig(enigma::x11::disp, conf, enigma::x11::win,
                                           compatible_size_id, original_rotation, CurrentTime);
  XRRFreeScreenConfigInfo(conf);
  return status;
}

bool display_set_frequency(int freq) {
  XRRScreenConfiguration *conf = XRRGetScreenInfo(enigma::x11::disp, enigma::x11::win);
  if (!conf)
    return false;
  Rotation original_rotation;
  SizeID original_size_id = XRRConfigCurrentConfiguration(conf, &original_rotation);

  const Status status = XRRSetScreenConfigAndRate(enigma::x11::disp, conf, enigma::x11::win,
                                                  original_size_id, original_rotation, freq, CurrentTime);
  XRRFreeScreenConfigInfo(conf);
  return status;
}

bool display_set_all(int w, int h, int freq, int bitdepth) {
  XRRScreenConfiguration *conf = XRRGetScreenInfo(enigma::x11::disp, enigma::x11::win);
  if (!conf)
    return false;  
  Rotation original_rotation;
  SizeID original_size_id = XRRConfigCurrentConfiguration(conf, &original_rotation);
  SizeID compatible_size_id = -1;

  if (!test_size_id(w, h, compatible_size_id, original_size_id)) {
    XRRFreeScreenConfigInfo(conf);
    return false;
  }
  Status status;

  if (freq != -1) {
    status = XRRSetScreenConfigAndRate(enigma::x11::disp, conf, enigma::x11::win,
 
  } else {
    status = XRRSetScreenConfig(enigma::x11::disp, conf, enigma::x11::win,

  }
  XRRFreeScreenConfigInfo(conf);
  return status;
}

bool display_test_all(int w, int h, int freq, int bitdepth) {
  XRRScreenConfiguration *conf = XRRGetScreenInfo(enigma::x11::disp, enigma::x11::win);
  if (!conf)
    return false; 
  Rotation original_rotation;
  SizeID original_size_id = XRRConfigCurrentConfiguration(conf, &original_rotation);
  SizeID compatible_size_id = -1;

  if (!test_size_id(w, h, compatible_size_id, original_size_id)) {
    XRRFreeScreenConfigInfo(conf);
    return false;
  }

  if (freq != -1) {
    int num_rates;
    short *xrrr;
    xrrr = XRRRates(enigma::x11::disp, 0, compatible_size_id, &num_rates);
    if (!xrrr || num_rates <= 0) {
      XRRFreeScreenConfigInfo(conf);
      return false;
    }
    bool rateSupported = false;
    for (int i = 0; i < num_rates; i++) {
      if (xrrr[i] == freq) {
        rateSupported = true;
        break;
      }
    }
    if (!rateSupported) {
      XRRFreeScreenConfigInfo(conf);
      return false;
    }
  }
  XRRFreeScreenConfigInfo(conf);
  return true;
}

} // namespace enigma_user
