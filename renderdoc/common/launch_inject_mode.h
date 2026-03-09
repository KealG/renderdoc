#pragma once

#include <stdint.h>

enum class LaunchInjectMode : uint32_t
{
  Automatic = 0,
  SuspendedOnly,
  ResumeRetry,
  LateAttach,
  Count,
};
