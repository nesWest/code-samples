#pragma once
/*******************************************************************************

	@file 	ParticleEmitterComponent.h

	@date 	01/09/2021

	@author West Foulks (WestFoulks@gmail.com)

	@brief 	The Particle Emitter Component Class allows a Emitter object to be
			 used with our object component architeture. Adding Serlization and
			 A user interface to be implemented.

*******************************************************************************/
#include "Component.h"				//Class Definition for Components
#include "ParticleEngineEmitter.h"	//Used to Communicate with the particle engine

class TransformComponent;
typedef struct ImGradientMark ImGradientMark;
class ImGradient;

class ParticleEmitterComponent : public Component 
{

public:
#pragma region Requires


	ParticleEmitterComponent() noexcept;
	~ParticleEmitterComponent() noexcept;


	ParticleEmitterComponent(const ParticleEmitterComponent& tocopy, bool isExact = true) noexcept;
	ParticleEmitterComponent& operator=(const ParticleEmitterComponent& toCopy) = delete;
	ParticleEmitterComponent(ParticleEmitterComponent&&) = delete;

	rttr::instance Get() const noexcept override { return *this; };

	ComponentType GetType() const noexcept override { return ComponentType::ParticleEmitter; }

	std::unique_ptr<Component> Copy(bool isExact) const noexcept override { return { std::make_unique<ParticleEmitterComponent>(*this, isExact) }; }

	std::vector<ComponentType> GetComponentPrerequisites() const noexcept override
	{
		return {ComponentType::Transform};
	}

#ifdef _DEBUG
	bool HasCustomInspector() const noexcept override { return true; }
#endif

#pragma endregion

#ifdef _DEBUG	
	bool DrawCustomInspector();
#endif
	void Update() noexcept override {};

	void LateUpdate() noexcept override;

	void Init() noexcept override;

	/// <summary>
	/// The following functions are for Primarily intended for Lua Scripting.
	/// 
	/// A User can edit the visuals with GUI
	/// Then call these to emmit the specific patterns created with it.
	/// </summary>

	/// <summary>
	/// Will emit particles after the delay that was set inside the 
	/// component editor;
	/// </summary>
	void EmitDelay();

	/// <summary>
	/// Will emit particles after a specified delay;
	/// </summary>
	void EmitDelayByFloat(float delay);

	/// <summary>
	/// Used this fuction to always emit instantly
	/// </summary>
	void EmitInstant();

	/// <summary>
	/// Returns the timer used to determine emission rate
	/// </summary>
	bool GetEmitOnTimer() { return emitOnTimer_; }
	
	/// <summary>
	/// Sets the time used to determine emission rates.
	/// </summary>
	void SetEmitOnTimer(bool emit) { emitOnTimer_ = emit; }

	//Other Getters and setters
	//To actaully change these variables you must use the setters any attempt to
	//set these directly WILL result in bugs and possibly crashes
	const std::string& GetParticleTexture() const;
	void SetParticleTexture(const std::string& name);

	const std::string& GetShapeTexture()const;
	void SetShapeTexture(const std::string& name);

	int GetOwnedParticles() const { return ownedParticles_; }
	void SetOwnedParticles(int amount);

	const std::vector<ParticleEngine::ColorGradientCPU> & GetColors() const { return ColorGradient_; }
	void SetColors(const std::vector<ParticleEngine::ColorGradientCPU>& colors);



private:
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Vector4 = DirectX::SimpleMath::Vector4;
	using Vector2 = DirectX::SimpleMath::Vector2;
	using Matrix = DirectX::SimpleMath::Matrix;

	TransformComponent* transform_;

	//This share pointer is the owner
	//if it goes out of scope the emitter is destroyed
	std::shared_ptr<ParticleEngine::EmitterData> emitter_;

	//The position of the emitter reltive to the game object its attqached to.
	Vector3 emitterPositionOffset_;

	//Standard Texture assigned to a particle on draw
	std::string pTexture_;

	//Particle Emitter Shape
	std::string emitterShape_; // a texture with alpha
	Vector2		emitterScale_; // Scale of the emitter

	//Emmission behavior 
	bool	emitOnTimer_;
	float	emitTime_; //In Seconds
	float   lastEmission_;

	int	emissionAmount_;
	int ownedParticles_;

	//Particle Setting Data
	Vector4 albedo_;
	Vector2 lifetime_; // [0]min [1]max
	Vector2 scale_;	   // [0]min [1]max

	bool	useDirectionForRotation_; //determains if you want to use the direction of movement as 0
	Vector2 particleImageRotation_;  // [0]min [1]max degrees

	//Physics
	Vector2 direction_;// [0]min [1]max
	bool useObjectRotation_; 

	Vector2 speed_;    // [0]min	[1]max
	Vector2 friction_; // [0]min	[1]max
	Vector2 accel_;    // [0]X direction [1] y direction 
	
	std::vector<ParticleEngine::ColorGradientCPU> ColorGradient_;

#ifdef _DEBUG	
	/// <summary>
	/// A helper functions for imgui menu
	/// </summary>
	/// <returns>changed value or unchanged value</returns>
	bool EmissionsMenu();
	bool VisualsOnSpawnMenu();
	bool MotionOnSpawnMenu();
	bool VisualsOverTimeMenu();

	//For Imgui Color Editor
	ImGradient* gradient_;
	ImGradientMark* draggingMark_ = nullptr;
	ImGradientMark* selectedMark_ = nullptr;
#endif


	RTTR_ENABLE(Component);  //Used For Component Serilization
	RTTR_REGISTRATION_FRIEND //Used For Component Serilization
};
