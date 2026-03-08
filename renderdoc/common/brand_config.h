#pragma once

#define RDOC_STRINGIZE2(a) #a
#define RDOC_STRINGIZE(a) RDOC_STRINGIZE2(a)

#define RDOC_WIDEN2(a) L##a
#define RDOC_WIDEN(a) RDOC_WIDEN2(a)

#define RDOC_PP_CONCAT2(a, b) RDOC_PP_CONCAT2_I(a, b)
#define RDOC_PP_CONCAT2_I(a, b) a##b
#define RDOC_PP_CONCAT3(a, b, c) RDOC_PP_CONCAT3_I(a, b, c)
#define RDOC_PP_CONCAT3_I(a, b, c) a##b##c

#define RDOC_BRAND_PRODUCT_NAME "ripperK"
#define RDOC_BRAND_REPLAY_BASE_NAME "ripperK"
#define RDOC_BRAND_CAPTURE_APP_BASE_NAME "ripperK_app"
#define RDOC_BRAND_BASE_NAME "ripperk"
#define RDOC_BRAND_BASE_NAME_UPPER "RIPPERK"
#define RDOC_BRAND_BASE_NAME_UPPER_TOKEN RIPPERK

#ifndef RDOC_BASE_NAME_UPPER
#define RDOC_BASE_NAME_UPPER RDOC_BRAND_BASE_NAME_UPPER_TOKEN
#endif

#define RDOC_BRAND_UI_NAME "qripperk"
#define RDOC_BRAND_UI_DISPLAY_NAME "QripperK"
#define RDOC_BRAND_UI_EXECUTABLE "qripperk.exe"
#define RDOC_BRAND_UI_INTERNAL_NAME "qripperk"
#define RDOC_BRAND_UI_STUB_NAME "ripperkui"
#define RDOC_BRAND_UI_STUB_EXECUTABLE "ripperkui.exe"

#define RDOC_BRAND_CMD_NAME "ripperkcmd"
#define RDOC_BRAND_CMD_DISPLAY_NAME "ripperKCmd"
#define RDOC_BRAND_CMD_EXECUTABLE "ripperkcmd.exe"
#define RDOC_BRAND_CMD_INTERNAL_NAME "ripperkcmd.exe"

#define RDOC_BRAND_UPGRADE_COMMAND "upgrade"
#define RDOC_BRAND_CRASH_HANDLE_COMMAND "crashhandle"
#define RDOC_BRAND_GLOBAL_HOOK_COMMAND "globalhook"

#define RDOC_BRAND_UPDATE_FAILED_OPTION "updatefailed"
#define RDOC_BRAND_UPDATE_DONE_ADMIN_OPTION "updatedone_admin"
#define RDOC_BRAND_UPDATE_DONE_OPTION "updatedone"
#define RDOC_BRAND_CRASH_OPTION "crash"

#define RDOC_BRAND_CORE_DLL_NAME "ripperk.dll"
#define RDOC_BRAND_CORE_INTERNAL_NAME "ripperk"

#define RDOC_BRAND_SHIM_NAME "ripperkshim"
#define RDOC_BRAND_SHIM_DLL_64 "ripperkshim64.dll"
#define RDOC_BRAND_SHIM_DLL_32 "ripperkshim32.dll"

#define RDOC_BRAND_GLOBAL_HOOK_DATA_NAME_64 "ripperKGlobalHookData64"
#define RDOC_BRAND_GLOBAL_HOOK_DATA_NAME_32 "ripperKGlobalHookData32"
#define RDOC_BRAND_GLOBAL_HOOK_RESTORE_FILENAME "ripperK_RestoreGlobalHook.reg"

#define RDOC_BRAND_TEMP_SUBFOLDER "ripperK"
#define RDOC_BRAND_APPDATA_SUBFOLDER "ripperk"
#define RDOC_BRAND_CAPTURE_PROGID "ripperK.RDCCapture.1"

#define RDOC_BRAND_CAPTURE_EXTENSION ".rdc"
#define RDOC_BRAND_CAPTURE_FILETYPE "rdc"
#define RDOC_BRAND_SETTINGS_EXTENSION ".cap"
#define RDOC_BRAND_SETTINGS_FILETYPE "cap"

#define RDOC_BRAND_CAPTURE_MAGIC_CHAR_0 'R'
#define RDOC_BRAND_CAPTURE_MAGIC_CHAR_1 'I'
#define RDOC_BRAND_CAPTURE_MAGIC_CHAR_2 'P'
#define RDOC_BRAND_CAPTURE_MAGIC_CHAR_3 'K'
#define RDOC_BRAND_LEGACY_CAPTURE_MAGIC_CHAR_0 'R'
#define RDOC_BRAND_LEGACY_CAPTURE_MAGIC_CHAR_1 'D'
#define RDOC_BRAND_LEGACY_CAPTURE_MAGIC_CHAR_2 'O'
#define RDOC_BRAND_LEGACY_CAPTURE_MAGIC_CHAR_3 'C'
#define RDOC_BRAND_READ_LEGACY_CAPTURE_MAGIC 1

