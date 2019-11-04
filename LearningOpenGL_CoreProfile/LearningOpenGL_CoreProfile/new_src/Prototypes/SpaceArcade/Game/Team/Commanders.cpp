#include "Commanders.h"
#include "../SpaceArcade.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SAWorldEntity.h"

namespace SA
{
	TeamCommander::TeamCommander(size_t team)
	{
		createGameComponent<TeamComponent>();
		cachedTeamId = team;

		TeamComponent* teamCom = getGameComponent<TeamComponent>();
		assert(teamCom);
		if (teamCom)
		{
			teamCom->setTeam(team);
		}
	}

	sp<WorldEntity> TeamCommander::getTargetOnTeam(size_t team)
	{
		if (pendingTargetsByTeam.size() >= team + 1)
		{
			std::stack<wp<WorldEntity>>& targetStack = pendingTargetsByTeam[team];
			
			wp<WorldEntity> target;
			while (target.expired() && targetStack.size() > 0)
			{
				target = targetStack.top();
				targetStack.pop();
			}

			if (!target.expired())
			{
				return target.lock();
			}
		}

		return sp<WorldEntity>(nullptr);
	}

	sp<SA::WorldEntity> TeamCommander::getTarget()
	{
		for (size_t teamId = 0; teamId < pendingTargetsByTeam.size(); ++teamId)
		{
			if (teamId == cachedTeamId) continue;

			if (sp<SA::WorldEntity> target = getTargetOnTeam(teamId))
			{
				return target;
			}
		}

		return nullptr;
	}

	void TeamCommander::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();

		LevelSystem& levelSystem = game.getLevelSystem();
		const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();

		if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
		{
			spaceLevel->onSpawnedEntity.addWeakObj(sp_this(), &TeamCommander::handleEntitySpawned);

			for (const sp<WorldEntity>& entity : spaceLevel->getWorldEntities())
			{
				//stack up the target data
				handleEntitySpawned(entity);
			}
		}
	}

	void TeamCommander::handleEntitySpawned(const sp<WorldEntity>& spawned)
	{
		TeamComponent* myTeamComp = getGameComponent<TeamComponent>();
		if (TeamComponent* spawnTeamCom = spawned->getGameComponent<TeamComponent>())
		{
			size_t spawnedTeam = spawnTeamCom->getTeam();
			if (spawnedTeam != myTeamComp->getTeam())
			{
				if (pendingTargetsByTeam.size() < spawnedTeam + 1)
				{
					pendingTargetsByTeam.resize(spawnedTeam + 1);
				}

				std::stack<wp<WorldEntity>>& targets = pendingTargetsByTeam[spawnedTeam];
				targets.push(spawned);
			}
		}
	}

}
