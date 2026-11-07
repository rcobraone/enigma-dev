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
#include <limits>
#include <cstddef>
#include <utility>


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
          : nullptr;
	
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
        live->second &&
        live->second->inst == inst) {
      enigma::instance_deactivated_list.insert(inode_pair(id, inst));
    }

    if (!moved && it) {
      ++it;
    }

  }
}

void instance_activate_all() {

    auto iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
       
        enigma::object_basic* const inst = iter->second;

        const auto live = enigma::instance_list.find(id);
        if (!inst || live == enigma::instance_list.end() ||
            !live->second || live->second->inst != inst) {
            iter = enigma::instance_deactivated_list.erase(iter);
            continue;
        }
        
        if (!inst) {
            iter = enigma::instance_deactivated_list.erase(iter);
            continue;

        }
        
        std::map<int,enigma::object_basic*>::const_iterator next = iter;
        ++next;
        const bool has_next =
            next != enigma::instance_deactivated_list.end();
        const int next_id = has_next ? next->first : 0;

        if (!inst) {
            enigma::instance_deactivated_list.erase(iter);
            iter = has_next
                ? enigma::instance_deactivated_list.lower_bound(next_id)
                : enigma::instance_deactivated_list.end();
            continue;
        }

        enigma::instance_deactivated_list.erase(iter);
        inst->activate();

        iter = has_next
            ? enigma::instance_deactivated_list.lower_bound(next_id)
            : enigma::instance_deactivated_list.end();       
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
        live->second &&
        live->second->inst == inst) {
      enigma::instance_deactivated_list.insert(inode_pair(id, inst));
    }

    if (!moved && it) {
      ++it;
    }
  }
}

void instance_activate_object(int obj) {
    auto iter = enigma::instance_deactivated_list.begin();
    while (iter != enigma::instance_deactivated_list.end()) {
		
        enigma::object_basic* const inst = iter->second;

        const auto live = enigma::instance_list.find(id);
        if (!inst || live == enigma::instance_list.end() ||
            !live->second || live->second->inst != inst) {
            iter = enigma::instance_deactivated_list.erase(iter);
            continue;
        }
        
        if (!inst) {
            iter = enigma::instance_deactivated_list.erase(iter);
            continue;
        }
       
            std::map<int,enigma::object_basic*>::const_iterator next = iter;
            ++next;
            const bool has_next =
                next != enigma::instance_deactivated_list.end();
            const int next_id = has_next ? next->first : 0;

        if (!inst) {
            enigma::instance_deactivated_list.erase(iter);
            iter = has_next
                ? enigma::instance_deactivated_list.lower_bound(next_id)
                : enigma::instance_deactivated_list.end();
            continue;
        }

        if (obj == all ||
            (obj < 100000
                 ? (inst->object_index == obj || inst->can_cast(obj))
                 : inst->id == static_cast<unsigned>(obj))) {
            enigma::instance_deactivated_list.erase(iter);             
            inst->activate();

            iter = has_next
                ? enigma::instance_deactivated_list.lower_bound(next_id)
                : enigma::instance_deactivated_list.end();
        }
        else {
            ++iter;
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
          : nullptr;

  if (!a)
    return;

  if (enigma::cleanups.find(a) != enigma::cleanups.end())
    return;

  a->myevent_destroy();

  if (enigma::cleanups.find(a) == enigma::cleanups.end())
    a->unlink();

  if (enigma::cleanups.find(a) == enigma::cleanups.end())
  {
    DEBUG_MESSAGE(
        "instance_destroy(): instance was not scheduled for cleanup.",
        MESSAGE_TYPE::M_ERROR
    );
  }

  if (!enigma::instance_event_iterator ||
      a != enigma::instance_event_iterator->inst)
  {
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
  if (num < 0)
    return noone;
	
  std::size_t nth = 0;
  const std::size_t target = static_cast<std::size_t>(num);

  for (enigma::iterator it = enigma::fetch_inst_iter_by_int(obj); it; ++it)
  {
    if (nth == target)
      return static_cast<enigma::instance_t>(it->id);

    ++nth;
  }
  
  return noone;
}

enigma::instance_t instance_last(int obj) {
  if (obj < 0 ||
      static_cast<std::size_t>(obj) >= enigma::object_idmax) {
    return noone;
  }

  const auto& object = enigma::objects[obj];

  if (object.count == 0 || !object.prev || !object.prev->inst)
    return noone;

  return static_cast<enigma::instance_t>(object.prev->inst->id);
}

int instance_number(int obj)
{
  switch (obj) {
    case self:
    case other:
      return (bool) enigma::fetch_inst_iter_by_int(obj);
    case all:
      {
        const std::size_t count = enigma::instance_list.size();
        const std::size_t max_count =
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            );
        return count > max_count
            ? std::numeric_limits<int>::max()
            : static_cast<int>(count);
      }
    default:
       if (obj < 0) return 0;
       if (static_cast<std::size_t>(obj) >= enigma::object_idmax)
        return 0;
  }
  return instance_count_to_int(enigma::objects[obj].count);
}

}
