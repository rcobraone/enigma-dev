#include "include.h"

#include "Platforms/xlib/XLIBwindow.h"

#include <X11/extensions/XTest.h>

namespace enigma {
  extern unsigned short keyrmap[256];
}

namespace enigma_user
{

void keyboard_key_press(int key) {
  if (key < 0 || key >= 256)
    return;

  key = XKeysymToKeycode(enigma::x11::disp, enigma::keyrmap[key]);
  if (key == 0)
    return;
 
  XTestFakeKeyEvent(enigma::x11::disp, key, True, 0);
}

void keyboard_key_release(int key) {
  if (key < 0 || key >= 256)
    return;
  
  key = XKeysymToKeycode(enigma::x11::disp, enigma::keyrmap[key]);
  if (key == 0)
    return;
   
  XTestFakeKeyEvent(enigma::x11::disp, key, False, 0); 
}

}
