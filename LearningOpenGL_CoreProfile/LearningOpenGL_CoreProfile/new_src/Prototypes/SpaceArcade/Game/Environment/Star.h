#pragma once

#include <cstdint>
#include <vector>
#include "../SpaceArcade.h"
#include "../SASpaceArcadeGlobalConstants.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/AdvancedPtrs.h"
#include "../../GameFramework/SAGameBase.h"


namespace SA
{
	//forward delcarations
	class SphereMeshTextured;
	class Shader;

	using StarContainer = std::vector<fwp<const class Star>>;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents a local star to the solar system; these influence the directional lighting of a scene
	//	
	//		for reference
	//		glm::vec3(1.0, 0.886, 0.533); is color of sun as seen from earth; which isn't a correct color in space, but I like it.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Star final : public GameEntity, public RemoveCopies
	{
	public:
		const uint32_t MAX_STAR_DIR_LIGHTS = GameBase::getConstants().MAX_DIR_LIGHTS;
		static const StarContainer& getSceneStars() { return sceneStars; }
	public:
		Star();
		~Star();
	public:
		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) const;
		glm::vec3 getLightDirection() const;
		void setXform(Transform& xform) { this->xform = xform; }
		void setForceCentered(bool bInForceCentered) { bForceCentered = bInForceCentered; }
		glm::vec3 getLightLDR() { return ldr_color; }
		glm::vec3 getLightHDR() { return ldr_color * hdr_intensity; }
	protected:
		virtual void postConstruct() override;
	private: //statics
		static sp<SphereMeshTextured> starMesh;
		static uint32_t starInstanceCount;
		static sp<Shader> starShader;
		static StarContainer sceneStars;
	private: //instance variables
		Transform xform;
		glm::vec3 ldr_color = glm::vec3(1.0f);
		float hdr_intensity = 1000.0f;
		bool bUseHDR = false;
		bool bForceCentered = true;
	};

}