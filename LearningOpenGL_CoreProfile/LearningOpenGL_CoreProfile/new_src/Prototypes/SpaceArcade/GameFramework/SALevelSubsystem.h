#pragma once
#include "..\GameFramework\SASubsystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"
//#include "SALevel.h"

namespace SA
{
	class LevelBase;

	class LevelSubsystem : public SubsystemBase
	{
	public:
		/** Broadcasts just before level is changed */
		MultiDelegate<const sp<LevelBase>& /*currentLevel*/, const sp<LevelBase>& /*newLevel*/> onPreLevelChange;

		/** Broadcasts just after level has changed */
		MultiDelegate<const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& /*newCurrentLevel*/> onPostLevelChange;

		/** Broadcasts when level start up is finished*/
		MultiDelegate<const sp<LevelBase>& /*currentLevel*/> onLevelStartupComplete;

	public:
		void loadLevel(sp<LevelBase>& newLevel);
		void unloadLevel(sp<LevelBase>& level);
		inline const sp<LevelBase>& getCurrentLevel() { return loadedLevel; }
		
	private:
		virtual void tick(float deltaSec);
		virtual void shutdown() override;

	private:
		sp<LevelBase> loadedLevel = nullptr;
	};


}