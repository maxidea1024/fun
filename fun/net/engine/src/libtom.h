#pragma once

#include "CorePrivatePCH.h"

//#if FUN_PLATFORM_WINDOWS_FAMILY
//#define LTC_WINTHREAD
//#else
//#define LTC_PTHREAD
//#endif
//
//#define LTC_NO_PROTOTYPES
//#define LTC_FORTUNA
//
//#include "tomcrypt.h"
//
//#if FUN_PLATFORM_WINDOWS_FAMILY
//
//#if FUN_64_BIT
//
//#ifdef _DEBUG
//#pragma comment(lib, "ThirdParty/libtomcrypt-develop/MSVC_x64_Debug/tomcryptd.lib")
//#else
//#pragma comment(lib, "ThirdParty/libtomcrypt-develop/MSVC_x64_Release/tomcrypt.lib")
//#endif
//
//#else
//
//#ifdef _DEBUG
//#pragma comment(lib, "ThirdParty/libtomcrypt-develop/MSVC_Win32_Debug/tomcryptd.lib")
//#else
//#pragma comment(lib, "ThirdParty/libtomcrypt-develop/MSVC_Win32_Release/tomcrypt.lib")
//#endif
//
//#endif
//
//#endif

#include "libtom_temp/crypto/headers/tomcrypt.h"
