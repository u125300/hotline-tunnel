#ifndef HOTLINE_TUNNEL_HTN_CONFIG_H_
#define HOTLINE_TUNNEL_HTN_CONFIG_H_

#define @WEBRTC_POSIX_OR_WIN@ 1

#if WIN32
#define _CRT_SECURE_NO_WARNINGS
#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <stdlib.h>
  #include <crtdbg.h>
  #ifndef DBG_NEW
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    #define new DBG_NEW
 #endif
#endif  // _DEBUG
#endif //WIN32

#endif // HOTLINE_TUNNEL_HTN_CONFIG_H_