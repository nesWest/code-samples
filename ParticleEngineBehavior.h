#pragma once
/*******************************************************************************

	@file       ParticleEngineBehavior.h

	@date       01/09/2021

	@authors    West Foulks (WestFoulks@gmail.com)

	@brief      This File houses a class that uses compute shaders to modify 
				each particles specific data. physics, transform,

*******************************************************************************/
#include "ParticleEngineEmitter.h"
#include "Bindable.h"
#include "Graphics.h"
#include "Camera.h"

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

		Behavior(Graphics& gfx) noexcept;
		~Behavior() noexcept;
		Behavior(const Behavior&) = default;
		Behavior& operator=(const Behavior&) = default;
		Behavior(Behavior&&) = default;
		Behavior& operator=(Behavior&&) = delete;

		void Update(EmitterManager& emitterManager);
		//RenderInfo GetRenderInfo();

	private:
		//Forward Refernce
		typedef struct DispatchInput DispatchInput;

		//The Most particles alive at 
		//Any on point in the systems lifetime
		//Can be used to more accuratly allocate 
		//number of particles
		UINT mostAlive_;//TODO: add again
		UINT totalAliveParticles_; //TODO: add

		/// <summary>
		/// Allows use of graphics functions 
		/// </summary>
		Graphics& gfx;

		/// <summary>
		/// The bind market to fetch resoruces
		/// </summary>
		BindMarket& mrkt_;

		//The Compute Shader ran across all particles
		ID3D11ComputeShader* csParticleShader_;
		
		//allows for sampling of textures
		Sampler* sampler_;
		
		//GlobalParameters
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
		//void OldMapGlobalParams(ID3D11DeviceContext* deviceContext);
		void MapGlobalParams();
		void MapEmitterParams(EmitterData* emitter);
	};

}
