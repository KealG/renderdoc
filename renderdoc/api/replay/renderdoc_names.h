/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2026 Baldur Karlsson
 * Copyright (c) 2014 Crytek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#define RENDERDOC_WIDEN2(str) L##str
#define RENDERDOC_WIDEN(str) RENDERDOC_WIDEN2(str)

#define RENDERDOC_PRODUCT_NAME "ripperK"
#define QRENDERDOC_PRODUCT_NAME "qripperK"

#define RENDERDOC_BASE_BASENAME "ripperk"
#define QRENDERDOC_BASENAME "qripperk"
#define RENDERDOC_UI_STUB_BASENAME "ripperkui"
#define RENDERDOC_CMD_BASENAME "ripperkcmd"
#define RENDERDOC_SHIM_BASENAME "ripperkshim"

#define RENDERDOC_CORE_DLL RENDERDOC_BASE_BASENAME ".dll"
#define QRENDERDOC_EXE QRENDERDOC_BASENAME ".exe"
#define RENDERDOC_UI_STUB_EXE RENDERDOC_UI_STUB_BASENAME ".exe"
#define RENDERDOC_CMD_EXE RENDERDOC_CMD_BASENAME ".exe"
#define RENDERDOC_SHIM_32_DLL RENDERDOC_SHIM_BASENAME "32.dll"
#define RENDERDOC_SHIM_64_DLL RENDERDOC_SHIM_BASENAME "64.dll"

#define QRENDERDOC_EXE_W RENDERDOC_WIDEN(QRENDERDOC_EXE)
#define RENDERDOC_CMD_EXE_W RENDERDOC_WIDEN(RENDERDOC_CMD_EXE)
#define RENDERDOC_SHIM_32_DLL_W RENDERDOC_WIDEN(RENDERDOC_SHIM_32_DLL)
#define RENDERDOC_SHIM_64_DLL_W RENDERDOC_WIDEN(RENDERDOC_SHIM_64_DLL)

#define RENDERDOC_REPLAY_MARKER_SYMBOL ripperk__replay__marker
#define RENDERDOC_REPLAY_MARKER_NAME "ripperk__replay__marker"

#define RENDERDOC_CRASHHANDLE_EVENT "RIPPERK_CRASHHANDLE"
#define RENDERDOC_BREAKPAD_PIPE_PREFIX "\\\\.\\pipe\\ripperKBreakpadServer"

#define RENDERDOC_GLOBAL_HOOK_DATA_32 "ripperKGlobalHookData32"
#define RENDERDOC_GLOBAL_HOOK_DATA_64 "ripperKGlobalHookData64"

#define RENDERDOC_CAPTURE_DIRECTORY "ripperK"
#define RENDERDOC_CAPTURE_FILE_ASSOC "ripperK.RDCCapture.1"
#define RENDERDOC_UPDATE_DIRECTORY "ripperKUpdate"
