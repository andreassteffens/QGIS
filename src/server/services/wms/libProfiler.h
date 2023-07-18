///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ___        __       ____                  ___      ___
// /\_ \    __/\ \     /\  _`\              /'___\ __ /\_ \
// \//\ \  /\_\ \ \____\ \ \L\ \_ __   ___ /\ \__//\_\\//\ \      __   _ __
//   \ \ \ \/\ \ \ '__`\\ \ ,__/\`'__\/ __`\ \ ,__\/\ \ \ \ \   /'__`\/\`'__\
//    \_\ \_\ \ \ \ \L\ \\ \ \/\ \ \//\ \L\ \ \ \_/\ \ \ \_\ \_/\  __/\ \ \/
//    /\____\\ \_\ \_,__/ \ \_\ \ \_\\ \____/\ \_\  \ \_\/\____\ \____\\ \_\
//    \/____/ \/_/\/___/   \/_/  \/_/ \/___/  \/_/   \/_/\/____/\/____/ \/_/
//
//
//  Copyright (C) 2007-2013 Cedric Guillemet
//
//  Contact  : cedric.guillemet@gmail.com
//  Twitter  : http://twitter.com/skaven_
//  Web home : http://skaven.fr
//
//  Made with the great help of Christophe Giraud and Maxime Houlier.
//
//    libProfiler is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    libProfiler is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with libProfiler.  If not, see <http://www.gnu.org/licenses/>
//
//
// Changelog:
// 23/12/12 : Initial release
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// libProfiler is a tool to measure time taken by a code portion. It can help you improve code performance.
// It can be easily integrated in your toolchain, continuous integration,...
// It's implemented as only one C++ header file!
// As you define the granularity, it may be more usefull than great tools like Verysleepy. And it
// works well with debug info turned off and full optmisation turned on.
// Sadly, it uses STL (nobody's perfect)
//
// How to use it:
//
// Include this header. For one of .cpp where it's included add:
// #define LIB_PROFILER_IMPLEMENTATION
//
// In your project preprocessor, define USE_PROFILER. If USE_PROFILER is not defined, every
// libProfiler macro is defined empty. So depending on your project target, you can enable/disable
// profiling.
//
// Let's see an example code :
//
//
// #include <iostream>
// #include <math.h>
//
// void myPrintf( const char *szText )
// {
//     printf("Profiler:%s", szText);
// }
//
//
// #define USE_PROFILER 1
// #define LIB_PROFILER_IMPLEMENTATION
// #define LIB_PROFILER_PRINTF myPrintf
// #include "libProfiler.h"
//
//
// void myFunction()
// {
//     PROFILER_START(myFunction);
//     float v = 0;
//     for(int i = 0;i<1000000;i++)
//         v += cosf(static_cast<float>(rand()));
//
//     printf("v = %5.4f\n", v);
//     PROFILER_END();
// }
//
// int main(int argc, const char * argv[])
// {
//     PROFILER_ENABLE;
//
//     PROFILER_START(Main);
//
//     std::cout << "Hello, World!\n";
//     myFunction();
//     myFunction();
//
//     PROFILER_END();
//
//     LogProfiler();
//
//     PROFILER_DISABLE;
//
//     return 0;
// }
//
//
//
// Enable/disable profiling with USE_PROFILER.
// In one of your .cpp file, define LIB_PROFILER_IMPLEMENTATION or you'll have troubles linking.
// You can overide the default printf output redefining preprocessor LIB_PROFILER_PRINTF.
// The sample will output:
//
// Hello, World!
// v = -1530.3564
// v = -190.7513
// Profiler:CALLSTACK of Thread 0
// Profiler:_______________________________________________________________________________________
// Profiler:| Total time   | Avg Time     |  Min time    |  Max time    | Calls  | Section
// Profiler:_______________________________________________________________________________________
// Profiler:|      79.0000 |      79.0000 |      79.0000 |      79.0000 |     1  | Main
// Profiler:|      79.0000 |      39.5000 |      38.0000 |      41.0000 |     2  |   myFunction
// Profiler:_______________________________________________________________________________________
//
// Profiler:
//
// Profiler:DUMP of Thread 0
// Profiler:_______________________________________________________________________________________
// Profiler:| Total time   | Avg Time     |  Min time    |  Max time    | Calls  | Section
// Profiler:_______________________________________________________________________________________
// Profiler:|      79.0000 |      79.0000 |      79.0000 |      79.0000 |      1 | Main
// Profiler:|      79.0000 |      39.5000 |      38.0000 |      41.0000 |      2 | myFunction
// Profiler:_______________________________________________________________________________________
//
// The first list correspond to the callstack ( with left spaced function name). You might see a
// a profiled block multiple time depending on where it was called.
// The second list is a flat one. Profiled block only appear once.
// Time unit is ms
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LIBPROFILER_H__
#define LIBPROFILER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
// includes

