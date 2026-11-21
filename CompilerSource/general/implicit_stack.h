/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
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

#include <stdlib.h>
#include <limits>
#include <new>
#include <cstring>

template <typename atype>
struct implicit_stack
{
  atype* safety;
  atype** astack;
  unsigned int ind, allocd;
  void push()
  {
    ind++;
    if (allocd<=ind)
    {
      unsigned int olds=allocd;
      while (allocd <= ind)
      {
        if (allocd > std::numeric_limits<unsigned int>::max() / 2)
        {
          ind--;
          return;
        }
        allocd <<= 1;
      }

      atype** nar=new (std::nothrow) atype*[allocd];
      if (nar==0)
      {
        ind--;
        allocd=olds;
        return;
      }
      if (olds>0)
      {
        std::memcpy(nar,astack,olds*sizeof(atype*));
        for (unsigned int i=olds;i<allocd;i++) nar[i]=0;
      }

      delete []astack;
      astack=nar;
    }
    if (astack[ind]==0) astack[ind]=new atype;
    if (astack[ind]==0) ind--;
  }
  void pop()
  {
    if (ind == 0)
      return;
	  
    delete astack[ind];
    astack[ind]=0;
    ind--;
  }

  atype &operator() ()
  {
    return *astack[ind];
  }

  implicit_stack()
  {
    ind=0;
    safety = new (std::nothrow) atype;
    astack = new (std::nothrow) atype*[1];
   allocd=astack!=NULL;
    if (safety == NULL || allocd == 0)
      exit(-18);
    astack[0] = new (std::nothrow) atype;
    if (astack[0] == NULL)
      exit(-18);
  }
  ~implicit_stack()
  {
    if (astack != 0)
    {
      for (unsigned int i=0; i<=ind; i++)
        delete astack[i];
      delete [] astack;
    }
    delete safety;
  }
};
