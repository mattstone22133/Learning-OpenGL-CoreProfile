#pragma once
#include <optional>

#include "AssetConfigs/SAConfigBase.h"
#include "SAProjectileSystem.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../GameFramework/SAWorldEntity.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../Tools/DataStructures/AdvancedPtrs.h"
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"

namespace SA
{
	class ConfigBase;
	class ActiveParticleGroup;
	class RNG;

	enum class PlacementType
	{
		COMMUNICATIONS, DEFENSE, TURRET, INVALID
	};
	std::string lexToString(PlacementType enumValue);
	PlacementType stringToLex(const std::string& strValue);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the data side of the placeable object
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct PlacementSubConfig
	{
		glm::vec3 position{ 0.f };
		glm::vec3 rotation_deg{ 0.f }; //yaw, pitch, roll
		glm::vec3 scale{ 1.f };
		std::string relativeFilePath;
		std::string relativeCollisionModelFilePath;
		PlacementType placementType;
			
		std::string getFullPath(const ConfigBase& owningConfig) const;
		std::string getCollisionModelFullPath(const ConfigBase& owningConfig) const;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the in world object that is associated with a ship. eg cannon, satellite, etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShipPlacementEntity : public RenderModelEntity, public IProjectileHitNotifiable
	{
	public:
		struct TeamData
		{
			glm::vec3 color{ 1.f };
			glm::vec3 shieldColor{ 1.f };
			glm::vec3 projectileColor{ 1.f };
			sp<ProjectileConfig> primaryProjectile;
			size_t team = 0;
		};
		using Parent = RenderModelEntity;
		using TargetType = WorldEntity;
	public:
		ShipPlacementEntity() : RenderModelEntity(nullptr, Transform{})
		{}
		glm::vec3 ShieldColor() const { return shieldColor; }
		void ShieldColor(glm::vec3 val) { shieldColor = val; }
	protected:
		virtual void postConstruct() override;
		virtual void onDestroyed() override; 
	public:
		virtual void draw(Shader& shader) override;
		virtual glm::vec3 getWorldPosition() const;
		glm::vec3 getWorldForward_n() const;
		glm::vec3 getLocalForward_n() const { return forward_ln; }
		glm::vec3 getWorldUp_n() const;
		glm::vec3 getLocalUp_n() const { return up_ln	; }
		void setTeamData(const TeamData& teamData);
		void setParentXform(glm::mat4 parentXform); //#TODO #scenenodes
		const glm::mat4& getParentXform() const { return parentXform; }
		virtual void setTransform(const Transform& inTransform) override;
		/** returns the model matrix considering the parent's transform*/
		const glm::mat4& getParentXLocalModelMatrix(){ return cachedModelMat_PxL; }
		virtual void replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig);
		void adjustHP(int amount);
		const TeamData& getTeamData() { return teamData; }
		void setHasGeneratorPower(bool bValue) { bHasGeneratorPower = bValue; }
		bool hasGeneratorPower() const { return bHasGeneratorPower; }
		PlacementType getPlacementType() const { return config.placementType; }
		void setTarget(const wp<TargetType>& newTarget);
	protected:
		void setForwardLocalSpace(glm::vec3 newForward_ls);
		virtual void updateModelMatrixCache();
		const glm::mat4& getSpawnXform() { return spawnXform; }
		glm::vec3 getSpawnUp_wn();
		glm::vec3 getSpawnRight_wn();
		glm::vec3 getSpawnForward_wn();
	private:
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;
		void doDamagedFX();
		void tickDestroyingFX();
	protected:
		RNG& getRNG() { return *myRNG; }
		bool hasStartedDestructionPhase() const {return destructionTickDelegate != nullptr;}
		glm::vec3 getUp_ln() const { return up_ln; }
		glm::vec3 getRight_ln() const { return right_ln; }
		glm::vec3 getForward_ln() const { return forward_ln; }
	private: //model editor special access
		friend class ModelConfigurerEditor_Level;
	public:
		std::string modelMatrixUniform = "model";
	protected:
		std::optional<glm::vec3> hitLocation;
		fwp<TargetType> myTarget = nullptr;
	private:
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr;
		sp<CollisionData> collisionData = nullptr;
		fwp<ActiveParticleGroup> activeShieldEffect;
		PlacementSubConfig config;
		glm::mat4 cachedModelMat_PxL{ 1.f };
		glm::mat4 parentXform{ 1.f };
		glm::mat4 spawnXform{ 1.f };
		mutable std::optional<glm::vec3> cachedWorldPosition = std::nullopt; //mutable so we can lazy calculate in const virtual function
		mutable std::optional<glm::vec3> cachedWorldForward_n = std::nullopt;
		mutable std::optional<glm::vec3> cachedWorldUp_n = std::nullopt;
		std::optional<glm::vec3> cache_spawnUp_wn = std::nullopt;
		std::optional<glm::vec3> cache_spawnRight_wn = std::nullopt;
		std::optional<glm::vec3> cache_spawnForward_wn = std::nullopt;
		glm::vec3 forward_ln = glm::vec3(0,0,1);
		glm::vec3 up_ln = glm::vec3(0, 1, 0);
		glm::vec3 right_ln{ 1,0,0 };
		glm::vec3 shieldColor = glm::vec3(1.f);
		TeamData teamData;
	private: //destruction fx
		sp<MultiDelegate<>> destructionTickDelegate = nullptr;
		const float destructionTickFrequencySec = 0.1f;
		float destroyAtSec = 3.0f;
		float currentDestrutionPhaseSec = 0.f;
		bool bHasGeneratorPower = true;
		sp<RNG> myRNG = nullptr;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Turret Placement
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TurretPlacement : public ShipPlacementEntity
	{
	public:
		using Parent = ShipPlacementEntity;
	public:
		void tick(float dt_sec) override;
		glm::vec3 getWorldStationaryForward_n();
	protected:
		virtual void postConstruct() override;
		virtual void updateModelMatrixCache() override;
		//virtual void replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig) override;
	private://cached
		std::optional<glm::vec3> cache_worldStationaryForward_n;
		bool bUseDefaultBarrelLocations = true; //specific to the turret model
		size_t barrelIndex = 0;
	private:
		std::vector<glm::vec3> barrelLocations_lp; //local points
		float rotationLimit_rad = glm::radians<float>(45.f);
		float rotationSpeed_radSec = glm::radians(30.f);
		float fireCooldown_sec = 1.0f;
		float timeSinseFire_sec = 1.0f;
		bool bShootingEnabled = true;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Communications Placement
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CommunicationPlacement : public ShipPlacementEntity
	{
	public:
		using Parent = ShipPlacementEntity;
	protected:
		void tick(float dt_sec) override;
		virtual void replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig) override;
		//virtual void updateModelMatrixCache() override;
	private:
		float rotationSpeed_radsec = glm::radians(30.f);
		float fireCooldown_sec = 10.0f;
		float timeSinseFire_sec = 1.0f;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Defense Placement
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class DefensePlacement : public ShipPlacementEntity
	{

	};
}