#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <map>
#include <string>

#if USE_PROFILER
#include <Windows.h>
#endif

#include "qgswmsutils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PROFILE/LOG

#ifndef LIB_PROFILER_PRINTF
#define LIB_PROFILER_PRINTF QgsWms::sbProfilerOutput
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// OS definition

#define PLATFORM_OS_WINDOWS    1
#define PLATFORM_OS_LINUX      2
#define PLATFORM_OS_MACOSX     3

//NOTE: all predefine C/C++ compiler macros: http://sourceforge.net/apps/mediawiki/predef/

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( __WIN64__ ) || defined( _WIN64 ) || defined( WIN32 )
#   define IS_OS_WINDOWS    1
#   define IS_OS_LINUX      0
#   define IS_OS_MACOSX     0
#   define PLATFORM_OS      PLATFORM_OS_WINDOWS
#   pragma message("Platform OS is Windows.")
#elif defined(__linux__) || defined( LINUX )
#   define IS_OS_WINDOWS    0
#   define IS_OS_LINUX      1
#   define IS_OS_MACOSX     0
#   define PLATFORM_OS      PLATFORM_OS_LINUX
#   pragma message("Platform OS is Linux.")
#elif ( defined(__APPLE__) && defined(__MACH__) )  || defined( MACOSX )
#   define IS_OS_WINDOWS    0
#   define IS_OS_LINUX      0
#   define IS_OS_MACOSX     1
#   define PLATFORM_OS      PLATFORM_OS_MACOSX
#   pragma message("Platform OS is MacOSX.")
#else
#   error "This platform is not supported."
#endif


#define PLATFORM_COMPILER_MSVC  1
#define PLATFORM_COMPILER_GCC   2

#if defined( _MSC_VER )
#   define PLATFORM_COMPILER            PLATFORM_COMPILER_MSVC
#   define PLATFORM_COMPILER_VERSION    _MSC_VER
#   define IS_COMPILER_MSVC     1
#   define IS_COMPILER_GCC      0
#   pragma message("Platform Compiler is Microsoft Visual C++.")
#elif defined( __GNUC__ )
#   define PLATFORM_COMPILER            PLATFORM_COMPILER_GCC
#   define PLATFORM_COMPILER_VERSION    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#   define IS_COMPILER_MSVC     0
#   define IS_COMPILER_GCC      1
#   pragma message("Platform Compiler is GCC.")
#else
#   error "This compiler is not supported."
#endif


#define PLATFORM_MEMORY_ADDRESS_SPACE_32BIT  1
#define PLATFORM_MEMORY_ADDRESS_SPACE_64BIT  2

#if defined(__x86_64__) || defined(_M_X64) || defined(__powerpc64__)
#   define IS_PLATFORM_64BIT                1
#   define IS_PLATFORM_32BIT                0
#   define PLATFORM_MEMORY_ADDRESS_SPACE    PLATFORM_MEMORY_ADDRESS_SPACE_64BIT
#   pragma message("Using 64bit memory address space.")
#else
#   define IS_PLATFORM_64BIT                0
#   define IS_PLATFORM_32BIT                1
#   define PLATFORM_MEMORY_ADDRESS_SPACE    PLATFORM_MEMORY_ADDRESS_SPACE_32BIT
#   pragma message("Using 32bit memory address space.")
#endif

#if IS_OS_WINDOWS
#if !defined(snprintf)
#define snprintf sprintf_s
#endif
#endif

#if IS_OS_WINDOWS
#if !defined(vnsprintf)
#define vnsprintf vsprintf_s
#endif
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#define _NAME_SEPARATOR_      "|"
#define _THREADID_NAME_SEPARATOR_ "@"

#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)


