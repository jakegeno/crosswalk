// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_APPLICATION_BROWSER_LINUX_RUNNING_APPLICATION_OBJECT_H_
#define XWALK_APPLICATION_BROWSER_LINUX_RUNNING_APPLICATION_OBJECT_H_

#include <string>
#include "base/memory/ref_counted.h"
#include "dbus/bus.h"
#include "ipc/ipc_channel_handle.h"
#include "xwalk/application/browser/application.h"
#include "xwalk/dbus/object_manager_adaptor.h"

namespace dbus {
class FileDescriptor;
}

namespace xwalk {
namespace application {

// Represents the running application inside D-Bus hierarchy of
// RunningApplicationsManager.
//
// Watches for the D-Bus presence of the launcher, when it disappears (e.g. the
// launcher was ended by a task manager) will terminate the corresponding
// application in Crosswalk.
class RunningApplicationObject : public dbus::ManagedObject {
 public:
  RunningApplicationObject(scoped_refptr<dbus::Bus> bus,
                           const std::string& app_id,
                           const std::string& launcher_name,
                           Application* application);

  virtual ~RunningApplicationObject();

  void ExtensionProcessCreated(const IPC::ChannelHandle& handle);
#if defined(OS_TIZEN)
  void SaveEncodedBundle(const std::string& encoded_bundle);
#endif

 private:
  void TerminateApplication();

  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  void OnTerminate(dbus::MethodCall* method_call,
                   dbus::ExportedObject::ResponseSender response_sender);

  void OnGetExtensionProcessChannel(
      dbus::MethodCall* method_call,
      dbus::ExportedObject::ResponseSender response_sender);

#if defined(OS_TIZEN)
  void OnHide(dbus::MethodCall* method_call,
              dbus::ExportedObject::ResponseSender response_sender);

  void OnSuspend(dbus::MethodCall* method_call,
                 dbus::ExportedObject::ResponseSender response_sender);

  void OnResume(dbus::MethodCall* method_call,
                dbus::ExportedObject::ResponseSender response_sender);

  void OnRemoveAllCookies(dbus::MethodCall* method_call,
                   dbus::ExportedObject::ResponseSender response_sender);

  void SetUserAgentStringOnIOThread(const std::string& user_agent_string);
  void OnSetUserAgentString(dbus::MethodCall* method_call,
                   dbus::ExportedObject::ResponseSender response_sender);

  void OnGetEncodedBundle(dbus::MethodCall* method_call,
                   dbus::ExportedObject::ResponseSender response_sender);
#endif

  void ListenForOwnerChange();
  void UnlistenForOwnerChange();
  void OnNameOwnerChanged(const std::string& service_owner);

  void OnLauncherDisappeared();

  void SendChannel(dbus::MethodCall* method_call,
                   dbus::ExportedObject::ResponseSender response_sender);

  scoped_refptr<dbus::Bus> bus_;
  std::string launcher_name_;
  dbus::Bus::GetServiceOwnerCallback owner_change_callback_;
  Application* application_;

  IPC::ChannelHandle ep_bp_channel_;
};

}  // namespace application
}  // namespace xwalk

#endif  // XWALK_APPLICATION_BROWSER_LINUX_RUNNING_APPLICATION_OBJECT_H_