#define RDOC_BRAND_SECTION_INTERNAL_PREFIX "ripperk/internal/"
#define RDOC_BRAND_SECTION_UI_PREFIX "ripperk/ui/"
#define RDOC_BRAND_SECTION_FRAMECAPTURE "ripperk/internal/framecapture"
#define RDOC_BRAND_SECTION_RESOLVEDB "ripperk/internal/resolvedb"
#define RDOC_BRAND_SECTION_BOOKMARKS "ripperk/ui/bookmarks"
#define RDOC_BRAND_SECTION_NOTES "ripperk/ui/notes"
#define RDOC_BRAND_SECTION_RESRENAMES "ripperk/ui/resrenames"
#define RDOC_BRAND_SECTION_EXTHUMB "ripperk/internal/exthumb"
#define RDOC_BRAND_SECTION_LOGFILE "ripperk/internal/logfile"
#define RDOC_BRAND_SECTION_EDITS "ripperk/ui/edits"
#define RDOC_BRAND_SECTION_D3D12CORE "ripperk/internal/d3d12core"
#define RDOC_BRAND_SECTION_D3D12SDKLAYERS "ripperk/internal/d3d12sdklayers"
#define RDOC_BRAND_SECTION_EMBEDDED_EXTERNALS "ripperk/internal/embeddedexternalfiles"

#define RDOC_BRAND_ENV_ORIGLIBPATH "RIPPERK_ORIGLIBPATH"
#define RDOC_BRAND_ENV_ORIGPRELOAD "RIPPERK_ORIGPRELOAD"
#define RDOC_BRAND_ENV_CAPFILE "RIPPERK_CAPFILE"
#define RDOC_BRAND_ENV_CAPOPTS "RIPPERK_CAPOPTS"
#define RDOC_BRAND_ENV_DEBUG_LOG_FILE "RIPPERK_DEBUG_LOG_FILE"
#define RDOC_BRAND_ENV_TEMP "RIPPERK_TEMP"
#define RDOC_BRAND_ENV_HOOK_EGL "RIPPERK_HOOK_EGL"
#define RDOC_BRAND_ENV_DEMOS_DATA "RIPPERK_DEMOS_DATA"
#define RDOC_BRAND_KEEP_LEGACY_POSIX_ENVS 0

#define RDOC_BRAND_POSIX_CONFIG_DIR ".ripperk"
#define RDOC_BRAND_POSIX_SHARE_DIR "ripperk"
#define RDOC_BRAND_SETTINGS_FILENAME "ripperk.conf"
#define RDOC_BRAND_REMOTE_SERVER_CONFIG_FILENAME "remoteserver.conf"
#define RDOC_BRAND_PLUGINS_SHARE_SUBDIR "share/ripperk/plugins"
#define RDOC_BRAND_ANDROID_PLUGINS_SHARE_SUBDIR "share/ripperk/plugins/android"
#define RDOC_BRAND_PYLIBS_SHARE_SUBDIR "share/ripperk/pylibs"

#define RDOC_BRAND_MIME_TYPE "application/x-ripperk-capture"
#define RDOC_BRAND_MIME_ICON_NAME "application-x-ripperk-capture"
#define RDOC_BRAND_DESKTOP_FILENAME "ripperk.desktop"
#define RDOC_BRAND_THUMBNAILER_FILENAME "ripperk.thumbnailer"
#define RDOC_BRAND_CAPTURE_XML_FILENAME "ripperk-capture.xml"
#define RDOC_BRAND_MENU_FILENAME "ripperk"
#define RDOC_BRAND_PIXMAP_ICON_16_FILENAME "ripperk-icon-16x16.xpm"
#define RDOC_BRAND_PIXMAP_ICON_32_FILENAME "ripperk-icon-32x32.xpm"

#define RDOC_BRAND_CRASH_EVENT_NAME "RIPPERK_CRASHHANDLE"
#define RDOC_BRAND_BREAKPAD_PIPE_PREFIX "ripperKBreakpadServer"
#define RDOC_BRAND_UPDATE_TEMPDIR "ripperKUpdate"

