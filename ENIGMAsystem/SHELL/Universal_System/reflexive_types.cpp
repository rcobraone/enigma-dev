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

#include <cmath>
#include <cstdlib>
#include <string>
#include "var4.h"
#include "reflexive_types.h"
#include "math_consts.h"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

// Variable not zero.
inline bool varnz(double x) { return std::fabs(x) > variant::epsilon; }

namespace enigma {
  //Make direction work
  void directionv::function(const variant&) {
    if (!std::isfinite(rval.d)) {
      return;
    }
    rval.d = std::fmod(rval.d, 360.0);
    if(rval.d < 0.0){
      rval.d += 360.0;
    }
    *hspd = *spd * std::cos(rval.d * kPi / 180.0);
    *vspd = *spd * -std::sin(rval.d * kPi / 180.0);
  }

  //Make speed work -- same as above, but rval.d and reflex1 are switched.
  void speedv::function(const variant&) {
    if (!std::isfinite(rval.d) || !std::isfinite(*dir)) {
      return;
    }
  
    *hspd = rval.d * std::cos(*dir * kPi / 180.0);
    *vspd = rval.d * -std::sin(*dir * kPi / 180.0);
  }

  //Make hspeed work
  void hspeedv::function(const variant &oldval) {
    if (rval.d == oldval.rval.d)
    {
      return;
    }
    else if (varnz(rval.d) || varnz(*vspd))
    {
      const double angle = std::atan2(-(*vspd), rval.d);
      if (!std::isfinite(angle)) return;

      double direction = angle * 180.0 / kPi;
      if (direction < 0.0) direction += 360.0;

      *dir = static_cast<int>(std::lround(direction)) % 360;
      *spd = std::hypot(rval.d, *vspd);
    }
    else
    {
      *spd = 0.0;
    }
  }

  //Make vspeed work -- Same as above, except the arguments to atan2 are reversed
  void vspeedv::function(const variant &oldval) {
    if (rval.d == oldval.rval.d)
    {
      return;
    }
    else if (varnz(rval.d) || varnz(*hspd))
    {
      const double angle = std::atan2(-rval.d, *hspd);
      if (!std::isfinite(angle)) return;

      double direction = angle * 180.0 / kPi;
      if (direction < 0.0) direction += 360.0;

      *dir = static_cast<int>(std::lround(direction)) % 360;
      *spd = std::hypot(*hspd, rval.d);
    }
    else
    {
      *spd = 0.0;
    }
  }
}
