/** Copyright (C) 2013 forthevin
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

#include <list>

namespace enigma {
  using std::list;
  typedef void (*callback_t )();

  // Before collision event.

  list<callback_t> before_collision_callbacks;
  void perform_callbacks_before_collision_event() {

    const list<callback_t> callbacks = before_collision_callbacks;
    for (callback_t callback : callbacks)
      if (callback) callback();   
  }
  void register_callback_before_collision_event(callback_t callback) {
	if (callback)
    before_collision_callbacks.push_back(callback);
  }

  // Particle updating.

  list<callback_t> particle_updating_callbacks;
  void perform_callbacks_particle_updating() {

    const list<callback_t> callbacks = particle_updating_callbacks;
    for (callback_t callback : callbacks)
      if (callback) callback();  
  }
  void register_callback_particle_updating(callback_t callback) {
	if (callback)
    particle_updating_callbacks.push_back(callback);
  }

  // Clean up room-end.
  list<callback_t> clean_up_roomend_callbacks;
  void perform_callbacks_clean_up_roomend() {
    const list<callback_t> callbacks = clean_up_roomend_callbacks;
    for (callback_t callback : callbacks)
      if (callback) callback();
  }
  void register_callback_clean_up_roomend(callback_t callback) {
	if (callback)
    clean_up_roomend_callbacks.push_back(callback);
  }
}

