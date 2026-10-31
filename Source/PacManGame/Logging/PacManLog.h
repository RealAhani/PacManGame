#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Declares the category. Other files include this header to use it.
// Args: name, default runtime verbosity, compile-time max verbosity.
DECLARE_LOG_CATEGORY_EXTERN(LogPacMan, Log, All);
