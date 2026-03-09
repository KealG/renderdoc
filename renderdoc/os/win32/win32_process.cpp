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

// must be separate so that it's included first and not sorted by clang-format
#include <windows.h>

#include <Psapi.h>
#include <tchar.h>
#include <tlhelp32.h>
#include "common/formatting.h"
#include "common/launch_inject_mode.h"
#include "core/core.h"
#include "os/os_specific.h"
#include "strings/string_utils.h"

#include <string>

static rdcarray<EnvironmentModification> &GetEnvModifications()
{
  static rdcarray<EnvironmentModification> envCallbacks;
  return envCallbacks;
}

struct InsensitiveComparison
{
  bool operator()(const rdcstr &a, const rdcstr &b) const { return strlower(a) < strlower(b); }
};

typedef std::map<rdcstr, rdcstr, InsensitiveComparison> EnvMap;

static EnvMap EnvStringToEnvMap(const wchar_t *envstring)
{
  EnvMap ret;

  const wchar_t *e = envstring;

  while(*e)
  {
    const wchar_t *equals = wcschr(e, L'=');

    rdcstr name = StringFormat::Wide2UTF8(rdcwstr(e, equals - e));
    rdcstr value = StringFormat::Wide2UTF8(equals + 1);

    ret[name] = value;

    // jump to \0 and past it
    e += wcslen(e) + 1;
  }

  return ret;
}

void Process::RegisterEnvironmentModification(const EnvironmentModification &modif)
{
  GetEnvModifications().push_back(modif);
}

static void ApplyEnvModifications(EnvMap &envValues,
                                  const rdcarray<EnvironmentModification> &modifications,
                                  bool setToSystem)
{
  for(size_t i = 0; i < modifications.size(); i++)
  {
    const EnvironmentModification &m = modifications[i];

    rdcstr value;

    auto it = envValues.find(m.name);
    if(it != envValues.end())
      value = it->second;

    switch(m.mod)
    {
      case EnvMod::Set: value = m.value.c_str(); break;
      case EnvMod::Append:
      {
        if(!value.empty())
        {
          if(m.sep == EnvSep::Platform || m.sep == EnvSep::SemiColon)
            value += ";";
          else if(m.sep == EnvSep::Colon)
            value += ":";
        }
        value += m.value.c_str();
        break;
      }
      case EnvMod::Prepend:
      {
        if(!value.empty())
        {
          rdcstr prep = m.value;
          if(m.sep == EnvSep::Platform || m.sep == EnvSep::SemiColon)
            prep += ";";
          else if(m.sep == EnvSep::Colon)
            prep += ":";
          value = prep + value;
        }
        else
        {
          value = m.value.c_str();
        }
        break;
      }
    }

    envValues[m.name] = value;

    if(setToSystem)
      SetEnvironmentVariableW(StringFormat::UTF82Wide(m.name).c_str(),
                              StringFormat::UTF82Wide(value).c_str());
  }
}

// on windows we apply environment changes here, after process initialisation
// but before any real work (in RenderDoc::Initialise) so that we support
// injecting the dll into processes we didn't launch (ie didn't control the
// starting environment for), or even the application loading the dll itself
// without any interaction with our replay app.
void Process::ApplyEnvironmentModification()
{
  // turn environment string to a UTF-8 map
  LPWCH envStrings = GetEnvironmentStringsW();
  EnvMap envValues = EnvStringToEnvMap(envStrings);
  FreeEnvironmentStringsW(envStrings);
  rdcarray<EnvironmentModification> &modifications = GetEnvModifications();

  ApplyEnvModifications(envValues, modifications, true);

  // these have been applied to the current process
  modifications.clear();
}

