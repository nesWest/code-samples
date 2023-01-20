/*******************************************************************************

	@file       ParticleEngineBehavior.cpp

	@date       01/09/2021

	@authors    West Foulks (WestFoulks@gmail.com)

	@brief      This File houses a class that uses compute shaders to modify 
				each particles specific data. physics, transform

*******************************************************************************/
#include "stdafx.h" 				//Header included in all files.
#include "ParticleEngineBehavior.h" //This files header
#include "ParticleEngineEmitter.h"	//Class Definition for object containing particles
#include <d3dcompiler.h>			//DirectX header
#include "Bindable.h"				//Part of our graphics engine
#include "Graphics.h"				//Part of our graphics engine
#include "Clock.h"					//Contains Delta Time
#include "Camera.h"					//To fetch Camera Location
#include "ResourceManager.h"		//Used to store resources. Textures, Meshs, Samplers
#include "Window.h"					//Contains a class that stores window Information
#include "Texture.h"				//Class Definition for The Texture Object
#include "Sampler.h"				//Class Definition for Sampler


namespace ParticleEngine
{
//Allows easier relase of Direct X Buffers
#define RELEASE(ptr){ if (ptr) { (ptr)->Release(); ptr = nullptr; } }

//Shortes the length of some lines
using XMFLOAT4 = DirectX::XMFLOAT4;
using XMFLOAT2 = DirectX::XMFLOAT2;

/// <summary>
/// cbGlobalParams are parameters that are put into every
/// compute shader dispatch call
/// </summary>
struct cbGlobalParams
{
	UINT  param[4];
	float paramf[4];
};

/// <summary>
/// cbEmitterParams are paremeters for a specific emitter
/// sent to the compute shader dispatch call
/// </summary>
struct cbEmitterParams
{
	XMFLOAT4 position;  //Emitter position xyzw
	XMFLOAT2 scale;	    //[0] x scale of the emitter [1] y scale of the emitter
	XMFLOAT2 colorData; //[0] # of colors [1]nothing
};

/// <summary>
/// A struct for easy imput of all dispatch information for
/// A compute Shader
/// </summary>
struct DispatchInput
{
	UINT aliveParticles_ = 0;
	ID3D11ShaderResourceView*  rvParticleData_IN_ = nullptr;
	ID3D11ShaderResourceView*  rvParticleData_OUT_ = nullptr;

	ID3D11UnorderedAccessView* uavParticleData_IN_ = nullptr;
	ID3D11UnorderedAccessView* uavParticleData_OUT_ = nullptr;

	ID3D11Buffer* bParticleData_IN_ = nullptr;
	ID3D11Buffer* bParticleData_OUT_ = nullptr;
};

Behavior::Behavior(Graphics& gfx) noexcept :
	gfx(gfx), resourceManager_(gfx.market)
{
	ID3D11Device* device = gfx.GetDevice();

	//Adds the needed compute shaderst to operate this calss
	AddComputeShaders(device);

	sampler_ = resourceManager_.FindResource<Sampler>("Sampler");

	CreateBuffers(device);
}

Behavior::~Behavior() noexcept
{
	RELEASE(cbGParameters_);
	RELEASE(cbEmitterParameters_);
	RELEASE(csParticleShader_);
}

void Behavior::Update(EmitterManager& emitterManager)
{
	ID3D11DeviceContext* deviceContext = Window::Instance().Gfx().GetContext();

	totalAliveParticles_ = 0;
	DispatchInput input;

	auto& manager = emitterManager.GetEmitters();
	auto emitterPtr = manager.begin();

	//retrieves all emitters from the emitter manager and dispatches a compute shader for each
	while ( emitterPtr != manager.end())
	{
		if (emitterPtr->expired())
		{
			//Cleans up unused weak ptrs
			emitterPtr = emitterManager.GetEmitters().erase(emitterPtr);
		}
		else
		{
			auto& emitter = *emitterPtr->lock().get();

			//Behavior manager has friend access to emitters
			input.aliveParticles_ = emitter.aliveParticles_;
			if (input.aliveParticles_ > 0)
			{
				//Keeps tracks of all alive particles
				totalAliveParticles_ += input.aliveParticles_;
				input.rvParticleData_IN_ = emitter.rvParticleData_IN_;
				input.rvParticleData_OUT_ = emitter.rvParticleData_OUT_;
				input.uavParticleData_OUT_ = emitter.uavParticleData_OUT_;
				input.uavParticleData_IN_ = emitter.uavParticleData_IN_;
				input.bParticleData_OUT_ = emitter.bParticleData_OUT_;
				input.bParticleData_IN_ = emitter.bParticleData_IN_;

				//Can change the render route here with checks on the emitter
				try
				{
					DispatchDefaultCompute(deviceContext, &input, &emitter);
				}
				catch (const Bindable::DirectXException)
				{
					LOG_ERROR("DirectX Exception", "Particle Engine Update Dispatch Failed");
				}
			}
			emitterPtr++;
		}
	}

}

void Behavior::CreateBuffers(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	//---------------------------------
	//Global Parameters Constant Buffer
	D3D11_BUFFER_DESC Desc;
	Desc.Usage			= D3D11_USAGE_DYNAMIC;
	Desc.BindFlags		= D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags		= 0;
	Desc.ByteWidth		= sizeof(cbGlobalParams);

	INFO_SET device->CreateBuffer(&Desc, nullptr, &cbGParameters_);
	DX_EXCEPT(hr);

	//--------------------------------
	//Emitter Parameters constant buffer
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(cbEmitterParams);

	INFO_SET device->CreateBuffer(&Desc, nullptr, &cbEmitterParameters_);
	DX_EXCEPT(hr);

}

void Behavior::AddComputeShaders(ID3D11Device* device)
{

	//Adds the nessisary compute shaders to this class.
	try
	{

		// Create Compute Shader Shader
		HRESULT hr = NULL;

		ID3DBlob* pBlob;

		//Finds the correct Compute Shader
		hr = D3DReadFileToBlob(L"./shaders/CSParticleBehaviorsDefault.cso", &pBlob);
		HR_EXCEPT(hr);

		// Create the Pixel shader from the buffer.
		hr = device->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &csParticleShader_);
		HR_EXCEPT(hr);

		pBlob->Release();

	}
	catch (const HResultException&)
	{
		LOG_ERROR("Failed DirectX", "failed to load particle behavior compute shader");
	}
}