#define RDOC_BRAND_MANUFACTURER_NAME "Baldur Karlsson"
#define RDOC_BRAND_WEBSITE_URL "https://renderdoc.org"
#define RDOC_BRAND_DOCUMENTATION_TITLE "ripperK Documentation"
#define RDOC_BRAND_DOCUMENTATION_HTML_TITLE "ripperK documentation"
#define RDOC_BRAND_DOCUMENTATION_COLLECTION_NAME "ripperK"
#define RDOC_BRAND_DOCUMENTATION_BASENAME "ripperk"
#define RDOC_BRAND_DOCUMENTATION_FILE "ripperk.chm"
#define RDOC_BRAND_DOCUMENTATION_URL "https://renderdoc.org/docs"
#define RDOC_BRAND_PYTHON_API_URL "https://renderdoc.org/docs/python_api/index.html"
#define RDOC_BRAND_IN_APPLICATION_API_URL "https://renderdoc.org/docs/in_application_api.html"
#define RDOC_BRAND_BUILDS_URL "https://renderdoc.org/builds"
#define RDOC_BRAND_UPDATE_URL_TEMPLATE "https://renderdoc.org/getupdateurl/%1/%2?htmlnotes=1"
#define RDOC_BRAND_TIPS_URL_TEMPLATE "https://renderdoc.org/tips/%1"
#define RDOC_BRAND_SUPPORT_EMAIL "baldurk@baldurk.org"
#define RDOC_BRAND_SOURCE_URL "https://github.com/baldurk/renderdoc"
#define RDOC_BRAND_ISSUES_URL "https://github.com/baldurk/renderdoc/issues"
#define RDOC_BRAND_CONTRIB_URL "https://github.com/baldurk/renderdoc-contrib"

#define RDOC_BRAND_PY_CORE_MODULE_NAME "ripperk"
#define RDOC_BRAND_PY_GUI_MODULE_NAME "qripperk"
#define RDOC_BRAND_KEEP_LEGACY_PYTHON_ALIASES 0
#define RDOC_BRAND_KEEP_LEGACY_ABI 1
#define RDOC_BRAND_LINUX_CORE_LIBRARY "libripperk.so"
#define RDOC_BRAND_APPLE_CORE_LIBRARY "libripperk.dylib"
#define RDOC_BRAND_ANDROID_CAPTURE_LIBRARY "libVkLayer_GLES_ripperK.so"

#define RDOC_BRAND_VULKAN_LAYER_NAME "VK_LAYER_RIPPERK_Capture"
#define RDOC_BRAND_VULKAN_CAPTURE_VAR "ENABLE_VULKAN_RIPPERK_CAPTURE"
#define RDOC_BRAND_VULKAN_LAYER_DESCRIPTION "Debugging capture layer for ripperK"
#define RDOC_BRAND_VULKAN_FORCED_INSTANCE_NAME "ripperK forced instance"
#define RDOC_BRAND_VULKAN_TOOL_NAME "ripperK"
#define RDOC_BRAND_VULKAN_TOOL_DESCRIPTION "Debugging capture layer for ripperK"
#define RDOC_BRAND_VULKAN_CAPTURE_APP_NAME "ripperK Capturing App"
#define RDOC_BRAND_VULKAN_ENGINE_NAME "ripperK"
#define RDOC_BRAND_VULKAN_LAYER_FUNC_PREFIX RDOC_PP_CONCAT3(VK_LAYER_, RDOC_BASE_NAME_UPPER, _Capture)
#define RDOC_BRAND_VULKAN_LAYER_FUNC(name) RDOC_PP_CONCAT2(RDOC_BRAND_VULKAN_LAYER_FUNC_PREFIX, name)

#define RDOC_BRAND_ANDROID_LIBRARY "libVkLayer_GLES_ripperK.so"
#define RDOC_BRAND_ANDROID_PACKAGE_BASE "org.ripperk.ripperkcmd"
#define RDOC_BRAND_CMD_ANDROID_EXTRA_KEY "ripperkcmd"
#define RDOC_BRAND_ANDROID_PROPERTY_PREFIX "debug.ripperk"
#define RDOC_BRAND_ANDROID_AUTOGRANT_PERMISSIONS_PROPERTY "debug.ripperk.autograntpermissions"
#define RDOC_BRAND_ANDROID_IGNORE_LAYERS_PROPERTY "debug.ripperk.IGNORE_LAYERS"
#define RDOC_BRAND_ANDROID_CAPTURE_OPTIONS_PROPERTY "debug.ripperk.RIPPERK_CAPOPTS"
#define RDOC_BRAND_ANDROID_SOCKET_PREFIX "ripperk"

#define RDOC_BRAND_BUGREPORT_URL "https://renderdoc.org/bugreporter"
#define RDOC_BRAND_ANALYTICS_URL "https://renderdoc.org/analytics"
