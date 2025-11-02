// Disable macro redefinition warning
#pragma warning(disable: 4005)

#include "resource.h"

#include <Union/Hook.h>
#include <ZenGin/zGothicAPI.h>

// Custom includes
#include <format>
#include <unordered_map>

#ifdef __G1
#define GOTHIC_NAMESPACE Gothic_I_Classic
#define ENGINE Engine_G1
HOOKSPACE(Gothic_I_Classic, GetGameVersion() == ENGINE);
HOOKSPACE_WITH_SG_RC( Gothic_I_Classic, GetGameVersion() == ENGINE, IDR_GOTHIC_1_CLASSIC_TXT, RT_RCDATA);
#include "Sources.hpp"
#endif

#ifdef __G1A
#define GOTHIC_NAMESPACE Gothic_I_Addon
#define ENGINE Engine_G1A
HOOKSPACE_WITH_SG_RC(Gothic_I_Addon, GetGameVersion() == ENGINE, IDR_GOTHIC_1_ADDON_TXT, RT_RCDATA);
#include "Sources.hpp"
#endif

#ifdef __G2
#define GOTHIC_NAMESPACE Gothic_II_Classic
#define ENGINE Engine_G2
HOOKSPACE_WITH_SG_RC(Gothic_II_Classic, GetGameVersion() == ENGINE, IDR_GOTHIC_2_CLASSIC_TXT, RT_RCDATA);
#include "Sources.hpp"
#endif

#ifdef __G2A
#define GOTHIC_NAMESPACE Gothic_II_Addon
#define ENGINE Engine_G2A
HOOKSPACE_WITH_SG_RC(Gothic_II_Addon, GetGameVersion() == ENGINE, IDR_GOTHIC_2_ADDON_TXT, RT_RCDATA);
#include "Sources.hpp"
#endif

HOOKSPACE(Global, true);