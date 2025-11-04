// Add your plugin files here

#include "Utils/Generic.hpp"
#include "Utils/String.hpp"

#include "Compass.hpp"
#include "Settings/YAGA.hpp"

#include "Speech/Speech.hpp"
#include "Speech/SpeechEngine.h"
#include "Speech/SpeechEngineNVDA.h"
#include "Speech/SpeechEngineSAPI.h"
#include "Speech/SpeechEngineZhengdu.h"

#include "Readers/CompassReader.hpp"
#include "Readers/DialogReader.hpp"
#include "Readers/FocusReader.hpp"
#include "Readers/GameReader.hpp"
#include "Readers/InventoryReader.hpp"
#include "Readers/MenuReader.hpp"

#include "Trackers/NPCTracker.hpp"
#include "Trackers/ObjectTracker.hpp"
#include "Trackers/VobTracker.hpp"

#include "WallDetection/Raycast.hpp"
#include "WallDetection/Sound3D.hpp"

#include "AudioHelper.hpp"
#include "BlindCameraModel.hpp"
#include "SmartMap.hpp"
#include "VobSoundEmitter.hpp"

#include "Hooks/oCAIHuman.hpp"
#include "Hooks/oCGame.hpp"
#include "Hooks/oCItemContainer.hpp"
#include "Hooks/oCMobInter.hpp"
#include "Hooks/oCNpc.hpp"
#include "Hooks/oCWorld.hpp"
#include "Hooks/zCAIPlayer.hpp"
#include "Hooks/zCMenu.hpp"
#include "Hooks/zCModel.hpp"
#include "Hooks/zCViewDialogChoice.hpp"

#include "Plugin.hpp"