#pragma once

// Reduce bloat
#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN) && !defined(FUN_BLOATED_WIN32)
    #define WIN32_LEAN_AND_MEAN
  #endif
  #if !defined(NOMINMAX)
    #define NOMINMAX
  #endif
#endif


// Microsoft Visual C++ includes copies of the Windows header files
// that were current at the time Visual C++ was released.
// The Windows header files use macros to indicate which versions
// of Windows support many programming elements. Therefore, you must
// define these macros to use new functionality introduced in each
// major operating system release. (Individual header files may use
// different macros; therefore, if compilation problems occur, check
// the header file that contains the definition for conditional
// definitions.) For more information, see SdkDdkVer.h.


#if !defined(_WIN32_WCE)
#if defined(_WIN32_WINNT)
  #if (_WIN32_WINNT < 0x0501)
    #error Unsupported Windows version.
  #endif
#elif defined(NTDDI_VERSION)
  #if (NTDDI_VERSION < 0x05010100)
    #error Unsupported Windows version.
  #endif
#elif !defined(_WIN32_WINNT)
  // Define minimum supported version.
  // This can be changed, if needed.
  // If allowed (see FUN_MIN_WINDOWS_OS_SUPPORT
  // below), Platform_WIN32.h will do its
  // best to determine the appropriate values
  // and may redefine these. See Platform_WIN32.h
  // for details.
  #define _WIN32_WINNT 0x0501
  #define NTDDI_VERSION 0x05010100
#endif
#endif

#if !defined(FUN_NO_WINDOWS_H)
  #include <windows.h>
  #ifdef __MINGW32__
    #include <Winsock2.h>
    #include <ws2tcpip.h>
  #endif // __MINGW32__
#endif

// To prevent Platform_WIN32.h to modify version defines,
// uncomment this, otherwise versions will be automatically
// discovered in Platform_WIN32.h.
// #define FUN_FORCE_MIN_WINDOWS_OS_SUPPORT

#if !defined(FUN_NO_UNWINDOWS)
// A list of annoying macros to #undef.
// Extend as required.
#undef GetBinaryType
#undef GetShortPathName
#undef GetLongPathName
#undef GetEnvironmentStrings
#undef SetEnvironmentStrings
#undef FreeEnvironmentStrings
#undef FormatMessage
#undef EncryptFile
#undef DecryptFile
#undef CreateMutex
#undef OpenMutex
#undef CreateEvent
#undef OpenEvent
#undef CreateSemaphore
#undef OpenSemaphore
#undef LoadLibrary
#undef GetModuleFileName
#undef CreateProcess
#undef GetCommandLine
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef ExpandEnvironmentStrings
#undef OutputDebugString
#undef FindResource
#undef UpdateResource
#undef FindAtom
#undef AddAtom
#undef GetSystemDirectory
#undef GetTempPath
#undef GetTempFileName
#undef SetCurrentDirectory
#undef GetCurrentDirectory
#undef CreateDirectory
#undef RemoveDirectory
#undef CreateFile
#undef DeleteFile
#undef SearchPath
#undef CopyFile
#undef MoveFile
#undef ReplaceFile
#undef GetComputerName
#undef SetComputerName
#undef GetUserName
#undef LogonUser
#undef GetVersion
#undef GetObject

//added at 2019/02/01 - exception.h에서 출돌이 났었음.
#undef GetMessage
#undef GetClassName
#undef Yield
#undef RegisterClass
#undef UnregisterClass
#undef GetTimeFormat
#undef GetDateFormat

#endif // FUN_NO_UNWINDOWS
