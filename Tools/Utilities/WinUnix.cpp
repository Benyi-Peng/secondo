/*

September 2003, M. Spiekermann: Implementation of getpagesize()

*/

#include <unistd.h>
#include <SecondoConfig.h>

#ifdef SECONDO_WIN32
#include <winbase.h>
#endif

#include "WinUnix.h"

int
WinUnix::getPageSize( void ) { 

#ifndef SECONDO_WIN32
   return ( getpagesize() );
#else
   LPSYSTEM_INFO lpSysInf;
   GetSystemInfo( lpSysInf );
   return ( lpSysInfo->dwPageSize );
#endif

}
