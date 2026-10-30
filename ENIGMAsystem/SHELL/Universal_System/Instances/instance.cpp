/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
**  Copyright (C) 2012 Alasdair Morrison                                        **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/


#include "Universal_System/Object_Tiers/object.h"
#include "Widget_Systems/widgets_mandatory.h"
#include "instance_system.h"
#include "instance.h"

#include <map>
#include <string>
#include <sstream>
#include <math.h>
#include <stdio.h>

namespace {
 inline std::string pointer2string(void* ptr) {
   std::stringstream ss;
   ss << ptr;  
   return  ss.str(); 
 }
}

namespace enigma
{
  int destroycalls = 0, createcalls = 0;
}

typedef std::pair<int,enigma::object_basic*> inode_pair;

namespace enigma_user
{

void instance_deactivate_all(bool notme) {
  enigma::object_basic *self =
      enigma::instance_event_iterator
          ? enigma::instance_event_iterator->inst
          : NULL;
	
  enigma::iterator it = enigma::instance_list_first();
  while (it) {
    enigma::object_basic* const inst = *it;

    if (notme && self && inst == self) {
      ++it;
      continue;
    }

    const int id = inst->id;
    inst->deactivate();

    // unlink() updates registered iterators when it removes a node.
    const bool moved = it && *it != inst;

    // Do not register an instance that was removed during deactivate().
    const auto live = enigma::instance_list.find(id);
    if (live != enigma::instance_list.end() &&
        live->second->inst == inst) {
      enigma::instance_deactivated_list.insert(inode_pair(id, inst));
    }

    if (!moved && it) {
      ++it;
    }

  }
}

void instance_activate_all() {

    std::map<int,enigma::object_basic*>::iterator iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
        enigma::object_basic *inst = iter->second;
        std::map<int,enigma::object_basic*>::iterator next = iter;
        ++next;
        enigma::instance_deactivated_list.erase(iter);
        iter = next;
        inst->activate();
    }
}

void instance_deactivate_object(int obj) {
  enigma::iterator it = enigma::fetch_inst_iter_by_int(obj);
  while (it) {
    enigma::object_basic* const inst = *it;
    const int id = inst->id;

    inst->deactivate();

    const bool moved = it && *it != inst;

    const auto live = enigma::instance_list.find(id);
    if (live != enigma::instance_list.end() &&
        live->second->inst == inst) {
      enigma::instance_deactivated_list.insert(inode_pair(id, inst));
    }

    if (!moved && it) {
      ++it;
    }
  }
}

void instance_activate_object(int obj) {
    std::map<int,enigma::object_basic*>::iterator iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
        enigma::object_basic* const inst = iter->second;
        if (obj == all || (obj < 100000 ? (inst->object_index==obj || inst->can_cast(obj)) : inst->id == unsigned(obj))) {
            std::map<int,enigma::object_basic*>::iterator next = iter;
            ++next;
            enigma::instance_deactivated_list.erase(iter);
            iter = next;   
            inst->activate();
        }
        else {
            iter++;
        }
    }
}

void instance_destroy(int id, bool dest_ev)
{
  enigma::iterator it = enigma::fetch_inst_iter_by_int(id);
  while (it) {
    enigma::object_basic* who = (*it);
    if (enigma::cleanups.find(who) == enigma::cleanups.end()) {
      if (dest_ev)
          who->myevent_destroy();
      if (enigma::cleanups.find(who) != enigma::cleanups.end())
        {
          if (it && *it == who)
            ++it;
     
        continue;
	 } 
          who->unlink();
        // unlink() moves iterators to the next instance.
        continue;     
    }
    
    ++it;
  }
}

void instance_destroy()
{
  enigma::object_basic* const a =
      enigma::instance_event_iterator
          ? enigma::instance_event_iterator->inst
          : NULL;

  if (!a)
    return;

  if (enigma::cleanups.find(a) == enigma::cleanups.end()) {
    a->myevent_destroy();
    if (enigma::cleanups.find(a) == enigma::cleanups.end())
        a->unlink();
  if (enigma::cleanups.find(a) == enigma::cleanups.end())
    DEBUG_MESSAGE(
        "instance_destroy(): instance was not scheduled for cleanup.",
        MESSAGE_TYPE::M_ERROR
    );
    if (!enigma::instance_event_iterator ||
    if (a != (enigma::instance_event_iterator
                  ? enigma::instance_event_iterator->inst : NULL))
    DEBUG_MESSAGE(
        "instance_destroy(): event iterator changed unexpectedly.",
        MESSAGE_TYPE::M_ERROR
    );
  }
}

bool instance_exists(int obj)
{
  return enigma::fetch_instance_by_int(obj) != NULL;
}

enigma::instance_t instance_find(int obj, int num)
{
  int nth = 0;
  for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it)
  {
    if (nth == num)
      return (int)it->id;
    ++nth;
  }
  return noone;
}

enigma::instance_t instance_last(int obj) {
  if (obj < 0 ||
      static_cast<size_t>(obj) >= enigma::object_idmax) {
    return noone;
  }

  return (enigma::objects[obj].count > 0)
      ? static_cast<int>(enigma::objects[obj].prev->inst->id)
      : noone;
}

int instance_number(int obj)
{
  switch (obj) {
    case self:
    case other:
      return (bool) enigma::fetch_inst_iter_by_int(obj);
    case all:
      return enigma::instance_list.size();
    default:
       if (obj < 0) return 0;
       if (static_cast<size_t>(obj) >= enigma::object_idmax)
        return 0;
  }
  return enigma::objects[obj].count;
}

}
