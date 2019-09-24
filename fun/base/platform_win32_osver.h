#if !defined(__FUN_BASE_BASE_H__)
#error Do not include this file directly. It should be included only in base.h file.
#endif

//TODO 이걸 여기서 포함해야하나??? 여기에다가 넣어버리면 전역으로 딸려가게 되는데 말이지...
#include "fun/base/windows_less.h"

#ifndef FUN_FORCE_MIN_WINDOWS_OS_SUPPORT

  // Determine the real version.
  // This setting can be forced from UnWindows.h
  #if defined (_WIN32_WINNT_WINBLUE)
    //Windows 8.1 _WIN32_WINNT_WINBLUE (0x0602)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WINBLUE
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WINBLUE
  #elif defined (_WIN32_WINNT_WIN8)
    //Windows 8 _WIN32_WINNT_WIN8 (0x0602)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WIN8
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WIN8
  #elif defined (_WIN32_WINNT_WIN7)
    //Windows 7 _WIN32_WINNT_WIN7 (0x0601)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WIN7
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WIN7
  #elif defined (_WIN32_WINNT_WS08)
    //Windows Server 2008 _WIN32_WINNT_WS08 (0x0600)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WS08
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WS08
  #elif defined (_WIN32_WINNT_VISTA)
    //Windows Vista _WIN32_WINNT_VISTA (0x0600)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_VISTA
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_VISTA
  #elif defined (_WIN32_WINNT_LONGHORN)
    //Windows Vista and server 2008 Development _WIN32_WINNT_LONGHORN (0x0600)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_LONGHORN
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  0x06000000 // hardcoded, VS90 can't find NTDDI_* macros
  #elif defined (_WIN32_WINNT_WS03)
    //Windows Server 2003 with SP1,
    //Windows XP with SP2 _WIN32_WINNT_WS03 (0x0502)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WS03
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WS03
  #elif defined (_WIN32_WINNT_WINXP)
    //Windows Server 2003, Windows XP _WIN32_WINNT_WINXP (0x0501)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WINXP
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  NTDDI_WINXP
  #elif defined (_WIN32_WINNT_WIN2K)
    //Windows 2000 _WIN32_WINNT_WIN2K (0x0500)
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  _WIN32_WINNT_WIN2K
  #elif defined (WINVER)
    // fail back on WINVER
    #ifdef _WIN32_WINNT
      #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT  WINVER
  #elif !defined(_WIN32_WINNT)
    // last resort = Win XP, SP1 is minimum supported
    #define _WIN32_WINNT  0x0501
    #ifdef NTDDI_VERSION
      #undef NTDDI_VERSION
    #endif
    #define NTDDI_VERSION  0x05010100
  #endif

#endif // FUN_FORCE_MIN_WINDOWS_OS_SUPPORT