void Behavior::DispatchDefaultCompute(ID3D11DeviceContext* deviceContext, DispatchInput* input, EmitterData* emitter)
{
	//Set the Compute shader
	deviceContext->CSSetShader(csParticleShader_, nullptr, 0u);

	// Binds particle data for the Compute Shader   
	ID3D11ShaderResourceView* rvIN[2] = { input->rvParticleData_IN_, emitter->rvColors_ };
	deviceContext->CSSetShaderResources(23, 2, rvIN);

	//For a output buffer for the compute shader
	ID3D11UnorderedAccessView* uavOut[1] = { input->uavParticleData_OUT_ };
	deviceContext->CSSetUnorderedAccessViews(0, 1, uavOut, (UINT*)(&uavOut));

	// Map Global Parameters
	MapGlobalParams();

	// Map Emitter Parameters
	MapEmitterParams(emitter);

	//Set Emitter and Global Params for the compute Shader
	ID3D11Buffer* cbIN[2] = { cbGParameters_, cbEmitterParameters_};
	deviceContext->CSSetConstantBuffers(0, 2, cbIN);

	//Set the sampler to for the compute shader
	sampler_->SetWithStage(Bindable::Stage::ComputeShader);

	//If the emitter contains a shape texture then set the texture
	if (emitter->GetEmitterShapeTexture() && emitter->GetEmitterShapeTexture()->IsLoaded())
	{
		emitter->GetEmitterShapeTexture()->SetWithStage(Bindable::Stage::ComputeShader);
	}

	// Run the Computer Shader
	deviceContext->Dispatch(input->aliveParticles_, 1, 1);

	// Ensures all buffers are unset
	ID3D11UnorderedAccessView* uavNULL[1] = { nullptr }; //Must be a pointer to a pointer
	deviceContext->CSSetUnorderedAccessViews(0, 1, uavNULL, (UINT*)(&uavOut));

	ID3D11ShaderResourceView* rvNULL[1] = { nullptr };   //Must be a pointer to a pointer
	deviceContext->CSSetShaderResources(23, 1, rvNULL);

	ID3D11Buffer* bNULL[1] = { nullptr }; 			     //Must be a pointer to a pointer
	deviceContext->CSSetConstantBuffers(0, 1, bNULL);

	deviceContext->CSSetShader(nullptr, nullptr, 0);
	
	//Swaps in and out buffers
	std::swap(input->rvParticleData_IN_, input->rvParticleData_OUT_);
	std::swap(input->uavParticleData_IN_, input->uavParticleData_OUT_);
	std::swap(input->bParticleData_IN_, input->bParticleData_OUT_);

	//Sorts the pool of particles in this emitter
	emitter->PartitionAliveDead(deviceContext);

}

void Behavior::MapGlobalParams()
{
	auto deviceContext = Window::Instance().Gfx().GetContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ZeroMemory(&MappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(cbGParameters_, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	//start mapping parameters

		//set Mapped GlobalParams
		auto globalParams = reinterpret_cast<cbGlobalParams*>(MappedResource.pData);

		//These Are temparary and may change
		globalParams->paramf[0] = Clock::DeltaTime();	//Time Since Last Frame
		globalParams->paramf[1] = 0;					//Space for additional parameters
		globalParams->param[0] = 0;						//Space for additional parameters
		globalParams->param[1] = 0;						//Space for additional parameters
		
	//Finish Mapping parameters
	deviceContext->Unmap(cbGParameters_, 0);
}

void Behavior::MapEmitterParams(EmitterData* emitter)
{

	auto deviceContext = Window::Instance().Gfx().GetContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ZeroMemory(&MappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));


	deviceContext->Map(cbEmitterParameters_, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	//start mapping parameters

		//set Mapped EmitterParams
		auto emitterParams = reinterpret_cast<cbEmitterParams*>(MappedResource.pData);

		emitterParams->position = emitter->Position();
		emitterParams->scale = emitter->Scale();
		emitterParams->colorData.x = (float) emitter->NumColors();

	//Finish Mapping parameters
	deviceContext->Unmap(cbEmitterParameters_, 0);

}

}//End of Graphics Engine NameSpace