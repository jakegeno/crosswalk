// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <glib.h>
#include "xwalk/application/browser/application_system_tizen.h"

#include <cstdio>
#include <string>

#include <appcore/appcore-common.h>  // NOLINT
#include <pkgmgr-info.h> // NOLINT

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "net/base/filename_util.h"
#include "xwalk/application/browser/application_service_tizen.h"
#include "xwalk/application/common/application_manifest_constants.h"
#include "xwalk/application/common/id_util.h"
#include "xwalk/application/extension/application_runtime_extension.h"
#include "xwalk/application/extension/application_widget_extension.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/common/xwalk_switches.h"

#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_glib.h"

enum app_event {
  AE_UNKNOWN,
  AE_CREATE,
  AE_TERMINATE,
  AE_PAUSE,
  AE_RESUME,
  AE_RESET,
  AE_LOWMEM_POST,
  AE_MEM_FLUSH,
  AE_MAX
};

// Private struct from appcore-internal, necessary to get events from
// the system.
struct ui_ops {
  void* data;
  void (*cb_app)(enum app_event evnt, void* data, bundle* b);
};

static ui_ops appcore_ops;

namespace xwalk {
namespace application {

ApplicationSystemTizen::ApplicationSystemTizen(XWalkBrowserContext* context)
  : ApplicationSystem(context) {
}

ApplicationSystemTizen::~ApplicationSystemTizen() {
}

namespace {

void application_event_cb(app_event event, void* data, bundle* b) {
  LOG(INFO) << "Received Tizen appcore event: " << event;
  ApplicationSystemTizen* app_system =
      reinterpret_cast<ApplicationSystemTizen*>(data);
  CHECK(app_system);
  if (event == AE_RESET) {
    const std::string path = app_system->launch_cmd_params->argv()[0];
    base::FilePath exec_path(path);

    std::string app_id = exec_path.BaseName().MaybeAsASCII();
    LOG(INFO) << "Attempting to launch the app with id: " << app_id;
    if (!IsValidApplicationID(app_id))
      return;

    ApplicationServiceTizen* app_service_tizen =
        ToApplicationServiceTizen(app_system->application_service_.get());
    Application* app = app_service_tizen->LaunchFromAppID(
        app_id, launch_params(*app_system->launch_cmd_params));

    app_system->current_app = static_cast<ApplicationTizen*>(app);

    if (!app)
      app_system->current_app->Terminate();
  }

  switch (event) {
    case AE_UNKNOWN:
    case AE_CREATE:
      break;
    case AE_TERMINATE:
      if (app_system->current_app)
        app_system->current_app->Terminate();
      break;
    case AE_PAUSE:
      if (app_system->current_app)
        app_system->current_app->Suspend();
      break;
    case AE_RESUME:
      if (app_system->current_app)
        app_system->current_app->Resume();
      break;
    case AE_RESET:
      break;
    case AE_LOWMEM_POST:
    case AE_MEM_FLUSH:
    case AE_MAX:
      break;
  }
}

}  // namespace

bool ApplicationSystemTizen::LaunchFromCommandLine(
    const base::CommandLine& cmd_line, const GURL& url) {
  fprintf(stderr, "Launch from cmd line\n");
  // Handles raw app_id passed as first non-switch argument.
  const base::CommandLine::StringVector& args = cmd_line.argv();
  CHECK(!args.empty());
  launch_cmd_params = const_cast<base::CommandLine*>(&cmd_line);
  base::FilePath exec_path(args[0]);

  std::string app_id = exec_path.BaseName().MaybeAsASCII();
  LOG(INFO) << "Attempting to launch the app with id: " << app_id;
  if (!IsValidApplicationID(app_id))
    return false;

  ApplicationServiceTizen* app_service_tizen =
      ToApplicationServiceTizen(application_service_.get());

  Application* app = app_service_tizen->LaunchFromAppID(app_id);

  if (!app)
    return false;


  const std::string& name = std::string("xwalk-") + app_id;
  appcore_ops.cb_app = application_event_cb;
  appcore_ops.data = this;

  // pass only positional arguments to appcore (possible aul parameters)
  // skip chromium flags
  size_t positionals = 0;
  size_t size = args.size() + 1;
  scoped_ptr<char*[]> argv(new char*[size]);
  memset(argv.get(), 0x0, size);
  for (size_t i = 0; i < size - 1; ++i) {
    if (!StartsWithASCII(args[i], "--", false))
      argv[positionals++] = const_cast<char*>(args[i].c_str());
  }

  if (appcore_init(name.c_str(), &appcore_ops, positionals, argv.get())) {
    LOG(ERROR) << "Failed to initialize appcore";
    app->Terminate();
    return false;
  }

  return true;
}

}  // namespace application
}  // namespace xwalk
