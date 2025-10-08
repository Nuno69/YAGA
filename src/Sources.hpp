// Add your plugin files here

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

#include "Readers/InventoryReader.hpp"

#include "Settings/YAGASettings.hpp"

#include "Trackers/NPCTracker.hpp"
#include "Trackers/ObjectTracker.hpp"
#include "Trackers/VobTracker.hpp"

#include "WallDetection/Sound3D.hpp"
#include "WallDetection/Raycast.hpp"

#include "AudioHelper.hpp"
#include "BlindCameraModel.hpp"
#include "Compass.hpp"
#include "SmartMap.hpp"
#include "Utils.hpp"

#include "Plugin.hpp"