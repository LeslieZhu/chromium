// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ppapi/c/ppb_console.h"
#include "ppapi/cpp/extensions/dev/socket_dev.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/tests/test_utils.h"

using namespace pp;
using namespace pp::ext;

namespace {

const char* const kSendContents = "0100000005320000005hello";
const char* const kReceiveContentsPrefix = "0100000005320000005";
const size_t kReceiveContentsSuffixSize = 11;

}  // namespace

class MyInstance : public Instance {
 public:
  explicit MyInstance(PP_Instance instance)
      : Instance(instance),
        socket_(InstanceHandle(instance)),
        console_interface_(NULL),
        socket_interface_(NULL),
        port_(0) {
  }
  virtual ~MyInstance() {
  }

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    console_interface_ = static_cast<const PPB_Console*>(
        Module::Get()->GetBrowserInterface(PPB_CONSOLE_INTERFACE));

    // TODO(yzshen): Remove this one once the cpp wrapper supports all socket
    // functions.
    socket_interface_ = static_cast<const PPB_Ext_Socket_Dev*>(
        Module::Get()->GetBrowserInterface(PPB_EXT_SOCKET_DEV_INTERFACE));
    if (!console_interface_ || !socket_interface_)
      return false;

    PostMessage(Var("ready"));
    return true;
  }

  virtual void HandleMessage(const pp::Var& message_data) {
    std::string output;
    do {
      if (!message_data.is_string()) {
        output = "Invalid control message.";
        break;
      }

      std::string control_message = message_data.AsString();
      std::vector<std::string> parts;
      size_t pos = 0;
      size_t next_match = 0;
      while (pos < control_message.size()) {
        next_match = control_message.find(':', pos);
        if (next_match == std::string::npos)
          next_match = control_message.size();
        parts.push_back(control_message.substr(pos, next_match - pos));
        pos = next_match + 1;
      }

      if (parts.size() != 3) {
        output = "Invalid protocol/address/port input.";
        break;
      }

      test_type_ = parts[0];
      address_ = parts[1];
      port_ = atoi(parts[2].c_str());
      Log(PP_LOGLEVEL_LOG, "Running tests, protocol %s, server %s:%d",
          test_type_.c_str(), address_.c_str(), port_);

      if (test_type_ == "tcp_server") {
        output = TestServerSocket();
      } else {
        output = TestClientSocket();
      }
    } while (false);

    NotifyTestDone(output);
  }

 private:
  std::string TestServerSocket() {
    int32_t socket_id = 0;
    {
      TestExtCompletionCallbackWithOutput<socket::CreateInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.Create(
          socket::SocketType_Dev(socket::SocketType_Dev::TCP),
          Optional<socket::CreateOptions_Dev>(), callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Create(): failed.";
      socket_id = callback.output().socket_id();
      if (socket_id <= 0)
        return "Create(): invalid socket ID.";
    }

    {
      TestCompletionCallback callback(pp_instance());
      PP_Var output = PP_MakeUndefined();
      callback.WaitForResult(socket_interface_->Listen(
          pp_instance(), Var(socket_id).pp_var(), Var(address_).pp_var(),
          Var(port_).pp_var(), PP_MakeUndefined(), &output,
          callback.GetCallback().pp_completion_callback()));
      if (callback.result() != PP_OK)
        return "Listen(): failed.";
      Var output_var(PASS_REF, output);
      if (output_var.AsInt() != 0)
        return "Listen(): failed.";
    }

    int32_t client_socket_id = 0;
    {
      TestExtCompletionCallbackWithOutput<socket::CreateInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.Create(
          socket::SocketType_Dev(socket::SocketType_Dev::TCP),
          Optional<socket::CreateOptions_Dev>(), callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Create(): failed.";
      client_socket_id = callback.output().socket_id();
      if (client_socket_id <= 0)
        return "Create(): invalid socket ID.";
    }

    {
      TestCompletionCallback callback(pp_instance());
      PP_Var output = PP_MakeUndefined();
      callback.WaitForResult(socket_interface_->Connect(
          pp_instance(), Var(client_socket_id).pp_var(), Var(address_).pp_var(),
          Var(port_).pp_var(), &output,
          callback.GetCallback().pp_completion_callback()));
      if (callback.result() != PP_OK)
        return "Connect(): failed.";
      Var output_var(PASS_REF, output);
      if (output_var.AsInt() != 0)
        return "Connect(): failed.";
    }

    int32_t accepted_socket_id = 0;
    {
      TestExtCompletionCallbackWithOutput<socket::AcceptInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.Accept(socket_id, callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Accept(): failed.";
      socket::AcceptInfo_Dev accept_info = callback.output();
      if (accept_info.result_code() != 0 || !accept_info.socket_id().IsSet())
        return "Accept(): failed.";
      accepted_socket_id = *accept_info.socket_id();
    }

    size_t bytes_written = 0;
    {
      TestExtCompletionCallbackWithOutput<socket::WriteInfo_Dev>
          callback(pp_instance());
      VarArrayBuffer array_buffer = ConvertToArrayBuffer(kSendContents);
      callback.WaitForResult(socket_.Write(client_socket_id, array_buffer,
                                           callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Write(): failed.";
      socket::WriteInfo_Dev write_info = callback.output();
      bytes_written = static_cast<size_t>(write_info.bytes_written());
      if (bytes_written <= 0)
        return "Write(): did not write any bytes.";
    }

    {
      TestExtCompletionCallbackWithOutput<socket::ReadInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.Read(
          accepted_socket_id, Optional<int32_t>(), callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Read(): failed.";

      std::string data_string = ConvertFromArrayBuffer(
          &callback.output().data());
      if (data_string.compare(0, std::string::npos, kSendContents,
                              bytes_written) != 0) {
        return "Read(): Received data does not match.";
      }
    }

    socket_.Destroy(client_socket_id);
    socket_.Destroy(accepted_socket_id);
    socket_.Destroy(socket_id);
    return std::string();
  }

  std::string TestClientSocket() {
    socket::SocketType_Dev socket_type;
    if (!socket_type.Populate(Var(test_type_).pp_var()))
      return "Invalid socket type.";

    int32_t socket_id = 0;
    {
      TestExtCompletionCallbackWithOutput<socket::CreateInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.Create(
          socket_type, Optional<socket::CreateOptions_Dev>(),
          callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "Create(): failed.";
      socket_id = callback.output().socket_id();
      if (socket_id <= 0)
        return "Create(): invalid socket ID.";
    }

    {
      TestExtCompletionCallbackWithOutput<socket::SocketInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.GetInfo(socket_id,
                                             callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "GetInfo(): failed.";

      socket::SocketInfo_Dev socket_info = callback.output();
      if (socket_info.socket_type().value != socket_type.value)
        return "GetInfo(): inconsistent socket type.";
      if (socket_info.connected())
        return "GetInfo(): socket should not be connected.";
      if (socket_info.peer_address().IsSet() || socket_info.peer_port().IsSet())
        return "GetInfo(): unconnected socket should not have peer.";
      if (socket_info.local_address().IsSet() ||
          socket_info.local_port().IsSet()) {
        return "GetInfo(): unconnected socket should not have local binding.";
      }
    }

    {
      if (socket_type.value == socket::SocketType_Dev::TCP) {
        TestCompletionCallback callback(pp_instance());
        PP_Var output = PP_MakeUndefined();
        callback.WaitForResult(socket_interface_->Connect(
            pp_instance(), Var(socket_id).pp_var(), Var(address_).pp_var(),
            Var(port_).pp_var(), &output,
            callback.GetCallback().pp_completion_callback()));
        if (callback.result() != PP_OK)
          return "Connect(): failed.";
        Var output_var(PASS_REF, output);
        if (output_var.AsInt() != 0)
          return "Connect(): failed.";
      } else {
        TestCompletionCallback callback(pp_instance());
        PP_Var output = PP_MakeUndefined();
        callback.WaitForResult(socket_interface_->Bind(
            pp_instance(), Var(socket_id).pp_var(), Var("0.0.0.0").pp_var(),
            Var(0).pp_var(), &output,
            callback.GetCallback().pp_completion_callback()));
        if (callback.result() != PP_OK)
          return "Bind(): failed.";
        Var output_var(PASS_REF, output);
        if (output_var.AsInt() != 0)
          return "Bind(): failed.";
      }
    }

    {
      TestExtCompletionCallbackWithOutput<socket::SocketInfo_Dev>
          callback(pp_instance());
      callback.WaitForResult(socket_.GetInfo(socket_id,
                                             callback.GetCallback()));
      if (callback.result() != PP_OK)
        return "GetInfo(): failed.";

      socket::SocketInfo_Dev socket_info = callback.output();
      if (socket_info.socket_type().value != socket_type.value)
        return "GetInfo(): inconsistent socket type.";
      if (!socket_info.local_address().IsSet() ||
          !socket_info.local_port().IsSet()) {
        return "GetInfo(): bound socket should have local address and port.";
      }
      if (socket_type.value == socket::SocketType_Dev::TCP) {
        if (!socket_info.connected())
          return "GetInfo(): TCP socket should be connected.";
        if (!socket_info.peer_address().IsSet() ||
            !socket_info.peer_port().IsSet()) {
          return "GetInfo(): connected TCP socket should have peer address and "
                 "port";
        }
        if (*socket_info.peer_address() != "127.0.0.1" ||
            *socket_info.peer_port() != port_) {
          return "GetInfo(): peer address and port should match the listening "
                 "server.";
        }
      } else {
        if (socket_info.connected())
          return "GetInfo(): UDP socket should not be connected.";
        if (socket_info.peer_address().IsSet() ||
            socket_info.peer_port().IsSet()) {
          return "GetInfo(): unconnected UDP socket should not have peer "
                 "address or port.";
        }
      }
    }

    {
      TestCompletionCallback callback(pp_instance());
      PP_Var output = PP_MakeUndefined();
      callback.WaitForResult(socket_interface_->SetNoDelay(
          pp_instance(), Var(socket_id).pp_var(), Var(true).pp_var(),
          &output, callback.GetCallback().pp_completion_callback()));
      if (callback.result() != PP_OK)
        return "SetNoDelay(): failed.";
      Var output_var(PASS_REF, output);
      if (socket_type.value == socket::SocketType_Dev::TCP) {
        if (!output_var.AsBool())
          return "SetNoDelay(): failed for TCP.";
      } else {
        if (output_var.AsBool())
          return "SetNoDelay(): did not fail for UDP.";
      }
    }

    {
      TestCompletionCallback callback(pp_instance());
      PP_Var output = PP_MakeUndefined();
      callback.WaitForResult(socket_interface_->SetKeepAlive(
          pp_instance(), Var(socket_id).pp_var(), Var(true).pp_var(),
          Var(1000).pp_var(), &output,
          callback.GetCallback().pp_completion_callback()));
      if (callback.result() != PP_OK)
        return "SetKeepAlive(): failed.";
      Var output_var(PASS_REF, output);
      if (socket_type.value == socket::SocketType_Dev::TCP) {
        if (!output_var.AsBool())
          return "SetKeepAlive(): failed for TCP.";
      } else {
        if (output_var.AsBool())
          return "SetKeepAlive(): did not fail for UDP.";
      }
    }

    {
      VarArrayBuffer input_array_buffer = ConvertToArrayBuffer(kSendContents);
      size_t bytes_written = 0;
      int32_t result_code = 0;
      VarArrayBuffer data;
      if (socket_type.value == socket::SocketType_Dev::TCP) {
        TestExtCompletionCallbackWithOutput<socket::ReadInfo_Dev>
            read_callback(pp_instance());
        int32_t read_result = socket_.Read(socket_id, Optional<int32_t>(),
                                           read_callback.GetCallback());
        if (read_result != PP_OK_COMPLETIONPENDING)
          return "Read(): did not wait for data.";

        TestExtCompletionCallbackWithOutput<socket::WriteInfo_Dev>
            write_callback(pp_instance());
        write_callback.WaitForResult(socket_.Write(
            socket_id, input_array_buffer, write_callback.GetCallback()));
        if (write_callback.result() != PP_OK)
          return "Write(): failed.";
        bytes_written = static_cast<size_t>(
            write_callback.output().bytes_written());

        read_callback.WaitForResult(read_result);
        if (read_callback.result() != PP_OK)
          return "Read(): failed.";
        socket::ReadInfo_Dev read_info = read_callback.output();
        result_code = read_info.result_code(),
        data = read_info.data();
      } else {
        TestExtCompletionCallbackWithOutput<socket::RecvFromInfo_Dev>
            recv_from_callback(pp_instance());
        int32_t recv_from_result = socket_.RecvFrom(
            socket_id, Optional<int32_t>(), recv_from_callback.GetCallback());
        if (recv_from_result != PP_OK_COMPLETIONPENDING)
          return "RecvFrom(): did not wait for data.";

        TestExtCompletionCallbackWithOutput<socket::WriteInfo_Dev>
            send_to_callback(pp_instance());
        send_to_callback.WaitForResult(socket_.SendTo(
            socket_id, input_array_buffer, address_, port_,
            send_to_callback.GetCallback()));
        if (send_to_callback.result() != PP_OK)
          return "SendTo(): failed.";
        bytes_written = static_cast<size_t>(
            send_to_callback.output().bytes_written());

        recv_from_callback.WaitForResult(recv_from_result);
        if (recv_from_callback.result() != PP_OK)
          return "RecvFrom(): failed.";
        socket::RecvFromInfo_Dev recv_from_info = recv_from_callback.output();
        result_code = recv_from_info.result_code();
        data = recv_from_info.data();
      }

      if (bytes_written != strlen(kSendContents))
        return "SendTo() or Write(): did not send the whole data buffer.";

      if (result_code > 0 &&
          static_cast<uint32_t>(result_code) != data.ByteLength()) {
        return "Read() or RecvFrom(): inconsistent result code and byte "
               "length.";
      }

      std::string output_string = ConvertFromArrayBuffer(&data);
      size_t prefix_len = strlen(kReceiveContentsPrefix);
      if (output_string.size() != prefix_len + kReceiveContentsSuffixSize ||
          output_string.compare(0, prefix_len, kReceiveContentsPrefix) != 0) {
        return std::string("Read() or RecvFrom(): mismatch data: ").append(
            output_string);
      }
    }

    socket_.Destroy(socket_id);
    return std::string();
  }

  void Log(PP_LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[512];
    vsnprintf(buf, sizeof(buf) - 1, format, args);
    buf[sizeof(buf) - 1] = '\0';
    va_end(args);

    Var value(buf);
    console_interface_->Log(pp_instance(), level, value.pp_var());
  }

  void NotifyTestDone(const std::string& message) {
    PostMessage(message);
  }

  VarArrayBuffer ConvertToArrayBuffer(const std::string data) {
    VarArrayBuffer array_buffer(data.size());
    memcpy(array_buffer.Map(), data.c_str(), data.size());
    array_buffer.Unmap();
    return array_buffer;
  }

  std::string ConvertFromArrayBuffer(VarArrayBuffer* array_buffer) {
    std::string result(static_cast<const char*>(array_buffer->Map()),
                       array_buffer->ByteLength());
    array_buffer->Unmap();
    return result;
  }

  socket::Socket_Dev socket_;
  const PPB_Console* console_interface_;
  const PPB_Ext_Socket_Dev* socket_interface_;

  std::string test_type_;
  std::string address_;
  int32_t port_;
};

class MyModule : public Module {
 public:
  MyModule() : Module() {}
  virtual ~MyModule() {}

  virtual Instance* CreateInstance(PP_Instance instance) {
    return new MyInstance(instance);
  }
};

namespace pp {

Module* CreateModule() {
  return new MyModule();
}

}  // namespace pp

