#pragma once
/*******************************************************************************

	@file       ParticleEngineBehavior.h

	@date       01/09/2021

	@authors    West Foulks (WestFoulks@gmail.com)

	@brief      This File houses a class that uses compute shaders to modify 
				each particles specific data. physics, transform,

*******************************************************************************/
#include "ParticleEngineEmitter.h"		//Class Definition for A Particle Emitter
#include "Bindable.h"					//Part of our graphics engine
#include "Graphics.h"					//Part of our graphics engine
#include "Camera.h"						//To fetch Camera Location

class ComputeShader;
class Graphics;
class BindMarket;

namespace ParticleEngine
{
	struct PARTICLE;
	struct cbGlobalParams;

	/// <summary>
	/// Info Required by the rendering portion
	/// of the particle engine.
	/// </summary>
	struct RenderInfo
	{
		ID3D11ShaderResourceView* particles;
		UINT AliveParticles;
	};

	class Behavior
	{

	public:

		//Constructors
		Behavior(Graphics& gfx) noexcept;
		~Behavior() noexcept;
		Behavior(const Behavior&) = default;
		Behavior& operator=(const Behavior&) = default;
		Behavior(Behavior&&) = default;
		Behavior& operator=(Behavior&&) = delete;

		void Update(EmitterManager& emitterManager);

	private:
		//Forward Refernce
		typedef struct DispatchInput DispatchInput;

		UINT totalAliveParticles_;

		/// <summary>
		/// Allows use of graphics functions 
		/// </summary>
		Graphics& gfx;

		/// <summary>
		/// Resource Manager used to store Textures, Meshs, Samplers, Ect
		/// </summary>
		ResourceManager& resourceManager_;

		//The Compute Shader ran across all particles
		ID3D11ComputeShader* csParticleShader_;
		
		//allows for sampling of textures
		Sampler* sampler_;
		
		//GlobalParameters Direct X buffers
		ID3D11Buffer* cbGParameters_ = nullptr;
		ID3D11Buffer* cbEmitterParameters_ = nullptr;

		//------------------------------------------
		//helper Functions

		//Creates Buffers for compute Shader
		void CreateBuffers(ID3D11Device* device); 

		//Add Shaders to the file
		void AddComputeShaders(ID3D11Device* device);

		//Dispatches the default compute shader for the behaviors
		void DispatchDefaultCompute(ID3D11DeviceContext* deviceContex, DispatchInput* input, EmitterData* emitter);

		//Map Global Params
		void MapGlobalParams();

		//Maps Params from a emitter object
		void MapEmitterParams(EmitterData* emitter);
	};

}
