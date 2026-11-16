// Add your plugin files here

#include "Utils/Log.hpp"
#include "Utils/Locale.hpp"
#include "Utils/String.hpp"
#include "Utils/Generic.hpp"

#include "Settings/YAGA.hpp"
#include "Compass.hpp"

#include "Speech/SpeechEngine.h"
#include "Speech/SpeechEngineZhengdu.h"
#include "Speech/SpeechEngineNVDA.h"
#include "Speech/SpeechEngineSAPI.h"
#include "Speech/Speech.hpp"

#include "Readers/CompassReader.hpp"
#include "Readers/DialogReader.hpp"
#include "Readers/FocusReader.hpp"
#include "Readers/GameReader.hpp"
#include "Readers/InventoryReader.hpp"
#include "Readers/MenuReader.hpp"

#include "SpatialCueManager.hpp"
#include "Trackers/VobTracker.hpp"
#include "WallDetection/Raycast.hpp"

#include "BlindCameraModel.hpp"
#include "VobSoundEmitter.hpp"
#include "Navigation/Sectors.hpp"
#include "Navigation/WaynetPatch_G1.hpp"
#include "Navigation/WaynetPatch.hpp"
#include "Navigation/Navigator.hpp"
#include "SmartMap.hpp"
#include "Dev/DevConsole.hpp"

#include "Hooks/oCAIHuman.hpp"
#include "Hooks/oCWorld.hpp"
#include "Hooks/oCGame.hpp"
#include "Hooks/oCItemContainer.hpp"
#include "Hooks/oCMobInter.hpp"
#include "Hooks/oCNpc.hpp"
#include "Hooks/zCAIPlayer.hpp"
#include "Hooks/zCInput.hpp"
#include "Hooks/zCMenu.hpp"
#include "Hooks/zCModel.hpp"
#include "Hooks/zCViewDialogChoice.hpp"

#include "Plugin.hpp"