inline void LOG( const char *format, ... )
{
  va_list ptr_arg;
  va_start( ptr_arg, format );

  static char tmps[1024];
  vsprintf( tmps, format, ptr_arg );


  LIB_PROFILER_PRINTF( tmps );

  va_end( ptr_arg );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// critical section

#if USE_PROFILER

// Critical Section
#if IS_OS_WINDOWS
typedef CRITICAL_SECTION ZCriticalSection_t;
inline char *ZGetCurrentDirectory( int bufLength, char *pszDest )
{
  return ( char * )GetCurrentDirectoryA( bufLength, pszDest );
}

#elif IS_OS_LINUX
#include <pthread.h>
typedef pthread_mutex_t ZCriticalSection_t;
inline char *ZGetCurrentDirectory( int bufLength, char *pszDest )
{
  return getcwd( pszDest, bufLength );
}

#elif IS_OS_MACOSX
#import <CoreServices/CoreServices.h>
typedef MPCriticalRegionID ZCriticalSection_t;
inline char *ZGetCurrentDirectory( int bufLength, char *pszDest )
{
  return getcwd( pszDest, bufLength );
}
#endif


__inline ZCriticalSection_t *NewCriticalSection()
{
#if IS_OS_LINUX
  ZCriticalSection_t *cs = new pthread_mutex_t;
  //(*cs) = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init( cs, NULL );
  return cs;
#elif IS_OS_MACOSX
  MPCriticalRegionID *criticalRegion = new MPCriticalRegionID;
  OSStatus err = MPCreateCriticalRegion( criticalRegion );
  if ( err != 0 )
  {
    delete criticalRegion;
    criticalRegion = NULL;
  }

  return criticalRegion;
#elif IS_OS_WINDOWS
  CRITICAL_SECTION *cs = new CRITICAL_SECTION;
  InitializeCriticalSection( cs );
  return cs;
#endif
}

__inline void DestroyCriticalSection( ZCriticalSection_t *cs )
{
#if IS_OS_LINUX
  delete cs;
#elif IS_OS_MACOSX
  MPDeleteCriticalRegion( *cs );
#elif IS_OS_WINDOWS
  DeleteCriticalSection( cs );
  delete cs;
#endif
}

__inline void LockCriticalSection( ZCriticalSection_t *cs )
{
#if IS_OS_LINUX
  pthread_mutex_lock( cs );
#elif IS_OS_MACOSX
  MPEnterCriticalRegion( *cs, kDurationForever );
#elif IS_OS_WINDOWS
  EnterCriticalSection( cs );
#endif
}

__inline void UnLockCriticalSection( ZCriticalSection_t *cs )
{
#if IS_OS_LINUX
  pthread_mutex_unlock( cs );
#elif IS_OS_MACOSX
  MPExitCriticalRegion( *cs );
#elif IS_OS_WINDOWS
  LeaveCriticalSection( cs );
#endif
}


bool Zprofiler_enable();
void Zprofiler_disable();
void Zprofiler_start( const char *profile_name );
void Zprofiler_end( );
void LogProfiler();

//defines

#define PROFILER_ENABLE Zprofiler_enable()
#define PROFILER_DISABLE Zprofiler_disable()
#define PROFILER_START(x) Zprofiler_start(QUOTE(x))
#define PROFILER_END() Zprofiler_end()

#else

#define LogProfiler()

#define PROFILER_ENABLE
#define PROFILER_DISABLE
#define PROFILER_START(x)
#define PROFILER_END()
#endif

#if USE_PROFILER
#ifdef LIB_PROFILER_IMPLEMENTATION

using namespace std;

void TimerInit();
//#if defined(WIN32)
double  startHighResolutionTimer( void );
/*
 #elif defined(_MAC_)
 void startHighResolutionTimer(unsigned long time[2]);
 #else
 void startHighResolutionTimer(unsigned long time[2]);
 #endif
 */
//unsigned long endHighResolutionTimer(unsigned long time[2]);


#if IS_OS_WINDOWS
// Create A Structure For The Timer Information
extern struct stTimer
{
  __int64     frequency;                  // Timer Frequency
  float     resolution;                 // Timer Resolution
  unsigned long mm_timer_start;               // Multimedia Timer Start Value
  unsigned long mm_timer_elapsed;             // Multimedia Timer Elapsed Time
  bool      performance_timer;              // Using The Performance Timer?
  __int64     performance_timer_start;          // Performance Timer Start Value
  __int64     performance_timer_elapsed;          // Performance Timer Elapsed Time
} timer;                            // Structure Is Named timer

#endif  //  IS_OS_WINDOWS


typedef struct stGenProfilerData
{
  double      totalTime;
  double      averageTime;
  double      minTime;
  double      maxTime;
  double      lastTime;       // Time of the previous passage
  double      elapsedTime;      // Elapsed Time
  unsigned long nbCalls;        // Numbers of calls
  char      szBunchCodeName[1024];  // temporary.
} tdstGenProfilerData;

//  Hold the call stack
typedef std::vector<tdstGenProfilerData> tdCallStackType;

//  Hold the sequence of profiler_starts
extern std::map<std::string, tdstGenProfilerData> mapProfilerGraph;

// Hold profiler data vector in function of the thread
extern map<unsigned long, tdCallStackType> mapCallsByThread;

// Critical section
extern ZCriticalSection_t *gProfilerCriticalSection;


// Critical section functions
/*
 void NewCriticalSection( void );
 void DestroyCriticalSection( void );
 void LockCriticalSection( void );
 void UnLockCriticalSection( void );
 */

#endif  // LIB_PROFILER_IMPLEMENTATION

#endif  // USE_PROFILER


#endif  // LIBPROFILER_H__
