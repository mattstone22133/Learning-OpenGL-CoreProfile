#include "SABaseLevel.h"
#include "..\..\Rendering\BuiltInShaders.h"
#include "..\..\GameFramework\SAPlayerSubsystem.h"
#include "..\..\GameFramework\SAGameBase.h"
#include "..\..\Tools\DataStructures\SATransform.h"
#include "..\..\GameFramework\SAPlayerBase.h"
#include "..\..\Rendering\Camera\SACameraBase.h"

namespace SA
{
	void BaseLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
		if (zeroPlayer)
		{
			const sp<CameraBase>& camera = zeroPlayer->getCamera();

			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0));

			forwardShadedModelShader->setUniform3f("cameraPosition", camera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);
			for (const sp<RenderModelEntity>& entity : renderEntities) 
			{
				mat4 model = glm::mat4(1.f);
				model = glm::translate(model, vec3(5.f, 0.f, -5.f));
				forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
				entity->getModel()->draw(*forwardShadedModelShader);
			}

		}
	}

	void BaseLevel::startLevel_v()
	{
		Level::startLevel_v();

		forwardShadedModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_SimpleLighting_fragSrc, false);
	}

	void BaseLevel::endLevel_v()
	{
		//this means that we're going to generate a new shader between each level transition.
		//this can be avoided with static members or by some other mechanism, but I do not see 
		//transitioning levels being a slow process currently, so each level gets its own shaders.
		forwardShadedModelShader = nullptr;

		Level::endLevel_v();
	}

}

