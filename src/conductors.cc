#include "htn_config.h"

#include <utility>
#include <vector>
#include <iostream>

#include "conductors.h"
#include "conductor.h"



namespace hotline {

Conductors::Conductors(SignalServerConnection* signal_client,
                     rtc::Thread* signal_thread,
                     UserArguments& arguments)
  : id_(0),
    signal_client_(signal_client),
    signal_thread_(signal_thread),
    server_mode_(arguments.server_mode),
    local_address_(arguments.local_address),
    remote_address_(arguments.remote_address),
    protocol_(arguments.protocol),
    room_id_(arguments.room_id),
    password_(arguments.password) {

  signal_client_->RegisterObserver(this);
}

Conductors::~Conductors() {
}


std::string Conductors::id_string() const {
  std::stringstream stream;
  stream << std::hex << id_;
  return stream.str();
}

uint64 Conductors::id_from_string(std::string id_string) {
  uint64 id = 0;
  std::stringstream stream;
  stream << std::hex << id_string;
  stream >> id;
  return id;
}


void Conductors::Close() {
  //:signal_client_->SignOut();
  // TODO: Delete all peers
}


//
// SignalServerConnectionObserver implementation.
//

void Conductors::OnConnected() {
  if (server_mode()) {
    signal_client_->CreateRoom(password_);
  }
  else {
    signal_client_->SignIn(room_id_, password_);
  }
}

void Conductors::OnCreatedRoom(std::string& room_id) {
  ASSERT(server_mode_);

  std::cout << "Your room id is " << room_id.c_str() << "." << std::endl;

  room_id_ = room_id;
  signal_client_->SignIn(room_id_, password_);
} 

void Conductors::OnSignedIn(std::string& room_id, uint64 peer_id) {

  if (server_mode()) {
    ASSERT(room_id_==room_id);
  }

  room_id_ = room_id;
  id_ = peer_id;

}

void Conductors::OnPeerConnected(uint64 peer_id) {

  typedef std::pair<uint64, rtc::scoped_refptr<Conductor>> PeerPair;

  //
  // Answerer
  //

  rtc::scoped_refptr<Conductor> conductor_answer(
          new rtc::RefCountedObject<Conductor> ());

  peers_answer_.insert(PeerPair(peer_id, conductor_answer));

  conductor_answer->Init(server_mode_,
                    local_address_,
                    remote_address_,
                    protocol_,
                    room_id_,
                    id_,
                    peer_id,
                    signal_client_,
                    signal_thread_);

  //
  // Offerer
  //

  rtc::scoped_refptr<Conductor> conductor_offer(
          new rtc::RefCountedObject<Conductor> ());

  peers_offer_.insert(PeerPair(peer_id, conductor_offer));

  conductor_offer->Init(server_mode_,
                    local_address_,
                    remote_address_,
                    protocol_,
                    room_id_,
                    id_,
                    peer_id,
                    signal_client_,
                    signal_thread_
                    );

  // ConnectToPeer if offerer
  conductor_offer->ConnectToPeer();

  if (server_mode()) {
    std::cout << "Peer connected. (peerid: " << std::to_string(peer_id) << ")." << std::endl;
  }
}

void Conductors::OnPeerDisconnected(uint64 peer_id) {
  LOG(LS_INFO) << "Peer " << std::to_string(peer_id) << " disconnected.";

  if (server_mode()) {
    std::cout << "Peer disconnected. (peerid: " << std::to_string(peer_id) << ")." << std::endl;
  }

  peers_offer_.erase(peer_id);
  peers_answer_.erase(peer_id);

  if (client_mode()) {
    if (peers_answer_.size() == 0 && peers_offer_.size() == 0) {
      signal_thread_->Post(this, MsgExit);
    }
  }

}

void Conductors::OnReceivedOffer(Json::Value& data) {

  std::string peer_id;
  uint64 npeer_id;

  if(!rtc::GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);

  if (peers_offer_.find(npeer_id) == peers_offer_.end()) {
    return;
  }

  peers_offer_[npeer_id]->OnReceivedOffer(data);
}



void Conductors::OnDisconnected() {
  std::cout << "Connection to signal server closed." << std::endl;
}

void Conductors::OnServerConnectionFailure(int code, std::string& message) {
  ASSERT(signal_thread_ != NULL);

  if (message.length() > 0) {
    std::cerr << message << std::endl;//:"Signal server connection error. " << GetSignalServerName() << "." << std::endl;
  }

  signal_thread_->Post(this, MsgExit);
}


void Conductors::OnMessage(rtc::Message* msg) {
  try {
    if (msg->message_id == ThreadMsgId::MsgExit) {
      OnClose();
      rtc::ThreadManager::Instance()->CurrentThread()->Stop();
    }
  }
  catch (...) {
    LOG(LS_WARNING) << "Conductors::OnMessage() Exception.";
  }
}

void Conductors::OnClose() {
  signal_client_->UnregisterObserver(this);
}


} // namespace hotline
