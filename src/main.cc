#include "htn_config.h"

#include <iostream>
#include <string>

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/logging.h"

#include "conductors.h"
#include "flagdefs.h"
#include "signalserver_connection.h"


#if WIN32
bool ReinitializeWinSock();
#endif

void Usage();
void Error(const std::string& msg);
void FatalError(const std::string& msg);


int main(int argc, char** argv) {

#if WIN32
  // Stop memory leak detection by default and start memory dection after following code
  // because to prevent display webrtc internal memory leak.
  //   rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);
  _CrtSetDbgFlag(0);

  //
  // Set current thread
  //
  rtc::EnsureWinsockInit();
  rtc::Win32Thread w32_thread;
  rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

  //
  // Enable memory leak detection
  //

#ifdef _DEBUG 
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif // DEBUG

  if (!ReinitializeWinSock()) {
    Error("WinSock initialization failed.");
    return 1;
  }
#endif // WIN32

  rtc::WindowsCommandLineArguments win_args;

  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
  if (FLAG_help || FLAG_h) {
    rtc::FlagList::Print(NULL, false);
    return 1;
  }

  //
  // Arguments 
  //

  hotline::UserArguments arguments;
  
  arguments.server_mode = FLAG_server;
  arguments.protocol = FLAG_udp ? cricket::PROTO_UDP : cricket::PROTO_TCP;
  arguments.room_id = FLAG_r;
  arguments.password = FLAG_p;

  if (arguments.server_mode) {
    if (argc != 1) {
      Usage();
      return 1;
    }
  }
  else{
    if (argc != 3) {
      Usage();
      return 1;
    }

    std::string local_port = argv[1];
    std::string remote_port = argv[2];

    if (local_port.find(":") == std::string::npos) local_port = "0.0.0.0:"+local_port;
    if (!arguments.local_address.FromString(local_port)) {
      LOG(LS_ERROR) << argv[1] + std::string(" is not a valid port or address");
      return 1;
    }

    if (remote_port.find(":") == std::string::npos) remote_port = "127.0.0.1:" + remote_port;
    if (!arguments.remote_address.FromString(remote_port)) {
      LOG(LS_ERROR) << argv[1] + std::string(" is not a valid port or address");
      return 1;
    }
  }

  //
  // Setup conductor
  //

  rtc::InitializeSSL();
  hotline::SignalServerConnection signal_client(rtc::ThreadManager::Instance()->CurrentThread());
  rtc::scoped_ptr<hotline::Conductors> conductors(
                              new hotline::Conductors( &signal_client,
                              rtc::ThreadManager::Instance()->CurrentThread(),
                              arguments)
                              );

  //
  // Connect to signal server
  //

  std::string server_url = GetSignalServerName();
  if ((server_url.find("ws://") != 0 && server_url.find("wss://") != 0)) {
    return 1;
  }

  if (server_url.back() != '/') server_url.push_back('/');
  server_url.append(kDefaultServerPath);

  signal_client.Connect(server_url);

  rtc::ThreadManager::Instance()->CurrentThread()->Run();

  rtc::CleanupSSL();
  return 0;
}



#if WIN32
bool ReinitializeWinSock() {

  // There is version confliction of WinSock between webrtc and libwebsockets.
  // Webrtc needs winsock 1.0 and libwebsockets needs winsock 2.2.
  // So cleanup webrtc's winsock initiaization and reinitialize with version 2.2.
  // otherwise, signalserver connection will be failed with LWS_CALLBACK_CLIENT_CONNECTION_ERROR.

  WSACleanup();

  WORD wVersionRequested;
  WSADATA wsaData;
  int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro from Windef.h */
  wVersionRequested = MAKEWORD(2, 2);

  err = WSAStartup(wVersionRequested, &wsaData);
  if (!err) return true;

  return false;
}
#endif // WIN32

// Prints out a usage message then exits.
void Usage() {
  std::cerr << "Hotline tunnel: Mini peer to peer VPN." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Usage" << std::endl;
  std::cerr << " Remote peer: htunnel -server [-p password]" << std::endl;
  std::cerr << " Local  peer: htunnel localport remotehost:port -r roomid [-p password -u udp]" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Example" << std::endl;
  std::cerr << " Remote: htunnel -server -p roompassword" << std::endl;
  std::cerr << " Local : htunnel 22 127.0.0.1:22 -r 12345 -p roompassword" << std::endl;
}


// Prints out an error message, a usage message, then exits.
void Error(const std::string& msg) {
  std::cerr << "error: " << msg << std::endl;
  std::cerr << std::endl;
  Usage();
}

void FatalError(const std::string& msg) {
  std::cerr << "error: " << msg << std::endl;
  std::cerr << std::endl;
  exit(1);
}
