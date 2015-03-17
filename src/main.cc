#include "htn_config.h"

#include <iostream>
#include <string>

#include "conductor.h"
#include "flagdefs.h"
#include "main_wnd.h"
#include "socket_connection.h"
#include "peer_connection_client.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/logging.h"


void Usage();
void Error(const std::string& msg);
void FatalError(const std::string& msg);


int PASCAL wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    wchar_t* cmd_line, int cmd_show) {

  rtc::WindowsCommandLineArguments win_args;
  int argc = win_args.argc();
  char **argv = win_args.argv();

  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
  if (FLAG_help) {
    rtc::FlagList::Print(NULL, false);
    return 0;
  }

  //
  // Arguments 
  //

  bool server_mode;
  rtc::SocketAddress local_address;
  rtc::SocketAddress remote_address;
  std::string tunnel_key;
  cricket::ProtocolType protocol;
  
  server_mode = FLAG_server;
  protocol = FLAG_udp ? cricket::PROTO_UDP : cricket::PROTO_TCP;
  tunnel_key = FLAG_k;

  if (server_mode) {
    if (argc != 1) Usage();
  }
  else{
    if (argc != 3) Usage();

    std::string local_port = argv[1];
    std::string remote_port = argv[2];

    if (local_port.find(":") == std::string::npos) local_port = "0.0.0.0:"+local_port;
    if (!local_address.FromString(local_port)) {
      LOG(LS_ERROR) << argv[1] + std::string(" is not a valid port or address");
      return -1;
    }

    if (remote_port.find(":") == std::string::npos) remote_port = "0.0.0.0:" + remote_port;
    if (!remote_address.FromString(remote_port)) {
      LOG(LS_ERROR) << argv[1] + std::string(" is not a valid port or address");
      return -1;
    }

  }

  //
  // Setup conductor
  //

  rtc::EnsureWinsockInit();
  rtc::Win32Thread w32_thread;
  rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

  MainWnd wnd("localhost", FLAG_port, FLAG_autoconnect, FLAG_autocall);
  if (!wnd.Create()) {
    ASSERT(false);
    return -1;
  }

  rtc::InitializeSSL();
  PeerConnectionClient client;
  SocketConnection socket_connection(server_mode);
  
  if (server_mode) {
    if (!socket_connection.SetServerMode()) return -1;
  }
  else {
    if (!socket_connection.SetClientMode(local_address, remote_address, tunnel_key, protocol)) return -1;
  }

  rtc::scoped_refptr<Conductor> conductor(
    new rtc::RefCountedObject<Conductor>(&client, &socket_connection, &wnd));

  //
  // Main loop.
  //

  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
    if (!wnd.PreTranslateMessage(&msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  if (conductor->connection_active() || client.is_connected()) {
    while ((conductor->connection_active() || client.is_connected()) &&
           (gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
      if (!wnd.PreTranslateMessage(&msg)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

  rtc::CleanupSSL();
  return 0;
}


// Prints out a usage message then exits.
void Usage() {
  std::cerr << "Hotline tunnel: Lightweight peer to peer and NAT traversal VPN." << std::endl;
  std::cerr << "Usage:" << std::endl;
  std::cerr << "Server side:  htunnel -server [-options]" << std::endl;
  std::cerr << "Client side:  htunnel -k tunnel-key localport remoteport [-options]" << std::endl;
  std::cerr << "options:" << std::endl;
  std::cerr << "         -udp      UDP mode" << std::endl;
  std::cerr << std::endl;
  exit(1);
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