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
  if (!std::isfinite(rval.d) || !std::isfinite(*spd)) {
    return;
  }
   rval.d = std::fmod(rval.d, 360.0);
  if (rval.d < 0.0) {
    rval.d += 360.0;
  }
  const double angle = rval.d * kPi / 180.0;
  const double new_hspd = *spd * std::cos(angle);
  const double new_vspd = -*spd * std::sin(angle);+
  if (!std::isfinite(new_hspd) || !std::isfinite(new_vspd)) {
    return;
  }

  *hspd = new_hspd;
  *vspd = new_vspd;
  }

  //Make speed work -- same as above, but rval.d and reflex1 are switched.
  void speedv::function(const variant&) {
  if (!std::isfinite(rval.d) || !std::isfinite(*dir)) {
    return;
  }

  const double angle = *dir * kPi / 180.0;
  const double new_hspd = rval.d * std::cos(angle);
  const double new_vspd = -rval.d * std::sin(angle);

  if (!std::isfinite(new_hspd) || !std::isfinite(new_vspd)) {
    return;
  }

  *hspd = new_hspd;
  *vspd = new_vspd;
  }

  //Make hspeed work
  void hspeedv::function(const variant &oldval) {
  if (!std::isfinite(rval.d) || !std::isfinite(*vspd)) {
    return;
  }

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

    const double new_speed = std::hypot(rval.d, *vspd);
    if (!std::isfinite(new_speed)) return;
    *dir = static_cast<int>(std::lround(direction)) % 360;

    *spd = new_speed;
  
    }
    else
    {
      *spd = 0.0;
    }
  }

  //Make vspeed work -- Same as above, except the arguments to atan2 are reversed
  void vspeedv::function(const variant &oldval) {
  if (!std::isfinite(rval.d) || !std::isfinite(*hspd)) {
    return;
  }

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

     const double new_speed = std::hypot(*hspd, rval.d);
     if (!std::isfinite(new_speed)) return;

     *dir = static_cast<int>(std::lround(direction)) % 360;
     *spd = new_speed;
  
    }
    else
    {
      *spd = 0.0;
    }
  }
}