rdcstr Process::GetEnvVariable(const rdcstr &name)
{
  DWORD len = GetEnvironmentVariableA(name.c_str(), NULL, 0);
  if(len == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
    return rdcstr();

  rdcstr ret;
  ret.resize(len + 1);

  GetEnvironmentVariableA(name.c_str(), ret.data(), len);
  ret.trim();
  return ret;
}

uint64_t Process::GetMemoryUsage()
{
  HANDLE proc = GetCurrentProcess();

  if(proc == NULL)
  {
    RDCERR("Couldn't open process: %d", GetLastError());
    return 0;
  }

  PROCESS_MEMORY_COUNTERS memInfo = {};

  uint64_t ret = 0;

  if(GetProcessMemoryInfo(proc, &memInfo, sizeof(memInfo)))
  {
    ret = memInfo.WorkingSetSize;
  }
  else
  {
    RDCERR("Couldn't get process memory info: %d", GetLastError());
  }

  return ret;
}

// helpers for various shims and dlls etc, not part of the public API
extern "C" __declspec(dllexport) void __cdecl INTERNAL_GetTargetControlIdent(uint32_t *ident)
{
  if(ident)
    *ident = RenderDoc::Inst().GetTargetControlIdent();
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetCaptureOptions(CaptureOptions *opts)
{
  if(opts)
    RenderDoc::Inst().SetCaptureOptions(*opts);
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetCaptureFile(const char *capfile)
{
  if(capfile)
    RenderDoc::Inst().SetCaptureFileTemplate(capfile);
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetDebugLogFile(const char *logfile)
{
  RENDERDOC_SetDebugLogFile(logfile ? logfile : rdcstr());
}

static EnvironmentModification tempEnvMod;

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvModName(const char *name)
{
  if(name)
    tempEnvMod.name = name;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvModValue(const char *value)
{
  if(value)
    tempEnvMod.value = value;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvSep(EnvSep *sep)
{
  if(sep)
    tempEnvMod.sep = *sep;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvMod(EnvMod *mod)
{
  if(mod)
  {
    tempEnvMod.mod = *mod;
    Process::RegisterEnvironmentModification(tempEnvMod);
  }
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_ApplyEnvMods(void *ignored)
{
  Process::ApplyEnvironmentModification();
}

static thread_local LaunchInjectMode tl_LaunchInjectMode = LaunchInjectMode::Automatic;

static LaunchInjectMode SanitiseLaunchInjectMode(uint32_t mode)
{
  if(mode >= (uint32_t)LaunchInjectMode::Count)
    return LaunchInjectMode::Automatic;

  return (LaunchInjectMode)mode;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetLaunchInjectMode(uint32_t mode)
{
  tl_LaunchInjectMode = SanitiseLaunchInjectMode(mode);
}

static LaunchInjectMode ConsumeLaunchInjectMode()
{
  LaunchInjectMode mode = tl_LaunchInjectMode;
  tl_LaunchInjectMode = LaunchInjectMode::Automatic;
  return mode;
}

static const char *LaunchInjectModeName(LaunchInjectMode mode)
{
  switch(mode)
  {
    case LaunchInjectMode::Automatic: return "Automatic";
    case LaunchInjectMode::SuspendedOnly: return "Suspended Only";
    case LaunchInjectMode::ResumeRetry: return "Resume & Retry";
    case LaunchInjectMode::LateAttach: return "Late Attach";
    default: break;
  }

  return "Automatic";
}

struct DLLInjectionResult
{
  bool success = false;
  DWORD error = ERROR_SUCCESS;
  uintptr_t loadResult = 0;
  uintptr_t confirmedModuleHandle = 0;
  DWORD loaderError = ERROR_SUCCESS;
  bool usedSearchDirFallback = false;
};

struct RemoteDLLLoadParams
{
  uintptr_t loadLibraryExW = 0;
  uintptr_t getLastError = 0;
  uintptr_t getModuleHandleW = 0;
  DWORD flags = 0;
  DWORD padding0 = 0;
  uintptr_t loadResult = 0;
  uintptr_t moduleHandle = 0;
  DWORD lastError = 0;
  DWORD padding1 = 0;
  wchar_t dllPath[MAX_PATH + 1] = {};
  wchar_t dllName[MAX_PATH + 1] = {};
};

static bool IsProcessActive(HANDLE hProcess)
{
  if(hProcess == NULL)
    return false;

  DWORD exitCode = 0;

  if(!GetExitCodeProcess(hProcess, &exitCode))
    return false;

  return exitCode == STILL_ACTIVE;
}

static void ResumeThreadCompletely(HANDLE hThread)
{
  if(hThread == NULL)
    return;

  for(int resumeAttempt = 0; resumeAttempt < 8; resumeAttempt++)
  {
    DWORD suspendCount = ResumeThread(hThread);

    if(suspendCount == (DWORD)-1)
    {
      DWORD err = GetLastError();

      if(err != ERROR_SUCCESS)
        RDCWARN("ResumeThread failed: %u", err);

      break;
    }

    if(suspendCount <= 1)
      break;
  }
}

static void PatchStubImmediate(uint8_t *stub, size_t offset, uint32_t value)
{
  memcpy(stub + offset, &value, sizeof(value));
}

static DLLInjectionResult RunRemoteLoadLibrary(HANDLE hProcess, const rdcwstr &libName,
                                               DWORD flags)
{
  DLLInjectionResult result;

  HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

  if(kernel32 == NULL)
  {
    result.error = GetLastError();
    return result;
  }

  FARPROC loadLibraryExW = GetProcAddress(kernel32, "LoadLibraryExW");
  FARPROC getLastError = GetProcAddress(kernel32, "GetLastError");
  FARPROC getModuleHandleW = GetProcAddress(kernel32, "GetModuleHandleW");

  if(loadLibraryExW == NULL || getLastError == NULL || getModuleHandleW == NULL)
  {
    result.error = GetLastError();
    return result;
  }

  RemoteDLLLoadParams params;
  params.loadLibraryExW = (uintptr_t)loadLibraryExW;
  params.getLastError = (uintptr_t)getLastError;
  params.getModuleHandleW = (uintptr_t)getModuleHandleW;
  params.flags = flags;

  wcsncpy_s(params.dllPath, ARRAY_COUNT(params.dllPath), libName.c_str(), _TRUNCATE);

  rdcstr dllName = get_basename(StringFormat::Wide2UTF8(libName));
  rdcwstr wdllName = StringFormat::UTF82Wide(dllName);
  wcsncpy_s(params.dllName, ARRAY_COUNT(params.dllName), wdllName.c_str(), _TRUNCATE);

#if ENABLED(RDOC_X64)
  uint8_t stub[] = {
      0x48, 0x83, 0xEC, 0x28, 0x49, 0x89, 0xCA, 0x49, 0x8D, 0x8A, 0, 0, 0, 0,
      0x31, 0xD2, 0x45, 0x8B, 0x82, 0, 0, 0, 0, 0x49, 0x8B, 0x82, 0, 0, 0, 0,
      0xFF, 0xD0, 0x49, 0x89, 0x82, 0, 0, 0, 0, 0x49, 0x8B, 0x82, 0, 0, 0, 0,
      0xFF, 0xD0, 0x41, 0x89, 0x82, 0, 0, 0, 0, 0x49, 0x8D, 0x8A, 0, 0, 0, 0,
      0x49, 0x8B, 0x82, 0, 0, 0, 0, 0xFF, 0xD0, 0x49, 0x89, 0x82, 0, 0, 0, 0,
      0x31, 0xC0, 0x48, 0x83, 0xC4, 0x28, 0xC3,
  };

  PatchStubImmediate(stub, 10, (uint32_t)offsetof(RemoteDLLLoadParams, dllPath));
  PatchStubImmediate(stub, 19, (uint32_t)offsetof(RemoteDLLLoadParams, flags));
  PatchStubImmediate(stub, 26, (uint32_t)offsetof(RemoteDLLLoadParams, loadLibraryExW));
  PatchStubImmediate(stub, 35, (uint32_t)offsetof(RemoteDLLLoadParams, loadResult));
  PatchStubImmediate(stub, 42, (uint32_t)offsetof(RemoteDLLLoadParams, getLastError));
  PatchStubImmediate(stub, 51, (uint32_t)offsetof(RemoteDLLLoadParams, lastError));
  PatchStubImmediate(stub, 58, (uint32_t)offsetof(RemoteDLLLoadParams, dllName));
  PatchStubImmediate(stub, 65, (uint32_t)offsetof(RemoteDLLLoadParams, getModuleHandleW));
  PatchStubImmediate(stub, 74, (uint32_t)offsetof(RemoteDLLLoadParams, moduleHandle));
#else
  uint8_t stub[] = {
      0x8B, 0x54, 0x24, 0x04, 0xFF, 0xB2, 0, 0, 0, 0, 0x6A, 0x00, 0x8D,
      0x82, 0, 0, 0, 0, 0x50, 0x8B, 0x82, 0, 0, 0, 0, 0xFF, 0xD0, 0x89,
      0x82, 0, 0, 0, 0, 0x8B, 0x82, 0, 0, 0, 0, 0xFF, 0xD0, 0x89, 0x82,
      0, 0, 0, 0, 0x8D, 0x82, 0, 0, 0, 0, 0x50, 0x8B, 0x82, 0, 0, 0, 0,
      0xFF, 0xD0, 0x89, 0x82, 0, 0, 0, 0, 0x31, 0xC0, 0xC2, 0x04, 0x00,
  };

  PatchStubImmediate(stub, 6, (uint32_t)offsetof(RemoteDLLLoadParams, flags));
  PatchStubImmediate(stub, 14, (uint32_t)offsetof(RemoteDLLLoadParams, dllPath));
  PatchStubImmediate(stub, 21, (uint32_t)offsetof(RemoteDLLLoadParams, loadLibraryExW));
  PatchStubImmediate(stub, 29, (uint32_t)offsetof(RemoteDLLLoadParams, loadResult));
  PatchStubImmediate(stub, 35, (uint32_t)offsetof(RemoteDLLLoadParams, getLastError));
  PatchStubImmediate(stub, 43, (uint32_t)offsetof(RemoteDLLLoadParams, lastError));
  PatchStubImmediate(stub, 49, (uint32_t)offsetof(RemoteDLLLoadParams, dllName));
  PatchStubImmediate(stub, 56, (uint32_t)offsetof(RemoteDLLLoadParams, getModuleHandleW));
  PatchStubImmediate(stub, 64, (uint32_t)offsetof(RemoteDLLLoadParams, moduleHandle));
#endif

  void *remoteParams =
      VirtualAllocEx(hProcess, NULL, sizeof(params), MEM_COMMIT, PAGE_READWRITE);
  void *remoteStub =
      VirtualAllocEx(hProcess, NULL, sizeof(stub), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

  if(remoteParams == NULL || remoteStub == NULL)
  {
    result.error = GetLastError();
    if(remoteParams)
      VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    if(remoteStub)
      VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  SIZE_T numWritten = 0;

  if(!WriteProcessMemory(hProcess, remoteParams, &params, sizeof(params), &numWritten) ||
     numWritten != sizeof(params))
  {
    result.error = GetLastError();
    VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  if(!WriteProcessMemory(hProcess, remoteStub, stub, sizeof(stub), &numWritten) ||
     numWritten != sizeof(stub))
  {
    result.error = GetLastError();
    VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  HANDLE hThread = CreateRemoteThread(hProcess, NULL, 1024 * 1024U,
                                      (LPTHREAD_START_ROUTINE)remoteStub, remoteParams, 0, NULL);

  if(hThread == NULL)
  {
    result.error = GetLastError();
    VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

  if(waitResult != WAIT_OBJECT_0)
  {
    result.error = GetLastError();
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  CloseHandle(hThread);

  SIZE_T numRead = 0;

  if(!ReadProcessMemory(hProcess, remoteParams, &params, sizeof(params), &numRead) ||
     numRead != sizeof(params))
  {
    result.error = GetLastError();
    VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
    return result;
  }

  result.loadResult = params.loadResult;
  result.confirmedModuleHandle = params.moduleHandle;
  result.loaderError = params.lastError;
  result.success = (params.loadResult != 0);

  VirtualFreeEx(hProcess, remoteParams, 0, MEM_RELEASE);
  VirtualFreeEx(hProcess, remoteStub, 0, MEM_RELEASE);
  return result;
}

static DLLInjectionResult InjectDLLOnce(HANDLE hProcess, const rdcwstr &libName)
{
  DLLInjectionResult result = RunRemoteLoadLibrary(hProcess, libName, 0);

  if(result.error == ERROR_SUCCESS && result.loadResult == 0)
    RDCWARN("LoadLibraryExW for '%ls' returned NULL (remote error %u)", libName.c_str(),
            result.loaderError);

  return result;
}

static DLLInjectionResult InjectDLLWithSearchDirFallback(HANDLE hProcess, const rdcwstr &libName)
{
  DLLInjectionResult result =
      RunRemoteLoadLibrary(hProcess, libName, LOAD_WITH_ALTERED_SEARCH_PATH);
  result.usedSearchDirFallback = true;

  if(result.error == ERROR_SUCCESS && result.loadResult == 0)
  {
    RDCWARN("LoadLibraryExW(LOAD_WITH_ALTERED_SEARCH_PATH) for '%ls' returned NULL (remote error "
            "%u)",
            libName.c_str(), result.loaderError);
  }

  return result;
}

static DLLInjectionResult InjectDLL(HANDLE hProcess, const rdcwstr &libName, uint32_t attempts,
                                    uint32_t delayMilliseconds)
{
  DLLInjectionResult result;

  for(uint32_t attempt = 0; attempt < attempts; attempt++)
  {
    result = InjectDLLOnce(hProcess, libName);

    if(!result.success && result.error == ERROR_SUCCESS)
    {
      DLLInjectionResult fallback = InjectDLLWithSearchDirFallback(hProcess, libName);

      if(fallback.usedSearchDirFallback)
        result = fallback;
      else if(result.error == ERROR_SUCCESS && fallback.error != ERROR_SUCCESS)
        result.error = fallback.error;
    }

    if(result.success)
      return result;

    if(attempt + 1 < attempts && delayMilliseconds > 0)
      Sleep(delayMilliseconds);
  }

  return result;
}

uintptr_t FindRemoteDLL(DWORD pid, rdcstr libName, bool logFailure)
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;

  rdcwstr wlibName = StringFormat::UTF82Wide(strlower(libName));

  // up to 10 retries
  for(int i = 0; i < 10; i++)
  {
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

    if(hModuleSnap == INVALID_HANDLE_VALUE)
    {
      DWORD err = GetLastError();

      RDCWARN("CreateToolhelp32Snapshot(%u) -> 0x%08x", pid, err);

      // retry if error is ERROR_BAD_LENGTH
      if(err == ERROR_BAD_LENGTH)
        continue;
    }

    // didn't retry, or succeeded
    break;
  }

  if(hModuleSnap == INVALID_HANDLE_VALUE)
  {
    RDCERR("Couldn't create toolhelp dump of modules in process %u", pid);
    return 0;
  }

  MODULEENTRY32 me32;
  RDCEraseEl(me32);
  me32.dwSize = sizeof(MODULEENTRY32);

  BOOL success = Module32First(hModuleSnap, &me32);

  if(success == FALSE)
  {
    DWORD err = GetLastError();

    RDCERR("Couldn't get first module in process %u: 0x%08x", pid, err);
    CloseHandle(hModuleSnap);
    return 0;
  }

  uintptr_t ret = 0;

  int numModules = 0;

  do
  {
    wchar_t modnameLower[MAX_MODULE_NAME32 + 1];
    RDCEraseEl(modnameLower);
    wcsncpy_s(modnameLower, me32.szModule, MAX_MODULE_NAME32);

    wchar_t *wc = &modnameLower[0];
    while(*wc)
    {
      *wc = towlower(*wc);
      wc++;
    }

    numModules++;

    if(wcsstr(modnameLower, wlibName.c_str()) == modnameLower)
    {
      ret = (uintptr_t)me32.modBaseAddr;
    }
  } while(ret == 0 && Module32Next(hModuleSnap, &me32));

  if(ret == 0)
  {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

    DWORD exitCode = 0;

    if(h)
      GetExitCodeProcess(h, &exitCode);

    if(logFailure && (h == NULL || exitCode != STILL_ACTIVE))
    {
      RDCERR(
          "Error injecting into remote process with PID %u which is no longer available.\n"
          "Possibly the process has crashed during early startup, or is missing DLLs to run?",
          pid);
    }
    else if(logFailure)
    {
      RDCERR("Couldn't find module '%s' among %d modules", libName.c_str(), numModules);
    }

    if(h)
      CloseHandle(h);
  }

  CloseHandle(hModuleSnap);

  return ret;
}

static rdcstr DescribeMitigationPolicies(HANDLE hProcess)
{
  typedef BOOL(WINAPI *PFN_GetProcessMitigationPolicy)(HANDLE, PROCESS_MITIGATION_POLICY, PVOID,
                                                       SIZE_T);

  static PFN_GetProcessMitigationPolicy getPolicy = NULL;
  static bool lookedUp = false;

  if(!lookedUp)
  {
    lookedUp = true;
    getPolicy = (PFN_GetProcessMitigationPolicy)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                               "GetProcessMitigationPolicy");
  }

  if(getPolicy == NULL)
    return "";

  rdcarray<rdcstr> notes;

  PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature;
  RDCEraseEl(signature);

  if(getPolicy(hProcess, ProcessSignaturePolicy, &signature, sizeof(signature)))
  {
    if(signature.MicrosoftSignedOnly)
      notes.push_back("the target only allows Microsoft-signed DLLs");
    if(signature.StoreSignedOnly)
      notes.push_back("the target only allows Store-signed DLLs");
    if(signature.MitigationOptIn)
      notes.push_back("the target opted into stronger binary signature enforcement");
  }

  PROCESS_MITIGATION_IMAGE_LOAD_POLICY imageLoad;
  RDCEraseEl(imageLoad);

  if(getPolicy(hProcess, ProcessImageLoadPolicy, &imageLoad, sizeof(imageLoad)))
  {
    if(imageLoad.NoRemoteImages)
      notes.push_back("the target blocks loading remote images");
    if(imageLoad.NoLowMandatoryLabelImages)
      notes.push_back("the target blocks loading low-integrity images");
    if(imageLoad.PreferSystem32Images)
      notes.push_back("the target prefers loading DLLs from System32");
  }

  PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCode;
  RDCEraseEl(dynamicCode);

  if(getPolicy(hProcess, ProcessDynamicCodePolicy, &dynamicCode, sizeof(dynamicCode)) &&
     dynamicCode.ProhibitDynamicCode)
  {
    notes.push_back("the target prohibits dynamically generated code");
  }

  PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extensionPoints;
  RDCEraseEl(extensionPoints);

  if(getPolicy(hProcess, ProcessExtensionPointDisablePolicy, &extensionPoints,
               sizeof(extensionPoints)) &&
     extensionPoints.DisableExtensionPoints)
  {
    notes.push_back("the target disables extension-point style DLL injection");
  }

  if(notes.empty())
    return "";

  rdcstr ret = "\n\nTarget mitigation policies detected: ";

  for(size_t i = 0; i < notes.size(); i++)
  {
    if(i > 0)
      ret += i + 1 == notes.size() ? "; and " : "; ";

    ret += notes[i];
  }

  ret += ".";
  return ret;
}

static rdcstr DescribeRemoteLoadFailure(DWORD loaderError)
{
  switch(loaderError)
  {
    case ERROR_MOD_NOT_FOUND:
      return " A dependency of the capture DLL could not be found in the target's loader search path.";
    case ERROR_PROC_NOT_FOUND:
      return " A dependent DLL was found, but an imported procedure was missing, which usually indicates a version mismatch.";
    case ERROR_BAD_EXE_FORMAT:
      return " A DLL loaded by the target has the wrong architecture, or is otherwise not a valid image for this process.";
    case ERROR_DLL_INIT_FAILED:
      return " A DLL initialisation routine failed inside the target process during startup.";
    case ERROR_ACCESS_DENIED:
      return " The target process, Windows policy, or security software denied the load attempt.";
#ifdef ERROR_INVALID_IMAGE_HASH
    case ERROR_INVALID_IMAGE_HASH:
      return " Windows code integrity rejected the image, often due to signature or security policy enforcement.";
#endif
#ifdef ERROR_DYNAMIC_CODE_BLOCKED
    case ERROR_DYNAMIC_CODE_BLOCKED:
      return " The target blocked execution of dynamically generated code, which can prevent the remote loader stub from running.";
#endif
    default: break;
  }

  return "";
}

struct ChildProcessInfo
{
  uint32_t pid = 0;
  rdcstr description;
};

static rdcarray<ChildProcessInfo> EnumerateLiveChildProcesses(uint32_t parentPID)
{
  rdcarray<ChildProcessInfo> ret;

  HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if(hProcessSnap == INVALID_HANDLE_VALUE)
    return ret;

  PROCESSENTRY32W pe32;
  RDCEraseEl(pe32);
  pe32.dwSize = sizeof(pe32);

  if(Process32FirstW(hProcessSnap, &pe32))
  {
    do
    {
      if(pe32.th32ParentProcessID != parentPID)
        continue;

      bool alive = true;
      HANDLE hChild = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);

      if(hChild)
      {
        DWORD exitCode = 0;
        if(GetExitCodeProcess(hChild, &exitCode))
          alive = (exitCode == STILL_ACTIVE);
        CloseHandle(hChild);
      }

      if(!alive)
        continue;

      ChildProcessInfo child;
      child.pid = pe32.th32ProcessID;
      child.description =
          StringFormat::Fmt("%s (%u)", StringFormat::Wide2UTF8(pe32.szExeFile).c_str(), child.pid);
      ret.push_back(child);
    } while(Process32NextW(hProcessSnap, &pe32));
  }

  CloseHandle(hProcessSnap);

  return ret;
}

static uint32_t FindCandidateChildProcess(uint32_t parentPID, rdcstr *description)
{
  rdcarray<ChildProcessInfo> children = EnumerateLiveChildProcesses(parentPID);

  uint32_t bestPID = 0;
  rdcstr bestDescription;

  for(const ChildProcessInfo &child : children)
  {
    if(child.pid >= bestPID)
    {
      bestPID = child.pid;
      bestDescription = child.description;
    }
  }

  if(description)
    *description = bestDescription;

  return bestPID;
}

static rdcstr DescribeLaunchProcessExit(HANDLE hProcess)
{
  if(hProcess == NULL)
    return "";

  DWORD exitCode = STILL_ACTIVE;

  if(!GetExitCodeProcess(hProcess, &exitCode) || exitCode == STILL_ACTIVE)
    return "";

  rdcstr ret = StringFormat::Fmt("\n\nThe launched process exited with code 0x%08x", exitCode);

  switch(exitCode)
  {
    case 0xC0000135:
      ret += " (STATUS_DLL_NOT_FOUND: a dependent DLL could not be found during process startup)";
      break;
    case 0xC0000139:
      ret += " (STATUS_ENTRYPOINT_NOT_FOUND: a dependent DLL version does not match what the executable expects)";
      break;
    case 0xC000007B:
      ret += " (STATUS_INVALID_IMAGE_FORMAT: a DLL has the wrong architecture or is corrupt)";
      break;
    case 0xC0000142:
      ret += " (STATUS_DLL_INIT_FAILED: a DLL failed during process initialisation)";
      break;
    case 0xC0000409:
      ret += " (STATUS_STACK_BUFFER_OVERRUN / fast-fail: the target aborted itself very early)";
      break;
    case 0xC0000005:
      ret += " (STATUS_ACCESS_VIOLATION: the target likely crashed during startup)";
      break;
    default: break;
  }

  ret += ".";
  return ret;
}

static rdcstr DescribeChildProcesses(uint32_t parentPID)
{
  rdcarray<ChildProcessInfo> children = EnumerateLiveChildProcesses(parentPID);

  if(children.empty())
    return "";

  rdcstr ret = "\n\nThe launched process spawned child process";
  ret += children.size() > 1 ? "es that are still running: " : " that is still running: ";

  for(size_t i = 0; i < children.size(); i++)
  {
    if(i > 0)
      ret += i + 1 == children.size() ? " and " : ", ";

    ret += children[i].description;
  }

  ret += ". This target may use a launcher; try enabling child process capture or inject into "
         "the final child process directly.";
  return ret;
}

static uintptr_t WaitForRemoteDLL(HANDLE hProcess, DWORD pid, const rdcstr &libName,
                                  uint32_t attempts, uint32_t delayMilliseconds)
{
  uintptr_t remoteDLL = 0;

  for(uint32_t attempt = 0; attempt < attempts; attempt++)
  {
    remoteDLL = FindRemoteDLL(pid, libName, attempt + 1 >= attempts);

    if(remoteDLL != 0)
      break;

    if(!IsProcessActive(hProcess))
      break;

    if(attempt + 1 < attempts && delayMilliseconds > 0)
      Sleep(delayMilliseconds);
  }

  return remoteDLL;
}

bool InjectFunctionCall(HANDLE hProcess, uintptr_t renderdoc_remote, const char *funcName,
                        void *data, const size_t dataLen)
{
  if(dataLen == 0)
  {
    RDCERR("Invalid function call injection attempt");
    return false;
  }

  RDCDEBUG("Injecting call to %s", funcName);

  HMODULE renderdoc_local = GetModuleHandleA(RDOC_BRAND_CORE_DLL_NAME);

  if(renderdoc_local == NULL)
  {
    RDCERR("Couldn't get handle for %s", RDOC_BRAND_CORE_DLL_NAME);
    return false;
  }

  uintptr_t func_local = (uintptr_t)GetProcAddress(renderdoc_local, funcName);

  if(func_local == 0)
  {
    RDCERR("Couldn't find %s in %s", funcName, RDOC_BRAND_CORE_DLL_NAME);
    return false;
  }

  // we've found SetCaptureOptions in our local instance of the module, now calculate the offset and
  // so get the function
  // in the remote module (which might be loaded at a different base address
  uintptr_t func_remote = func_local + renderdoc_remote - (uintptr_t)renderdoc_local;

  void *remoteMem = VirtualAllocEx(hProcess, NULL, dataLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

  if(remoteMem == NULL)
  {
    RDCERR("Couldn't allocate remote memory for %s: %u", funcName, GetLastError());
    return false;
  }

  SIZE_T numWritten;

  if(!WriteProcessMemory(hProcess, remoteMem, data, dataLen, &numWritten) || numWritten != dataLen)
  {
    RDCERR("Couldn't write remote memory for %s: %u", funcName, GetLastError());
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    return false;
  }

  HANDLE hThread =
      CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)func_remote, remoteMem, 0, NULL);

  if(hThread == NULL)
  {
    RDCERR("Couldn't create remote thread for %s: %u", funcName, GetLastError());
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    return false;
  }

  DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

  if(waitResult != WAIT_OBJECT_0)
  {
    RDCERR("WaitForSingleObject failed for %s: %u", funcName, GetLastError());
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    return false;
  }

  if(!ReadProcessMemory(hProcess, remoteMem, data, dataLen, &numWritten) || numWritten != dataLen)
  {
    RDCERR("Couldn't read remote memory back for %s: %u", funcName, GetLastError());
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    return false;
  }

  CloseHandle(hThread);
  VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);

  return true;
}

static uint32_t QueryTargetControlIdent(HANDLE hProcess, uintptr_t renderdoc_remote)
{
  uint32_t ident = 0;

  for(uint32_t attempt = 0; attempt < 50 && ident == 0; attempt++)
  {
    if(!InjectFunctionCall(hProcess, renderdoc_remote, "INTERNAL_GetTargetControlIdent", &ident,
                           sizeof(ident)))
      break;

    if(ident != 0)
      break;

    if(!IsProcessActive(hProcess))
      break;

    Sleep(10);
  }

  return ident;
}

static PROCESS_INFORMATION RunProcess(const rdcstr &app, const rdcstr &workingDir,
                                      const rdcstr &cmdLine,
                                      const rdcarray<EnvironmentModification> &env, bool internal,
                                      HANDLE *phChildStdOutput_Rd, HANDLE *phChildStdError_Rd)
{
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  SECURITY_ATTRIBUTES pSec;
  SECURITY_ATTRIBUTES tSec;

  RDCEraseEl(pi);
  RDCEraseEl(si);
  RDCEraseEl(pSec);
  RDCEraseEl(tSec);

  si.cb = sizeof(si);

  pSec.nLength = sizeof(pSec);
  tSec.nLength = sizeof(tSec);

  rdcwstr workdir = L"";

  if(!workingDir.empty())
    workdir = StringFormat::UTF82Wide(workingDir);
  else
    workdir = StringFormat::UTF82Wide(get_dirname(app));

  wchar_t *paramsAlloc = NULL;

  rdcwstr wapp = StringFormat::UTF82Wide(app);

  // CreateProcessW can modify the params, need space.
  size_t len = wapp.length() + 10;

  rdcwstr wcmd = L"";

  if(!cmdLine.empty())
  {
    wcmd = StringFormat::UTF82Wide(cmdLine);
    len += wcmd.length();
  }

  paramsAlloc = new wchar_t[len];

  RDCEraseMem(paramsAlloc, len * sizeof(wchar_t));

  wcscpy_s(paramsAlloc, len, L"\"");
  wcscat_s(paramsAlloc, len, wapp.c_str());
  wcscat_s(paramsAlloc, len, L"\"");

  if(!cmdLine.empty())
  {
    wcscat_s(paramsAlloc, len, L" ");
    wcscat_s(paramsAlloc, len, wcmd.c_str());
  }

  bool inheritHandles = false;

  HANDLE hChildStdOutput_Wr = 0, hChildStdError_Wr = 0;
  if(phChildStdOutput_Rd)
  {
    RDCASSERT(phChildStdError_Rd);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if(!CreatePipe(phChildStdOutput_Rd, &hChildStdOutput_Wr, &sa, 0))
      RDCERR("Could not create pipe to read stdout");
    if(!SetHandleInformation(*phChildStdOutput_Rd, HANDLE_FLAG_INHERIT, 0))
      RDCERR("Could not set pipe handle information");

    if(!CreatePipe(phChildStdError_Rd, &hChildStdError_Wr, &sa, 0))
      RDCERR("Could not create pipe to read stdout");
    if(!SetHandleInformation(*phChildStdError_Rd, HANDLE_FLAG_INHERIT, 0))
      RDCERR("Could not set pipe handle information");

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hChildStdOutput_Wr;
    si.hStdError = hChildStdError_Wr;

    // Need to inherit handles in CreateProcess for ReadFile to read stdout
    inheritHandles = true;
  }

  // if it's a utility launch, hide the command prompt window from showing
  if(phChildStdOutput_Rd || internal)
    si.dwFlags |= STARTF_USESHOWWINDOW;

  if(!internal)
    RDCLOG("Running process %s", app.c_str());

  // turn environment string to a UTF-8 map
  std::wstring envString;

  if(!env.empty())
  {
    LPWCH envStrings = GetEnvironmentStringsW();
    EnvMap envValues = EnvStringToEnvMap(envStrings);
    FreeEnvironmentStringsW(envStrings);

    ApplyEnvModifications(envValues, env, false);

    for(auto it = envValues.begin(); it != envValues.end(); ++it)
    {
      envString += StringFormat::UTF82Wide(it->first).c_str();
      envString += L"=";
      envString += StringFormat::UTF82Wide(it->second).c_str();
      envString.push_back(0);
    }
  }

  BOOL retValue = CreateProcessW(
      NULL, paramsAlloc, &pSec, &tSec, inheritHandles, CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
      envString.empty() ? NULL : (void *)envString.data(), workdir.c_str(), &si, &pi);

  DWORD err = GetLastError();

  if(phChildStdOutput_Rd)
  {
    CloseHandle(hChildStdOutput_Wr);
    CloseHandle(hChildStdError_Wr);
  }

  SAFE_DELETE_ARRAY(paramsAlloc);

  if(!retValue)
  {
    if(!internal)
      RDCWARN("Process %s could not be loaded (error %d).", app.c_str(), err);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    RDCEraseEl(pi);
  }

  return pi;
}

rdcpair<RDResult, uint32_t> Process::InjectIntoProcess(uint32_t pid,
                                                       const rdcarray<EnvironmentModification> &env,
                                                       const rdcstr &capturefile,
                                                       const CaptureOptions &opts, bool waitForExit)
{
  rdcwstr wcapturefile = StringFormat::UTF82Wide(capturefile);

  HANDLE hProcess =
      OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                      PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE,
                  FALSE, pid);

  if(opts.delayForDebugger > 0)
  {
    RDCDEBUG("Waiting for debugger attach to %lu", pid);
    uint32_t timeout = 0;

    BOOL debuggerAttached = FALSE;

    while(!debuggerAttached)
    {
      CheckRemoteDebuggerPresent(hProcess, &debuggerAttached);

      Sleep(10);
      timeout += 10;

      if(timeout > opts.delayForDebugger * 1000)
        break;
    }

    if(debuggerAttached)
      RDCDEBUG("Debugger attach detected after %.2f s", float(timeout) / 1000.0f);
    else
      RDCDEBUG("Timed out waiting for debugger, gave up after %u s", opts.delayForDebugger);
  }

  RDCLOG("Injecting renderdoc into process %lu", pid);

  wchar_t renderdocPath[MAX_PATH] = {0};
  GetModuleFileNameW(GetModuleHandleA(RDOC_BRAND_CORE_DLL_NAME), &renderdocPath[0],
                                      MAX_PATH - 1);

  wchar_t renderdocPathLower[MAX_PATH] = {0};
  memcpy(renderdocPathLower, renderdocPath, MAX_PATH * sizeof(wchar_t));
  for(size_t i = 0; i < MAX_PATH && renderdocPathLower[i]; i++)
  {
    // lowercase
    if(renderdocPathLower[i] >= 'A' && renderdocPathLower[i] <= 'Z')
      renderdocPathLower[i] = 'a' + char(renderdocPathLower[i] - 'A');

    // normalise paths
    if(renderdocPathLower[i] == '/')
      renderdocPathLower[i] = '\\';
  }

  BOOL isWow64 = FALSE;
  BOOL success = IsWow64Process(hProcess, &isWow64);

  if(!success)
  {
    DWORD err = GetLastError();
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                     "Couldn't determine bitness of process, err: %08x", err);
    CloseHandle(hProcess);
    return {result, 0};
  }

  bool capalt = false;

#if DISABLED(RDOC_X64)
  BOOL selfWow64 = FALSE;

  HANDLE hSelfProcess = GetCurrentProcess();

  // check to see if we're a WoW64 process
  success = IsWow64Process(hSelfProcess, &selfWow64);

  CloseHandle(hSelfProcess);

  if(!success)
  {
    DWORD err = GetLastError();
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                     "Couldn't determine bitness of self, err: %08x", err);
    CloseHandle(hProcess);
    return {result, 0};
  }

  // we know we're 32-bit, so if the target process is not wow64
  // and we are, it's 64-bit. If we're both not wow64 then we're
  // running on 32-bit windows, and if we're both wow64 then we're
  // both 32-bit on 64-bit windows.
  //
  // We don't support capturing 64-bit programs from a 32-bit install
  // because it's pointless - a 64-bit install will work for all in
  // that case. But we do want to handle the case of:
  // 64-bit renderdoc -> 32-bit program (via 32-bit renderdoccmd)
  //    -> 64-bit program (going back to 64-bit renderdoccmd).
  // so we try to see if we're an x86 invoked renderdoccmd in an
  // otherwise 64-bit install, and 'promote' back to 64-bit.
  if(selfWow64 && !isWow64)
  {
    wchar_t *slash = wcsrchr(renderdocPath, L'\\');

    if(slash && slash > renderdocPath + 4)
    {
      slash -= 4;

      if(slash && !wcsncmp(slash, L"\\x86", 4))
      {
        RDCDEBUG("Promoting back to 64-bit");
        capalt = true;
      }
    }

    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    if(!capalt)
    {
      const wchar_t *devLocation = wcsstr(renderdocPathLower, L"\\win32\\development\\");
      if(!devLocation)
        devLocation = wcsstr(renderdocPathLower, L"\\win32\\release\\");

      if(devLocation)
      {
        RDCDEBUG("Promoting back to 64-bit");
        capalt = true;
      }
    }

    // if we couldn't promote, then bail out.
    if(!capalt)
    {
      RDCDEBUG("Running from %ls", renderdocPathLower);

      CloseHandle(hProcess);
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                       "Can't capture 64-bit program with 32-bit build of "
                       RDOC_BRAND_PRODUCT_NAME ". Please run a 64-bit build of "
                       RDOC_BRAND_PRODUCT_NAME);
      return {result, 0};
    }
  }
#else
  // farm off to alternate bitness helper command

  // if the target process is 'wow64' that means it's 32-bit.
  capalt = (isWow64 == TRUE);
#endif

  if(capalt)
  {
#if ENABLED(RDOC_X64)
    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    const wchar_t *devLocation = wcsstr(renderdocPathLower, L"\\x64\\development\\");
    if(devLocation)
    {
      size_t idx = devLocation - renderdocPathLower;

      renderdocPath[idx] = 0;

      wcscat_s(renderdocPath, L"\\Win32\\Development\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
    }

    if(!devLocation)
    {
      devLocation = wcsstr(renderdocPathLower, L"\\x64\\release\\");

      if(devLocation)
      {
        size_t idx = devLocation - renderdocPathLower;

        renderdocPath[idx] = 0;

        wcscat_s(renderdocPath, L"\\Win32\\Release\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
      }
    }

    if(!devLocation)
    {
      // look in a subfolder for x86.

      // remove the filename from the path
      wchar_t *slash = wcsrchr(renderdocPath, L'\\');

      if(slash)
        *slash = 0;

      // append path
      wcscat_s(renderdocPath, L"\\x86\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
    }
#else
    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    const wchar_t *devLocation = wcsstr(renderdocPathLower, L"\\win32\\development\\");
    if(devLocation)
    {
      size_t idx = devLocation - renderdocPathLower;

      renderdocPath[idx] = 0;

      wcscat_s(renderdocPath, L"\\x64\\Development\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
    }

    if(!devLocation)
    {
      devLocation = wcsstr(renderdocPathLower, L"\\win32\\release\\");

      if(devLocation)
      {
        size_t idx = devLocation - renderdocPathLower;

        renderdocPath[idx] = 0;

        wcscat_s(renderdocPath, L"\\x64\\Release\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
      }
    }

    if(!devLocation)
    {
      // look upwards on 32-bit to find the parent helper command.
      wchar_t *slash = wcsrchr(renderdocPath, L'\\');

      // remove the filename
      if(slash)
        *slash = 0;

      // remove the \\x86
      slash = wcsrchr(renderdocPath, L'\\');

      if(slash)
        *slash = 0;

      // append path
      wcscat_s(renderdocPath, L"\\" RDOC_WIDEN(RDOC_BRAND_CMD_EXECUTABLE));
    }
#endif

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES pSec;
    SECURITY_ATTRIBUTES tSec;

    RDCEraseEl(pi);
    RDCEraseEl(si);
    RDCEraseEl(pSec);
    RDCEraseEl(tSec);

    // hide the console window
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    pSec.nLength = sizeof(pSec);
    tSec.nLength = sizeof(tSec);

    // serialise to string with two chars per byte
    rdcstr optstr = opts.EncodeAsString();

    wchar_t *paramsAlloc = new wchar_t[2048];

    rdcstr debugLogfile = RDCGETLOGFILE();
    rdcwstr wdebugLogfile = StringFormat::UTF82Wide(debugLogfile);

    _snwprintf_s(
        paramsAlloc, 2047, 2047,
        L"\"%ls\" capaltbit --pid=%u --capfile=\"%ls\" --debuglog=\"%ls\" --capopts=\"%hs\"",
        renderdocPath, pid, wcapturefile.c_str(), wdebugLogfile.c_str(), optstr.c_str());

    RDCDEBUG("params %ls", paramsAlloc);

    paramsAlloc[2047] = 0;

    wchar_t *commandLine = paramsAlloc;

    std::wstring cmdWithEnv;

    if(!env.empty())
    {
      cmdWithEnv = paramsAlloc;

      for(const EnvironmentModification &e : env)
      {
        rdcstr name = e.name.trimmed();
        rdcstr value = e.value;

        if(name == "")
          break;

        cmdWithEnv += L" +env-";
        switch(e.mod)
        {
          case EnvMod::Set: cmdWithEnv += L"replace"; break;
          case EnvMod::Append: cmdWithEnv += L"append"; break;
          case EnvMod::Prepend: cmdWithEnv += L"prepend"; break;
        }

        if(e.mod != EnvMod::Set)
        {
          switch(e.sep)
          {
            case EnvSep::Platform: cmdWithEnv += L"-platform"; break;
            case EnvSep::SemiColon: cmdWithEnv += L"-semicolon"; break;
            case EnvSep::Colon: cmdWithEnv += L"-colon"; break;
            case EnvSep::NoSep: break;
          }
        }

        cmdWithEnv += L" ";

        // escape the parameters
        for(size_t it = 0; it < name.size(); it++)
        {
          if(name[it] == '"')
          {
            name.insert(it, '\\');
            it++;
          }
        }

        for(size_t it = 0; it < value.size(); it++)
        {
          if(value[it] == '"')
          {
            value.insert(it, '\\');
            it++;
          }
        }

        if(name.back() == '\\')
          name += "\\";

        if(value.back() == '\\')
          value += "\\";

        cmdWithEnv += L"\"" + std::wstring(StringFormat::UTF82Wide(name).c_str()) + L"\" ";
        cmdWithEnv += L"\"" + std::wstring(StringFormat::UTF82Wide(value).c_str()) + L"\" ";
      }

      commandLine = (wchar_t *)cmdWithEnv.c_str();
    }

    BOOL retValue = CreateProcessW(NULL, commandLine, &pSec, &tSec, false,
                                   CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &si, &pi);

    SAFE_DELETE_ARRAY(paramsAlloc);

    if(!retValue)
    {
      RDResult result;
#if RENDERDOC_OFFICIAL_BUILD
      SET_ERROR_RESULT(result, ResultCode::InternalError,
                       "Can't run 32-bit " RDOC_BRAND_CMD_NAME " to capture 32-bit program.");
#else
      SET_ERROR_RESULT(
          result, ResultCode::InternalError,
          "Can't run 32-bit " RDOC_BRAND_CMD_NAME " to capture 32-bit program."
          "If this is a locally built " RDOC_BRAND_PRODUCT_NAME
          " you must build both 32-bit and 64-bit versions.");
#endif
      CloseHandle(hProcess);
      return {result, 0};
    }

    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hThread, INFINITE);
    CloseHandle(pi.hThread);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);

    if(waitForExit)
      WaitForSingleObject(hProcess, INFINITE);

    CloseHandle(hProcess);

    if(exitCode == 0)
    {
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::UnknownError,
                       "Encountered error while launching target 32-bit program.");
      return {result, 0};
    }

    if(exitCode < RenderDoc_FirstTargetControlPort)
    {
      ResultCode code = (ResultCode)exitCode;

      RDResult result;
      SET_ERROR_RESULT(result, code, "32-bit " RDOC_BRAND_CMD_NAME " returned '%s'",
                       ToStr(code).c_str());
      return {code, 0};
    }

    return {ResultCode::Succeeded, (uint32_t)exitCode};
  }

  DLLInjectionResult dllResult = InjectDLL(hProcess, renderdocPath, 3, 10);

  const char *rdoc_dll = RDOC_BRAND_CORE_DLL_NAME;

  uintptr_t loc = WaitForRemoteDLL(hProcess, pid, RDOC_BRAND_CORE_DLL_NAME, 50, 10);
  rdcstr mitigationNotes = DescribeMitigationPolicies(hProcess);

  rdcpair<RDResult, uint32_t> result = {ResultCode::Succeeded, 0};

  if(loc == 0)
  {
    if(!IsProcessActive(hProcess))
    {
      SET_ERROR_RESULT(
          result.first, ResultCode::InjectionFailed,
          "Failed to inject %s into process because the target exited during startup. Check that "
          "the process did not crash or exit early in initialisation, e.g. if the working "
          "directory is incorrectly set.%s",
          rdoc_dll, mitigationNotes.c_str());
    }
    else if(dllResult.loadResult != 0 && dllResult.confirmedModuleHandle == 0)
    {
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "LoadLibraryExW reported success for %s, but GetModuleHandleW could not "
                       "find the module immediately afterwards. This usually means the target "
                       "unloaded or rejected the capture DLL during startup.%s",
                       rdoc_dll, mitigationNotes.c_str());
    }
    else if(dllResult.loadResult == 0)
    {
      rdcstr fallbackNote;

      if(dllResult.usedSearchDirFallback)
      {
        fallbackNote =
            " The injector also retried with LoadLibraryExW(..., "
            "LOAD_WITH_ALTERED_SEARCH_PATH), so a plain dependency search path problem is less "
            "likely.";
      }

      if(dllResult.loaderError != ERROR_SUCCESS)
      {
        rdcstr loaderNote = DescribeRemoteLoadFailure(dllResult.loaderError);

        SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                         "Failed to inject %s into process because remote LoadLibraryExW returned "
                         "NULL with remote last error %u. This usually means the DLL or one of "
                         "its dependencies could not be loaded into the target process.%s%s%s",
                         rdoc_dll, dllResult.loaderError, fallbackNote.c_str(),
                         loaderNote.c_str(), mitigationNotes.c_str());
      }
      else
      {
        SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                         "Failed to inject %s into process because remote LoadLibraryExW returned "
                         "NULL. This usually means the DLL or one of its dependencies could not "
                         "be loaded into the target process.%s%s",
                         rdoc_dll, fallbackNote.c_str(), mitigationNotes.c_str());
      }
    }
    else if(dllResult.error != ERROR_SUCCESS)
    {
      SET_ERROR_RESULT(
          result.first, ResultCode::InjectionFailed,
          "Failed to inject %s into process (win32 error %u). Check that the process did not "
          "crash or exit early in initialisation, e.g. if the working directory is incorrectly "
          "set.%s",
          rdoc_dll, dllResult.error, mitigationNotes.c_str());
    }
    else
    {
      SET_ERROR_RESULT(
          result.first, ResultCode::InjectionFailed,
          "Failed to inject %s into process. Check that the process did not crash or exit early "
          "in initialisation, e.g. if the working directory is incorrectly set.%s",
          rdoc_dll, mitigationNotes.c_str());
    }
  }
  else
  {
    // safe to cast away the const as we know these functions don't modify the parameters

    if(!capturefile.empty())
      InjectFunctionCall(hProcess, loc, "INTERNAL_SetCaptureFile", (void *)capturefile.c_str(),
                         capturefile.size() + 1);

    rdcstr debugLogfile = RDCGETLOGFILE();

    InjectFunctionCall(hProcess, loc, "INTERNAL_SetDebugLogFile", (void *)debugLogfile.c_str(),
                       debugLogfile.size() + 1);

    InjectFunctionCall(hProcess, loc, "INTERNAL_SetCaptureOptions", (CaptureOptions *)&opts,
                       sizeof(CaptureOptions));

    result.second = QueryTargetControlIdent(hProcess, loc);

    if(!env.empty())
    {
      for(const EnvironmentModification &e : env)
      {
        rdcstr name = e.name.trimmed();
        rdcstr value = e.value;
        EnvMod mod = e.mod;
        EnvSep sep = e.sep;

        if(name == "")
          break;

        InjectFunctionCall(hProcess, loc, "INTERNAL_EnvModName", (void *)name.c_str(),
                           name.size() + 1);
        InjectFunctionCall(hProcess, loc, "INTERNAL_EnvModValue", (void *)value.c_str(),
                           value.size() + 1);
        InjectFunctionCall(hProcess, loc, "INTERNAL_EnvSep", &sep, sizeof(sep));
        InjectFunctionCall(hProcess, loc, "INTERNAL_EnvMod", &mod, sizeof(mod));
      }

      // parameter is unused
      void *dummy = NULL;
      InjectFunctionCall(hProcess, loc, "INTERNAL_ApplyEnvMods", &dummy, sizeof(dummy));
    }

    if(result.second == 0)
    {
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Injected %s into the target process, but the target control connection "
                       "did not become ready in time.%s",
                       rdoc_dll, mitigationNotes.c_str());
    }
  }

  if(waitForExit)
    WaitForSingleObject(hProcess, INFINITE);

  CloseHandle(hProcess);

  return result;
}

static bool InjectSucceeded(const rdcpair<RDResult, uint32_t> &result)
{
  return result.first == ResultCode::Succeeded && result.second != 0;
}

static rdcstr GetLaunchInjectFailureMessage(const rdcpair<RDResult, uint32_t> &result)
{
  if(!result.first.message.empty())
    return result.first.message;

  return ToStr(result.first.code);
}

static void WaitForProcessInputIdle(HANDLE hProcess, DWORD timeout)
{
  DWORD idleResult = WaitForInputIdle(hProcess, timeout);

  if(idleResult == WAIT_FAILED)
  {
    DWORD err = GetLastError();

    if(err != ERROR_SUCCESS)
      RDCDEBUG("WaitForInputIdle failed: %u", err);
  }
}

static rdcpair<RDResult, uint32_t> RetryLaunchInjectIntoRunningProcess(
    HANDLE hProcess, uint32_t pid, const rdcstr &capturefile, const CaptureOptions &opts,
    const rdcpair<RDResult, uint32_t> &initialResult)
{
  const DWORD retryDelays[] = {0, 10, 25, 50, 100, 200, 400, 800};
  rdcpair<RDResult, uint32_t> result = initialResult;

  for(size_t attempt = 0; attempt < ARRAY_COUNT(retryDelays); attempt++)
  {
    if(retryDelays[attempt] > 0)
      Sleep(retryDelays[attempt]);

    if(!IsProcessActive(hProcess))
      break;

    WaitForProcessInputIdle(hProcess, 50);

    RDCDEBUG("Retrying injection into running process %u (attempt %zu)", pid, attempt + 1);

    result = Process::InjectIntoProcess(pid, {}, capturefile, opts, false);

    if(InjectSucceeded(result))
      return result;
  }

  return result;
}

static rdcpair<RDResult, uint32_t> LateAttachIntoRunningProcess(
    HANDLE hProcess, uint32_t pid, const rdcstr &capturefile, const CaptureOptions &opts,
    const rdcpair<RDResult, uint32_t> &initialResult)
{
  const DWORD retryDelays[] = {0, 50, 100, 250, 500, 1000, 1500};
  rdcpair<RDResult, uint32_t> result = initialResult;

  WaitForProcessInputIdle(hProcess, 1000);

  for(size_t attempt = 0; attempt < ARRAY_COUNT(retryDelays); attempt++)
  {
    if(retryDelays[attempt] > 0)
      Sleep(retryDelays[attempt]);

    if(!IsProcessActive(hProcess))
      break;

    WaitForProcessInputIdle(hProcess, 250);

    RDCDEBUG("Late-attaching into running process %u (attempt %zu)", pid, attempt + 1);

    result = Process::InjectIntoProcess(pid, {}, capturefile, opts, false);

    if(InjectSucceeded(result))
      return result;
  }

  return result;
}

static rdcpair<RDResult, uint32_t> FollowLauncherChildProcess(
    uint32_t parentPID, const rdcstr &capturefile, const CaptureOptions &opts,
    const rdcpair<RDResult, uint32_t> &initialResult, bool *attempted)
{
  if(attempted)
    *attempted = false;

  const DWORD retryDelays[] = {0, 25, 50, 100, 200, 400, 800, 1200};
  rdcpair<RDResult, uint32_t> result = initialResult;

  for(size_t attempt = 0; attempt < ARRAY_COUNT(retryDelays); attempt++)
  {
    if(retryDelays[attempt] > 0)
      Sleep(retryDelays[attempt]);

    rdcstr childDescription;
    uint32_t childPID = FindCandidateChildProcess(parentPID, &childDescription);

    if(childPID == 0)
      continue;

    if(attempted)
      *attempted = true;

    RDCDEBUG("Attempting injection into child process %u spawned by launcher %u (%s)", childPID,
             parentPID, childDescription.c_str());

    HANDLE hChild = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, childPID);

    if(hChild)
    {
      result = LateAttachIntoRunningProcess(hChild, childPID, capturefile, opts, result);
      CloseHandle(hChild);
    }
    else
    {
      result = Process::InjectIntoProcess(childPID, {}, capturefile, opts, false);
    }

    if(InjectSucceeded(result))
      return result;
  }

  return result;
}

static rdcstr JoinLaunchInjectAttempts(const rdcarray<rdcstr> &attempts)
{
  rdcstr ret;

  for(size_t i = 0; i < attempts.size(); i++)
  {
    if(!ret.empty())
      ret += i + 1 == attempts.size() ? " and " : ", ";

    ret += attempts[i];
  }

  return ret;
}

static rdcpair<RDResult, uint32_t> MakeLaunchInjectFailure(
    LaunchInjectMode mode, const rdcarray<rdcstr> &attempts,
    const rdcpair<RDResult, uint32_t> &failure, const rdcstr &extraNotes)
{
  rdcpair<RDResult, uint32_t> result = failure;

  if(InjectSucceeded(result))
    return result;

  rdcstr attemptsText = JoinLaunchInjectAttempts(attempts);
  rdcstr failureMessage = GetLaunchInjectFailureMessage(failure);
  ResultCode failureCode = failure.first.code;

  if(attemptsText.empty())
    attemptsText = "the selected launch injection strategy";

  if(failureCode == ResultCode::Succeeded)
    failureCode = ResultCode::InjectionFailed;

  result.first = RDResult(
      failureCode,
      StringFormat::Fmt(
          "Launch injection mode '%s' failed after trying %s.\n\n%s\n\nIf the target has "
          "unusual startup timing, try a different Launch Injection mode in the capture "
          "dialog.%s",
          LaunchInjectModeName(mode), attemptsText.c_str(), failureMessage.c_str(),
          extraNotes.c_str()));
  result.second = 0;
  return result;
}

uint32_t Process::LaunchProcess(const rdcstr &app, const rdcstr &workingDir, const rdcstr &cmdLine,
                                bool internal, ProcessResult *result)
{
  HANDLE hChildStdOutput_Rd = NULL, hChildStdError_Rd = NULL;

  rdcstr appPath = app;
  size_t len = appPath.length();
  rdcstr ext;
  if(len > 4)
    ext = strlower(appPath.substr(len - 4));
  if(ext != ".exe")
    appPath += ".exe";

  PROCESS_INFORMATION pi =
      RunProcess(appPath, workingDir, cmdLine, {}, internal, result ? &hChildStdOutput_Rd : NULL,
                 result ? &hChildStdError_Rd : NULL);

  if(pi.dwProcessId == 0)
  {
    if(!internal)
      RDCWARN("Couldn't launch process '%s'", appPath.c_str());

    if(hChildStdError_Rd != NULL)
      CloseHandle(hChildStdError_Rd);
    if(hChildStdOutput_Rd != NULL)
      CloseHandle(hChildStdOutput_Rd);

    return 0;
  }

  if(!internal)
    RDCLOG("Launched process '%s' with '%s'", appPath.c_str(), cmdLine.c_str());

  ResumeThread(pi.hThread);

  if(result)
  {
    result->strStdout = "";
    result->strStderror = "";

    char chBuf[4096];
    DWORD dwOutputRead, dwErrorRead;
    BOOL success = FALSE;
    rdcstr s;
    for(;;)
    {
      success = ReadFile(hChildStdOutput_Rd, chBuf, sizeof(chBuf), &dwOutputRead, NULL);
      s = rdcstr(chBuf, dwOutputRead);
      result->strStdout += s;

      if(!success && !dwOutputRead)
        break;
    }

    for(;;)
    {
      success = ReadFile(hChildStdError_Rd, chBuf, sizeof(chBuf), &dwErrorRead, NULL);
      s = rdcstr(chBuf, dwErrorRead);
      result->strStderror += s;

      if(!success && !dwErrorRead)
        break;
    }

    CloseHandle(hChildStdOutput_Rd);
    CloseHandle(hChildStdError_Rd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&result->retCode);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return pi.dwProcessId;
}

uint32_t Process::LaunchScript(const rdcstr &script, const rdcstr &workingDir,
                               const rdcstr &argList, bool internal, ProcessResult *result)
{
  // Change parameters to invoke command interpreter
  rdcstr args = "/C " + script + " " + argList;

  return LaunchProcess("cmd.exe", workingDir, args, internal, result);
}

rdcpair<RDResult, uint32_t> Process::LaunchAndInjectIntoProcess(
    const rdcstr &app, const rdcstr &workingDir, const rdcstr &cmdLine,
    const rdcarray<EnvironmentModification> &env, const rdcstr &capturefile,
    const CaptureOptions &opts, bool waitForExit)
{
  void *func =
      GetProcAddress(GetModuleHandleA(RDOC_BRAND_CORE_DLL_NAME), "INTERNAL_SetCaptureFile");

  if(func == NULL)
  {
    const char *rdoc_dll = RDOC_BRAND_CORE_DLL_NAME;
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InternalError,
                     "Can't find required export function in %s - corrupted/missing file?",
                     rdoc_dll);
    return {result, 0};
  }

  if(get_basename(app) == "explorer.exe" || get_basename(app) == "dllhost.exe")
  {
    RDResult result;
    SET_ERROR_RESULT(
        result, ResultCode::InjectionFailed,
        "For safety reasons RenderDoc does not support capturing executables with a "
        "reserved system filename such as '%s'. Please rename your executable to capture.",
        get_basename(app).c_str());
    return {result, 0};
  }

  PROCESS_INFORMATION pi = RunProcess(app, workingDir, cmdLine, env, false, NULL, NULL);

  if(pi.dwProcessId == 0)
  {
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InjectionFailed, "Failed to launch process.");
    return {result, 0};
  }

  LaunchInjectMode launchMode = ConsumeLaunchInjectMode();
  rdcpair<RDResult, uint32_t> ret = {ResultCode::InjectionFailed, 0};
  rdcarray<rdcstr> attemptedPaths;
  bool resumed = false;

  if(launchMode != LaunchInjectMode::LateAttach)
  {
    attemptedPaths.push_back("suspended launch injection");
    ret = InjectIntoProcess(pi.dwProcessId, {}, capturefile, opts, false);
  }

  if(!InjectSucceeded(ret) && launchMode != LaunchInjectMode::SuspendedOnly &&
     IsProcessActive(pi.hProcess))
  {
    if(launchMode != LaunchInjectMode::LateAttach)
      RDCDEBUG("Initial suspended injection into process %u failed, retrying after resume",
               pi.dwProcessId);

    ResumeThreadCompletely(pi.hThread);
    resumed = true;

    if(launchMode == LaunchInjectMode::Automatic || launchMode == LaunchInjectMode::ResumeRetry)
    {
      attemptedPaths.push_back("resume-and-retry injection");
      ret = RetryLaunchInjectIntoRunningProcess(pi.hProcess, pi.dwProcessId, capturefile, opts, ret);
    }

    if(!InjectSucceeded(ret) && IsProcessActive(pi.hProcess) &&
       (launchMode == LaunchInjectMode::Automatic || launchMode == LaunchInjectMode::LateAttach))
    {
      attemptedPaths.push_back("late attach after startup idle");
      ret = LateAttachIntoRunningProcess(pi.hProcess, pi.dwProcessId, capturefile, opts, ret);
    }
  }

  if(!resumed)
  {
    ResumeThreadCompletely(pi.hThread);
    resumed = true;
  }

  if(!InjectSucceeded(ret) && launchMode == LaunchInjectMode::Automatic &&
     !IsProcessActive(pi.hProcess))
  {
    bool attemptedChildFollow = false;
    rdcpair<RDResult, uint32_t> childRet =
        FollowLauncherChildProcess(pi.dwProcessId, capturefile, opts, ret, &attemptedChildFollow);

    if(attemptedChildFollow)
    {
      attemptedPaths.push_back("launcher child follow-up injection");
      ret = childRet;
    }
  }

  if(!InjectSucceeded(ret))
  {
    rdcstr extraNotes = DescribeLaunchProcessExit(pi.hProcess);
    extraNotes += DescribeChildProcesses(pi.dwProcessId);

    ret = MakeLaunchInjectFailure(launchMode, attemptedPaths, ret, extraNotes);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return ret;
  }

  if(waitForExit)
    WaitForSingleObject(pi.hProcess, INFINITE);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return ret;
}

bool Process::CanGlobalHook()
{
  // all we need is admin rights and it's the caller's responsibility to ensure that.
  return true;
}

// to simplify the below code, rather than splitting by 32-bit/64-bit we split by native and Wow32.
// This means that for 32-bit code (whether it's on 32-bit OS or not) we just have native, and the
// Wow32 stuff is empty/unused. For 64-bit we use both. Thus the native registry key is always the
// same path regardless of the bitness we're running as and we don't have to move things around or
// have conditionals all over

struct GlobalHookData
{
  struct
  {
    HANDLE pipe = NULL;
    DWORD appinitEnabled = 0;
    rdcwstr appinitDLLs;
  } dataNative, dataWow32;

  int32_t finished = 0;
  Threading::ThreadHandle pipeThread = 0;
};

// utility function to close the registry keys, print an error, and quit
static RDResult HandleRegError(HKEY keyNative, HKEY keyWow32, LSTATUS ret, const char *msg)
{
  if(keyNative)
    RegCloseKey(keyNative);

  if(keyWow32)
    RegCloseKey(keyWow32);

  RDCLOG("Error with AppInit registry keys - %s (%d)", msg, ret);

  RETURN_ERROR_RESULT(ResultCode::InjectionFailed,
                      "Error updating registry to enable global hook.\n"
                      "Check that RenderDoc is correctly running as administrator.");
}

#define REG_CHECK(msg)                                    \
  if(ret != ERROR_SUCCESS)                                \
  {                                                       \
    return HandleRegError(keyNative, keyWow32, ret, msg); \
  }

// function to backup the previous settings for AppInit, then enable it and write our own paths.
RDResult BackupAndChangeRegistry(GlobalHookData &hookdata, const rdcstr &shimpathWow32,
                                 const rdcstr &shimpathNative)
{
  HKEY keyNative = NULL;
  HKEY keyWow32 = NULL;

  // AppInit_DLLs requires short paths, but short paths can be disabled globally or on a per-volume
  // level. If short paths are disabled we'll get the long path back, we *always* expect the path to
  // get shorter because the shim filename is bigger than 8.3.

  DWORD nativeShortSize = GetShortPathNameW(StringFormat::UTF82Wide(shimpathNative).c_str(), NULL,
                                            (DWORD)shimpathNative.length());
  if(nativeShortSize == (DWORD)shimpathNative.length() + 1)
  {
    RETURN_ERROR_RESULT(
        ResultCode::FileIOFailed,
        RDOC_BRAND_PRODUCT_NAME " is installed on a volume or system that has short paths "
        "disabled.\n"
        "For the global hook, short paths must be enabled where " RDOC_BRAND_PRODUCT_NAME
        " is installed.");
  }

  if(!shimpathWow32.empty())
  {
    DWORD wow32ShortSize = GetShortPathNameW(StringFormat::UTF82Wide(shimpathWow32).c_str(), NULL,
                                             (DWORD)shimpathWow32.length());

    if(wow32ShortSize == (DWORD)shimpathWow32.length() + 1)
    {
      RETURN_ERROR_RESULT(
          ResultCode::FileIOFailed,
          RDOC_BRAND_PRODUCT_NAME " is installed on a volume or system that has short paths "
          "disabled.\n"
          "For the global hook, short paths must be enabled where " RDOC_BRAND_PRODUCT_NAME
          " is installed.");
    }
  }

  // open the native key
  LSTATUS ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, &keyNative, NULL);

  REG_CHECK("Could not open AppInit key");

  // if we are doing Wow32, open that key as well
  if(!shimpathWow32.empty())
  {
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                          "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                          0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &keyWow32, NULL);

    REG_CHECK("Could not open AppInit key");
  }

  const DWORD one = 1;

  // fetch the previous data for LoadAppInit_DLLs and AppInit_DLLs
  DWORD sz = 4;
  ret = RegGetValueA(keyNative, NULL, "LoadAppInit_DLLs", RRF_RT_REG_DWORD, NULL,
                     (void *)&hookdata.dataNative.appinitEnabled, &sz);
  REG_CHECK("Could not fetch LoadAppInit_DLLs");

  sz = 0;
  ret = RegGetValueW(keyNative, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL, NULL, &sz);
  if(ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
  {
    hookdata.dataNative.appinitDLLs = rdcwstr(sz / sizeof(wchar_t));
    ret = RegGetValueW(keyNative, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL,
                       hookdata.dataNative.appinitDLLs.data(), &sz);
  }
  REG_CHECK("Could not fetch AppInit_DLLs");

  // set DWORD:1 for LoadAppInit_DLLs and convert our path to a short path then set it
  ret = RegSetValueExA(keyNative, "LoadAppInit_DLLs", 0, REG_DWORD, (const BYTE *)&one, sizeof(one));
  REG_CHECK("Could not set LoadAppInit_DLLs");

  rdcwstr shortpath(shimpathNative.size());
  GetShortPathNameW(StringFormat::UTF82Wide(shimpathNative).c_str(), shortpath.data(),
                    (DWORD)shortpath.length());

  ret = RegSetValueExW(keyNative, L"AppInit_DLLs", 0, REG_SZ, (const BYTE *)shortpath.data(),
                       DWORD(shortpath.length() * sizeof(wchar_t)));
  REG_CHECK("Could not set AppInit_DLLs");

  // if we're doing Wow32, repeat the process for those keys
  if(keyWow32)
  {
    sz = 4;
    ret = RegGetValueA(keyWow32, NULL, "LoadAppInit_DLLs", RRF_RT_REG_DWORD, NULL,
                       (void *)&hookdata.dataWow32.appinitEnabled, &sz);
    REG_CHECK("Could not fetch LoadAppInit_DLLs");

    sz = 0;
    ret = RegGetValueW(keyWow32, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL, NULL, &sz);
    if(ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
    {
      hookdata.dataWow32.appinitDLLs = rdcwstr(sz / sizeof(wchar_t));
      ret = RegGetValueW(keyWow32, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL,
                         hookdata.dataWow32.appinitDLLs.data(), &sz);
    }
    REG_CHECK("Could not fetch AppInit_DLLs");

    ret = RegSetValueExA(keyWow32, "LoadAppInit_DLLs", 0, REG_DWORD, (const BYTE *)&one, sizeof(one));
    REG_CHECK("Could not set LoadAppInit_DLLs");

    shortpath = rdcwstr(shimpathWow32.size());
    GetShortPathNameW(StringFormat::UTF82Wide(shimpathWow32).c_str(), shortpath.data(),
                      (DWORD)shortpath.length());

    ret = RegSetValueExW(keyWow32, L"AppInit_DLLs", 0, REG_SZ, (const BYTE *)shortpath.data(),
                         DWORD(shortpath.length() * sizeof(wchar_t)));
    REG_CHECK("Could not set AppInit_DLLs");
  }

  std::wstring backup;

  // write a .reg file that contains the previous settings, so that if all else fails the user can
  // manually insert it back into the registry to restore everything.
  backup += L"Windows Registry Editor Version 5.00\n";
  backup += L"\n";
  backup += L"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows]\n";
  backup += L"\"LoadAppInit_DLLs\"=dword:0000000";
  backup += (hookdata.dataNative.appinitEnabled ? L"1\n" : L"0\n");
  backup += L"\"AppInit_DLLs\"=\"";
  // we append with the C string so we don't add trailing NULLs into the text.
  backup += hookdata.dataNative.appinitDLLs.c_str();
  backup += L"\"\n";
  if(keyWow32)
  {
    backup += L"\n";
    backup +=
        L"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\"
        L"Windows NT\\CurrentVersion\\Windows]\n";
    backup += L"\"LoadAppInit_DLLs\"=dword:0000000";
    backup += (hookdata.dataWow32.appinitEnabled ? L"1\n" : L"0\n");
    backup += L"\"AppInit_DLLs\"=\"";
    backup += hookdata.dataWow32.appinitDLLs.c_str();
    backup += L"\"\n";
  }

  if(keyNative)
    RegCloseKey(keyNative);

  if(keyWow32)
    RegCloseKey(keyWow32);

  keyNative = keyWow32 = NULL;

  // write it to disk but don't fail if we can't, just print it to the log and keep going.
  wchar_t reg_backup[MAX_PATH];
  GetTempPathW(MAX_PATH, reg_backup);
  wcscat_s(reg_backup, RDOC_WIDEN(RDOC_BRAND_GLOBAL_HOOK_RESTORE_FILENAME));

  FILE *f = NULL;
  _wfopen_s(&f, reg_backup, L"w");
  if(f)
  {
    fputws(backup.c_str(), f);
    fclose(f);
  }
  else
  {
    RDCERR("Error opening registry backup file %ls", reg_backup);
    RDCERR("Backup registry data is:\n\n%ls\n\n", backup.c_str());
  }

  return RDResult();
}

// switch error-handling to print-and-continue, as we can't really do anything about it at this
// point and we want to continue restoring in case only one thing failed.
#undef REG_CHECK
#define REG_CHECK(msg)                                                      \
  if(ret != ERROR_SUCCESS)                                                  \
  {                                                                         \
    HandleRegError(keyNative, keyWow32, ret, "Could not open AppInit key"); \
  }

void RestoreRegistry(const GlobalHookData &hookdata)
{
  HKEY keyNative = NULL;
  HKEY keyWow32 = NULL;
  LSTATUS ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, &keyNative, NULL);

  REG_CHECK("Could not open AppInit key");

#if ENABLED(RDOC_X64)
  ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0,
                        NULL, 0, KEY_READ | KEY_WRITE, NULL, &keyWow32, NULL);

  REG_CHECK("Could not open AppInit key");
#endif

  // set the native values back to where they were
  ret = RegSetValueExA(keyNative, "LoadAppInit_DLLs", 0, REG_DWORD,
                       (const BYTE *)&hookdata.dataNative.appinitEnabled,
                       sizeof(hookdata.dataNative.appinitEnabled));
  REG_CHECK("Could not set LoadAppInit_DLLs");

  ret = RegSetValueExW(keyNative, L"AppInit_DLLs", 0, REG_SZ,
                       (const BYTE *)hookdata.dataNative.appinitDLLs.c_str(),
                       DWORD(hookdata.dataNative.appinitDLLs.length() * sizeof(wchar_t)));
  REG_CHECK("Could not set AppInit_DLLs");

  // if we opened it, restore the Wow32 values as well
  if(keyWow32)
  {
    ret = RegSetValueExA(keyWow32, "LoadAppInit_DLLs", 0, REG_DWORD,
                         (const BYTE *)&hookdata.dataWow32.appinitEnabled,
                         sizeof(hookdata.dataWow32.appinitEnabled));
    REG_CHECK("Could not set LoadAppInit_DLLs");

    ret = RegSetValueExW(keyWow32, L"AppInit_DLLs", 0, REG_SZ,
                         (const BYTE *)hookdata.dataWow32.appinitDLLs.c_str(),
                         DWORD(hookdata.dataWow32.appinitDLLs.length() * sizeof(wchar_t)));
    REG_CHECK("Could not set AppInit_DLLs");
  }
}

static GlobalHookData *globalHook = NULL;

// a thread we run in the background just to keep the pipes open and wait until we're ready to stop
// the global hook.
static void GlobalHookThread()
{
  Threading::SetCurrentThreadName("GlobalHookThread");

  // keep looping doing an atomic compare-exchange to check that finished is still 0
  while(Atomic::CmpExch32(&globalHook->finished, 0, 0) == 0)
  {
    // wake every quarter of a second to test again
    Threading::Sleep(250);
  }

  char exitData[32] = "exit";

  // write some data into the pipe and close it. The data is (currently) unimportant, just that it
  // causes the blocking read on the other end to succeed and close the program.
  DWORD dummy = 0;
  if(globalHook->dataNative.pipe)
  {
    WriteFile(globalHook->dataNative.pipe, exitData, (DWORD)sizeof(exitData), &dummy, NULL);
    CloseHandle(globalHook->dataNative.pipe);
  }

  if(globalHook->dataWow32.pipe)
  {
    WriteFile(globalHook->dataWow32.pipe, exitData, (DWORD)sizeof(exitData), &dummy, NULL);
    CloseHandle(globalHook->dataWow32.pipe);
  }
}

RDResult Process::StartGlobalHook(const rdcstr &pathmatch, const rdcstr &capturefile,
                                  const CaptureOptions &opts)
{
  if(pathmatch.empty())
  {
    RETURN_ERROR_RESULT(ResultCode::InvalidParameter,
                        "Invalid global hook parameter, empty path to match");
  }

  rdcstr renderdocPath;
  FileIO::GetLibraryFilename(renderdocPath);

  renderdocPath = get_dirname(renderdocPath);

  // the native helper command is always next to the dll. Wow32 will be somewhere else
  rdcstr cmdpathNative = renderdocPath + "\\" RDOC_BRAND_CMD_EXECUTABLE;
  rdcstr cmdpathWow32;

  rdcstr shimpathNative = renderdocPath;
  rdcstr shimpathWow32;

#if ENABLED(RDOC_X64)

  // native shim is just the matching 64-bit shim dll
  shimpathNative = renderdocPath + "\\" RDOC_BRAND_SHIM_DLL_64;

  // if it looks like we're in the development environment, look for the alternate bitness in the
  // corresponding folder
  int devLocation = renderdocPath.find("\\x64\\Development");
  if(devLocation >= 0)
  {
    renderdocPath.erase(devLocation, ~0U);

    shimpathWow32 = renderdocPath + "\\Win32\\Development\\" RDOC_BRAND_SHIM_DLL_32;
    cmdpathWow32 = renderdocPath + "\\Win32\\Development\\" RDOC_BRAND_CMD_EXECUTABLE;
  }
  else
  {
    devLocation = renderdocPath.find("\\x64\\Release");

    if(devLocation >= 0)
    {
      renderdocPath.erase(devLocation, ~0U);

      shimpathWow32 = renderdocPath + "\\Win32\\Release\\" RDOC_BRAND_SHIM_DLL_32;
      cmdpathWow32 = renderdocPath + "\\Win32\\Release\\" RDOC_BRAND_CMD_EXECUTABLE;
    }
  }

  // if we're not in the dev environment, assume it's under a x86\ subfolder
  if(devLocation < 0)
  {
    shimpathWow32 = renderdocPath + "\\x86\\" RDOC_BRAND_SHIM_DLL_32;
    cmdpathWow32 = renderdocPath + "\\x86\\" RDOC_BRAND_CMD_EXECUTABLE;
  }

#else

  // nothing fancy to do here for 32-bit, just point the shim next to our dll.
  shimpathNative = renderdocPath + "\\" RDOC_BRAND_SHIM_DLL_32;

#endif

  GlobalHookData hookdata;

  // try to backup and change the registry settings to start loading our shim dlls. If that fails,
  // we bail out immediately
  RDResult regStatus = BackupAndChangeRegistry(hookdata, shimpathWow32, shimpathNative);
  if(regStatus != ResultCode::Succeeded)
    return regStatus;

  PROCESS_INFORMATION pi = {0};
  STARTUPINFO si = {0};
  SECURITY_ATTRIBUTES pSec = {0};
  SECURITY_ATTRIBUTES tSec = {0};
  pSec.nLength = sizeof(pSec);
  tSec.nLength = sizeof(tSec);

  si.cb = sizeof(si);

  // serialise to string with two chars per byte
  rdcstr optstr = opts.EncodeAsString();
  rdcstr debugLogfile = RDCGETLOGFILE();

  rdcstr params = StringFormat::Fmt(
      "\"%s\" " RDOC_BRAND_GLOBAL_HOOK_COMMAND
      " --match \"%s\" --capfile \"%s\" --debuglog \"%s\" --capopts \"%s\"",
      cmdpathNative.c_str(), pathmatch.c_str(), capturefile.c_str(), debugLogfile.c_str(),
      optstr.c_str());

  rdcwstr paramsAlloc = StringFormat::UTF82Wide(params);

  // we'll be setting stdin
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

  // hide the console window
  si.wShowWindow = SW_HIDE;

  // this is the end of the pipe that the child will inherit and use as stdin
  HANDLE childEnd = NULL;

  DWORD err;

  // create a pipe with the writing end for us, and the reading end as the child process's stdin
  {
    SECURITY_ATTRIBUTES pipeSec;
    pipeSec.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipeSec.bInheritHandle = TRUE;
    pipeSec.lpSecurityDescriptor = NULL;

    BOOL res;
    res = CreatePipe(&childEnd, &hookdata.dataNative.pipe, &pipeSec, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError, "Could not create 32-bit stdin pipe (err %u)",
                          err);
    }

    // we don't want the child process to inherit our end
    res = SetHandleInformation(hookdata.dataNative.pipe, HANDLE_FLAG_INHERIT, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError,
                          "Could not make 32-bit stdin pipe inheritable (err %u)", err);
    }

    si.hStdInput = childEnd;
  }

  // launch the process
  BOOL retValue = CreateProcessW(NULL, &paramsAlloc[0], &pSec, &tSec, true, CREATE_NEW_CONSOLE,
                                 NULL, NULL, &si, &pi);

  err = GetLastError();

  // we don't need this end anymore, the child has it
  CloseHandle(childEnd);

  if(retValue == FALSE)
  {
    CloseHandle(hookdata.dataNative.pipe);
    RestoreRegistry(hookdata);
    RETURN_ERROR_RESULT(ResultCode::InternalError, "Can't launch " RDOC_BRAND_CMD_NAME
                                                   " from '%s' (err %u)",
                        cmdpathNative.c_str(), err);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  RDCEraseEl(pi);

// repeat the process for the Wow32 renderdoccmd
#if ENABLED(RDOC_X64)
  params = StringFormat::Fmt(
      "\"%s\" " RDOC_BRAND_GLOBAL_HOOK_COMMAND
      " --match \"%s\" --capfile \"%s\" --debuglog \"%s\" --capopts \"%s\"",
      cmdpathWow32.c_str(), pathmatch.c_str(), capturefile.c_str(), debugLogfile.c_str(),
      optstr.c_str());

  paramsAlloc = StringFormat::UTF82Wide(params);

  {
    SECURITY_ATTRIBUTES pipeSec;
    pipeSec.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipeSec.bInheritHandle = TRUE;
    pipeSec.lpSecurityDescriptor = NULL;

    BOOL res;
    res = CreatePipe(&childEnd, &hookdata.dataWow32.pipe, &pipeSec, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError, "Could not create 64-bit stdin pipe (err %u)",
                          err);
    }

    res = SetHandleInformation(hookdata.dataWow32.pipe, HANDLE_FLAG_INHERIT, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError,
                          "Could not make 64-bit stdin pipe inheritable (err %u)", err);
    }

    si.hStdInput = childEnd;
  }

  retValue = CreateProcessW(NULL, &paramsAlloc[0], &pSec, &tSec, true, CREATE_NEW_CONSOLE, NULL,
                            NULL, &si, &pi);

  err = GetLastError();

  // we don't need this end anymore
  CloseHandle(childEnd);

  if(retValue == FALSE)
  {
    CloseHandle(hookdata.dataNative.pipe);
    CloseHandle(hookdata.dataWow32.pipe);
    RestoreRegistry(hookdata);
    RETURN_ERROR_RESULT(ResultCode::InternalError, "Can't launch " RDOC_BRAND_CMD_NAME
                                                   " from '%s' (err %u)",
                        cmdpathWow32.c_str(), err);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
#endif

  // set static global pointer with our data, and launch the thread
  globalHook = new GlobalHookData;
  *globalHook = hookdata;

  globalHook->pipeThread = Threading::CreateThread(&GlobalHookThread);

  return RDResult();
}

bool Process::IsGlobalHookActive()
{
  return globalHook != NULL;
}
void Process::StopGlobalHook()
{
  if(!globalHook)
    return;

  // set the finished flag and join to the thread so it closes the pipes (and so the child
  // processes)
  Atomic::Inc32(&globalHook->finished);

  Threading::JoinThread(globalHook->pipeThread);
  Threading::CloseThread(globalHook->pipeThread);

  // restore the registry settings from before we started
  RestoreRegistry(*globalHook);

  delete globalHook;
  globalHook = NULL;
}

bool Process::IsModuleLoaded(const rdcstr &module)
{
  return GetModuleHandleA(module.c_str()) != NULL;
}

void *Process::LoadModule(const rdcstr &module)
{
  HMODULE mod = GetModuleHandleA(module.c_str());
  if(mod != NULL)
    return mod;

  return LoadLibraryA(module.c_str());
}

void *Process::GetFunctionAddress(void *module, const rdcstr &function)
{
  if(module == NULL)
    return NULL;

  return (void *)GetProcAddress((HMODULE)module, function.c_str());
}

uint32_t Process::GetCurrentPID()
{
  return (uint32_t)GetCurrentProcessId();
}

void Process::Shutdown()
{
  // nothing to do
}
