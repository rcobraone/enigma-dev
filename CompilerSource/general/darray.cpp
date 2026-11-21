//This file is entirely for debug.
//Same GPL copyright applies as in header.

#include <iostream>
using namespace std;
void post_access_watch(unsigned i,unsigned sz,unsigned all)
{
  if (i > sz)
    cout <<
    "***************************************************\n" <<
    "Though this is not a fatal error, its occurrance is\n" <<
    "potentially dangerous. Please report the occurrence\n" <<
    "***************************************************\n";
}
