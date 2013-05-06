// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_TCP_H_
#define CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_TCP_H_

#include <queue>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "content/browser/renderer_host/p2p/socket_host.h"
#include "content/common/p2p_sockets.h"
#include "net/base/completion_callback.h"
#include "net/base/ip_endpoint.h"

namespace net {
class DrainableIOBuffer;
class GrowableIOBuffer;
class StreamSocket;
}  // namespace net

namespace content {

class CONTENT_EXPORT P2PSocketHostTcpBase : public P2PSocketHost {
 public:
  P2PSocketHostTcpBase(IPC::Sender* message_sender, int id);
  virtual ~P2PSocketHostTcpBase();

  bool InitAccepted(const net::IPEndPoint& remote_address,
                    net::StreamSocket* socket);

  // P2PSocketHost overrides.
  virtual bool Init(const net::IPEndPoint& local_address,
                    const net::IPEndPoint& remote_address) OVERRIDE;
  virtual void Send(const net::IPEndPoint& to,
                    const std::vector<char>& data) OVERRIDE;
  virtual P2PSocketHost* AcceptIncomingTcpConnection(
      const net::IPEndPoint& remote_address, int id) OVERRIDE;

 protected:
  // Derived classes will provide the implementation.
  virtual int ProcessInput(char* input, int input_len) = 0;
  virtual void DoSend(const net::IPEndPoint& to,
                      const std::vector<char>& data) = 0;

  void WriteOrQueue(scoped_refptr<net::DrainableIOBuffer>& buffer);
  void OnPacket(const std::vector<char>& data);
  void OnError();

 private:
  friend class P2PSocketHostTcpTest;
  friend class P2PSocketHostTcpServerTest;
  friend class P2PSocketHostStunTcpTest;

  void DidCompleteRead(int result);
  void DoRead();

  void DoWrite();
  void HandleWriteResult(int result);

  // Callbacks for Connect(), Read() and Write().
  void OnConnected(int result);
  void OnRead(int result);
  void OnWritten(int result);

  net::IPEndPoint remote_address_;

  scoped_ptr<net::StreamSocket> socket_;
  scoped_refptr<net::GrowableIOBuffer> read_buffer_;
  std::queue<scoped_refptr<net::DrainableIOBuffer> > write_queue_;
  scoped_refptr<net::DrainableIOBuffer> write_buffer_;

  bool write_pending_;

  bool connected_;

  DISALLOW_COPY_AND_ASSIGN(P2PSocketHostTcpBase);
};

class CONTENT_EXPORT P2PSocketHostTcp : public P2PSocketHostTcpBase {
 public:
  P2PSocketHostTcp(IPC::Sender* message_sender, int id);
  virtual ~P2PSocketHostTcp();

 protected:
  virtual int ProcessInput(char* input, int input_len) OVERRIDE;
  virtual void DoSend(const net::IPEndPoint& to,
                      const std::vector<char>& data) OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(P2PSocketHostTcp);
};

// P2PSocketHostStunTcp class provides the framing of STUN messages when used
// with TURN. These messages will not have length at front of the packet and
// are padded to multiple of 4 bytes.
// Formatting of messages is defined in RFC5766.
class CONTENT_EXPORT P2PSocketHostStunTcp : public P2PSocketHostTcpBase {
 public:
  P2PSocketHostStunTcp(IPC::Sender* message_sender, int id);
  virtual ~P2PSocketHostStunTcp();

 protected:
  virtual int ProcessInput(char* input, int input_len) OVERRIDE;
  virtual void DoSend(const net::IPEndPoint& to,
                      const std::vector<char>& data) OVERRIDE;
 private:
  int GetExpectedPacketSize(const char* data, int len, int* pad_bytes);

  DISALLOW_COPY_AND_ASSIGN(P2PSocketHostStunTcp);
};


}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_TCP_H_
