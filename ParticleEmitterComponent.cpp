/*******************************************************************************

	@file    ParticleEmitterComponent.cpp

	@date    01/09/2021

	@authors West Foulks (WestFoulks@Gmail.com)

	@brief 

*******************************************************************************/
#include "stdafx.h"
#include "ParticleEmitterComponent.h"
#include "ParticleEngine.h"
#include "GameObject.h"
#include "Component.h"
#include "TransformComponent.h"
#include "Clock.h"
#include "ImGuiUtil.h"
#include "imgui\imgui_color_gradient.h"

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Matrix  = DirectX::SimpleMath::Matrix;

RTTR_REGISTRATION
{
	using namespace rttr;

	rttr::registration::class_<ParticleEmitterComponent>("ParticleEmitterComponent")
	.property("EmitOnTimer",		&ParticleEmitterComponent::emitOnTimer_)
	.property("EmitTime",			&ParticleEmitterComponent::emitTime_)
	.property("Albedo",				&ParticleEmitterComponent::albedo_)
	.property("ScaleMinMax",		&ParticleEmitterComponent::scale_)
	.property("LifeTimeMinMax",		&ParticleEmitterComponent::lifetime_)
	.property("EmissionsAmount",	&ParticleEmitterComponent::emissionAmount_)
	.property("DirectionMinMax",	&ParticleEmitterComponent::direction_)
	.property("UseObjectRotation",	&ParticleEmitterComponent::useObjectRotation_)
	.property("ParticleSpeed",		&ParticleEmitterComponent::speed_)
    .property("ParticleFriction",   &ParticleEmitterComponent::friction_)
	.property("OwnedParticles",			 &ParticleEmitterComponent::GetOwnedParticles,	&ParticleEmitterComponent::SetOwnedParticles)
	.property("ParticleTexture",		 &ParticleEmitterComponent::GetParticleTexture,	&ParticleEmitterComponent::SetParticleTexture)
	.property("Acceleration",			 &ParticleEmitterComponent::accel_)
	.property("EmitterShape",			 &ParticleEmitterComponent::GetShapeTexture,	&ParticleEmitterComponent::SetShapeTexture)
	.property("EmitterScale",			 &ParticleEmitterComponent::emitterScale_)
	.property("ColorGradient",		     &ParticleEmitterComponent::GetColors,			&ParticleEmitterComponent::SetColors)
	.property("ParticleImageRotation",	 &ParticleEmitterComponent::particleImageRotation_)
	.property("UseDirectionForRotation", &ParticleEmitterComponent::useDirectionForRotation_)
	.property("EmitterOffset",			 &ParticleEmitterComponent::emitterPositionOffset_)
	.constructor();
}

#ifdef _DEBUG

#pragma region Imgui Helper functions

bool ParticleEmitterComponent::EmissionsMenu()
{
	std::string tooltip;
	bool changed = false;

	//---------------------------------------
	//position offset start menu
	//makes sure static var is set correctly
	float pos[3] = { emitterPositionOffset_.x, emitterPositionOffset_.y, emitterPositionOffset_.z };
	if (ImGuiUtil::DrawVec3("Particle Lifetime", pos))
	{
		emitterPositionOffset_.x = pos[0];
		emitterPositionOffset_.y = pos[1];
		emitterPositionOffset_.z = pos[2];
		changed |= true;
	}
	tooltip = "Offset to apply to the emitter component";

	if (ImGui::Button("Test The Emitter"))
	{
		EmitDelay();
	}

	changed |= ImGuiUtil::DrawBool("Emit With Timer", emitOnTimer_);

	if (emitOnTimer_)
	{
		changed |= ImGuiUtil::DrawFloat("Timer in seconds", emitTime_);
		ImGuiUtil::Tooltip("The emitter will always emit every time this timer reaches zero");
	}

	if (!emitOnTimer_)
	{
		changed |= ImGuiUtil::DrawFloat("Delay in seconds", emitTime_);

		tooltip = "When EmitTimed() is called via script it will delay the emissions \n";
		tooltip += "This is usefull when layering multiple emitters at the same time";
		ImGuiUtil::Tooltip(tooltip.c_str());
	}

	if (changed |= ImGuiUtil::DrawInt("Max Particles", ownedParticles_))
	{
		if (ownedParticles_ < 2)
			ownedParticles_ = 2;

		//Recreate emitter object
		SetOwnedParticles(ownedParticles_);
	}
	ImGuiUtil::Tooltip("The max amount of particles this emitter has alive at one time. Please only use what is necessary!!!");

	changed |= ImGuiUtil::DrawInt("Amount of Emissions", emissionAmount_);
	ImGuiUtil::Tooltip("The number of particles to emit every emission");

	//---------------------------------------
	//LifeTime start menu
	//makes sure static var is set correctly
	float lifetime[2] = { lifetime_.x, lifetime_.y };
	if (ImGuiUtil::DrawVec2("Particle Lifetime", lifetime))
	{
		lifetime_.x = lifetime[0];
		lifetime_.y = lifetime[1];
		changed |= true;
	}
	tooltip = "Lifetime will be randomly chosen between "
		+ std::to_string(lifetime_.x) + " and "
		+ std::to_string(lifetime_.y);

	ImGuiUtil::Tooltip(tooltip);

	ImGuiUtil::MultiSpaceing(6);

	//---------------------------------------
	//Shape Scale Start Menu
	float scale[2] = { emitterScale_.x, emitterScale_.y };
	if (ImGuiUtil::DrawVec2("Shape Scale", scale))
	{
		emitterScale_.x = scale[0];
		emitterScale_.y = scale[1];
		changed |= true;
	}
	tooltip = "Width of"
		+ std::to_string(emitterScale_.x) + " Height of "
		+ std::to_string(emitterScale_.y);
	ImGuiUtil::Tooltip(tooltip);


	if (changed |= ImGuiUtil::DrawString("Shape Texture", emitterShape_, StringFieldType::Texture))
	{
		if (!Window::Instance().Gfx().market.ResourceExists(emitterShape_))
		{
			LOG_INFO("ParticleEmitter", "tried to set a texture that does not exist")
		}
		else
		{
			SetShapeTexture(emitterShape_);
		}
	}
	ImGuiUtil::Tooltip("The texture to assigned to an emitter that dictates where particles can spawn");

	return changed;
}

bool ParticleEmitterComponent::VisualsOnSpawnMenu()
{
	bool changed = false;
	std::string tooltip;

	if (changed |= ImGuiUtil::DrawString("Particle Texture", pTexture_, StringFieldType::Texture))
	{
		if (!Window::Instance().Gfx().market.ResourceExists(pTexture_))
		{
			LOG_INFO("ParticleEmitter", "tried to set a texture that does not exist")
		}
		else
		{
			SetParticleTexture(pTexture_);
		}
	}
	ImGuiUtil::Tooltip("The texture to assign to each particle visually.");

	ImVec4 albedo = albedo_;
	if (ImGuiUtil::DrawColor("Albedo", (float*)&albedo))
	{
		albedo_ = albedo;
		auto mark = gradient_->getMarks();
		if (mark.begin() == mark.end())
			gradient_->addMark(0,ImColor(albedo_));
		else
		{
			*mark.front() = ImGradientMark(ImColor(albedo_), mark.front()->position);
		}
		changed |= true;
	}
	ImGuiUtil::Tooltip("Selects the color to blend with the texture assigned to particles");

	//---------------------------------------
	//Scale Start Menu
	//makes sure static var is set correctly
	float scale[2] = { scale_.x, scale_.y };
	if (ImGuiUtil::DrawVec2("Scale", scale))
	{
		scale_.x = scale[0];
		scale_.y = scale[1];
		changed |= true;
	}
	tooltip = "Scale will be randomly chosen between "
		+ std::to_string(scale_.x) + " and "
		+ std::to_string(scale_.y);
	ImGuiUtil::Tooltip(tooltip);


	//---------------------------------------
	//Use Direction for 0 degrees
	changed |= ImGuiUtil::DrawBool("Use directional Rotation", useDirectionForRotation_);
	tooltip = "When checked makes 0 degrees equal the direciton of starting movement";
	ImGuiUtil::Tooltip(tooltip);

	//---------------------------------------
	//Particle Image Rotation menu
	//makes sure static var is set correctly
	float rotation[2] = { particleImageRotation_.x, particleImageRotation_.y };
	if (ImGuiUtil::DrawVec2("image Rotation", rotation))
	{
		particleImageRotation_.x = rotation[0];
		particleImageRotation_.y = rotation[1];
		changed |= true;
	}
	tooltip = "Rotation of the sprite will be randomly chosen between "
		+ std::to_string(particleImageRotation_.x) + " and "
		+ std::to_string(particleImageRotation_.y) + "in degrees";

	ImGuiUtil::Tooltip(tooltip);

	return changed;

}

bool ParticleEmitterComponent::MotionOnSpawnMenu()
{
	bool changed = false;
	std::string tooltip;

	//---------------------------------------
	//Use Object Rotation
	changed |= ImGuiUtil::DrawBool("Use Object Rotation", useObjectRotation_);
	tooltip = "When checked adds the object rotation to the Emission direction";
	ImGuiUtil::Tooltip(tooltip);

	//---------------------------------------
	//Direction Start Menu
	float direction[2] = { direction_.x, direction_.y };
	if (ImGuiUtil::DrawVec2("Emission Direction", direction))
	{
		direction_.x = direction[0];
		direction_.y = direction[1];
		changed |= true;
	}
	tooltip = " Initial Emission Direction will be randomly chosen between "
		+ std::to_string(direction_.x) + " and "
		+ std::to_string(direction_.y) + " degrees";
	ImGuiUtil::Tooltip(tooltip);

	//---------------------------------------
	//Speed Start Menu
	float speed[2] = { speed_.x, speed_.y };
	if (ImGuiUtil::DrawVec2("Initial Speed", speed))
	{
		speed_.x = speed[0];
		speed_.y = speed[1];
		changed |= true;
	}
	tooltip = "Initial Speed will be randomly chosen between "
		+ std::to_string(speed_.x) + " and "
		+ std::to_string(speed_.y);
	ImGuiUtil::Tooltip(tooltip);

	//---------------------------------------
	//Acceleration Start Menu
	float accel[2] = { accel_.x, accel_.y };
	if (ImGuiUtil::DrawVec2("Initial acceleration", accel))
	{
		accel_.x = accel[0];
		accel_.y = accel[1];
		changed |= true;
	}
	tooltip = "The acceleration in the x direction is "
		+ std::to_string(accel_.x) + " and y direction is"
		+ std::to_string(accel_.y);
	ImGuiUtil::Tooltip(tooltip);

	//Friction Start Menu
	//makes sure static var is set correctly
	float friction[2] = { friction_.x, friction_.y };
	if (ImGuiUtil::DrawVec2("Friction", friction))
	{
		friction_.x = friction[0];
		friction_.y = friction[1];
		changed |= true;

	}
	tooltip = "Friction will be randomly chosen between "
		+ std::to_string(friction_.x) + " and "
		+ std::to_string(friction_.y);
	ImGuiUtil::Tooltip(tooltip);

	return changed;
}

bool ParticleEmitterComponent::VisualsOverTimeMenu()
{
	bool changed = false;
	std::string tooltip;

	ImGui::Text("Color Over Time");
	if (ImGui::GradientEditor(gradient_, draggingMark_, selectedMark_))
	{
		//If the color gradient is altered then 
		changed |= true;

		//Fetch all the colors
		auto marks = gradient_->getMarks();
		
		//If the Mark for albedo was removed add it again
		if (marks.begin() == marks.end())
			gradient_->addMark(0, ImColor(albedo_));
		else
		{
			//Copy data to albedo just incase it changed in the ramp
			albedo_.x = marks.front()->color[0];
			albedo_.y = marks.front()->color[1];
			albedo_.z = marks.front()->color[2];
			albedo_.w = marks.front()->color[3];
		}

		//Clear the old color gradient
		ColorGradient_.clear();

		//copy data to color gradient
		for (auto mark : marks)
		{
			ParticleEngine::ColorGradientCPU colors(*mark);
			ColorGradient_.push_back(colors);
		}

		//Send color gradient to the emitter
		emitter_->ColorsGradient(ColorGradient_);

	}
	tooltip = "The ramp represents the entire lifetime of a particle.";
	ImGuiUtil::Tooltip(tooltip);

	return changed;
}

#pragma endregion

bool ParticleEmitterComponent::DrawCustomInspector()
{
	//Tracks Changes
	bool changed = false;
	std::string tooltip;

	//---------------------------------------------------------
	//Emissions Menu
	ImGuiUtil::MultiSpaceing(3);
	ImGui::Separator();
	if (ImGuiUtil::LowImportanceMenuItem("Emissions"))
	{
		//Draws the menu that manages all emissions info
		changed |= EmissionsMenu();

		ImGui::TreePop();//Ends the tree node
	}

	//---------------------------------------------------------
	//Starting Visuals Menu
	ImGuiUtil::MultiSpaceing(3);
	ImGui::Separator();
	if (ImGuiUtil::LowImportanceMenuItem("Initial Visuals"))
	{
		changed |= VisualsOnSpawnMenu();
		ImGui::TreePop(); //ends the tree node
	}

	//---------------------------------------------------------
	//Initial Motions Menu
	ImGuiUtil::MultiSpaceing(3);
	ImGui::Separator();
	if (ImGuiUtil::LowImportanceMenuItem("Initial Motion"))
	{
		changed |= MotionOnSpawnMenu();

		ImGui::TreePop();//Ends the tree node
	}

	//---------------------------------------------------------
	//Visuals over time Menu
	ImGuiUtil::MultiSpaceing(3);
	ImGui::Separator();
	if (ImGuiUtil::LowImportanceMenuItem("Visuals Over Time"))
	{
		changed = VisualsOverTimeMenu();
		ImGui::TreePop();//Ends the tree node
	}

	return changed;

}

#endif // _DEBUG

#pragma region Constructor

ParticleEmitterComponent::ParticleEmitterComponent() noexcept :
	Component("ParticleEmitterComponent"),
	emitterPositionOffset_(0,0,0),
	emitOnTimer_(false),
	emitTime_(1.0f),
	lastEmission_(0.0f),
	albedo_(1.f),
	lifetime_(1.f, 1.f),
	scale_(.05, .05),
	emissionAmount_(10),
	direction_(0.0f, 360.0f),
	useObjectRotation_(true),
	speed_(1.0f, 1.0f),
	friction_(0.f, 0.f),
	accel_(0.f, 0.f),
	ownedParticles_(300),
	emitterScale_(0.f, 0.f),
	pTexture_("WhiteParticle"),
	emitterShape_("BlueCircle"),
	emitter_(),
	ColorGradient_(1),
	particleImageRotation_(0,0),
	useDirectionForRotation_(false)
{

#ifdef _DEBUG
	//For Imgui Menu
	gradient_ = new ImGradient;
#endif
	SetOwnedParticles(ownedParticles_);
}

ParticleEmitterComponent::ParticleEmitterComponent(const ParticleEmitterComponent& tocopy, bool isExact) noexcept :
Component(tocopy, isExact),
emitterPositionOffset_(tocopy.emitterPositionOffset_),
emitOnTimer_(tocopy.emitOnTimer_),
emitTime_(tocopy.emitTime_),
lastEmission_(tocopy.lastEmission_),
albedo_(tocopy.albedo_),
lifetime_(tocopy.lifetime_),
scale_(tocopy.scale_),
emissionAmount_(tocopy.emissionAmount_),
direction_(tocopy.direction_),
useObjectRotation_(tocopy.useObjectRotation_),
speed_(tocopy.speed_),
friction_(tocopy.friction_),
accel_(tocopy.accel_),
ownedParticles_(tocopy.ownedParticles_),
pTexture_(tocopy.pTexture_),
emitterScale_(tocopy.emitterScale_),
emitterShape_(tocopy.emitterShape_),
emitter_(),
ColorGradient_(tocopy.ColorGradient_),
particleImageRotation_(tocopy.particleImageRotation_),
useDirectionForRotation_(tocopy.useDirectionForRotation_)
{


#ifdef _DEBUG
	//For Imgui color Menu
	gradient_ = new ImGradient;
#endif
	SetOwnedParticles(ownedParticles_);
}

ParticleEmitterComponent::~ParticleEmitterComponent() noexcept
{
#ifdef _DEBUG
	delete gradient_;
#endif
}

#pragma endregion

#pragma region Update/Init

void ParticleEmitterComponent::Init() noexcept
{
}

void ParticleEmitterComponent::LateUpdate() noexcept
{
	if (emitOnTimer_)
	{
		lastEmission_ += Clock::DeltaTime();
		if(lastEmission_ >= emitTime_)
		{
			lastEmission_ = 0.f;
			EmitInstant();
		}
	}
	else
	{
		if (lastEmission_ > 0.f)
		{
			lastEmission_ -= Clock::DeltaTime();
			if (lastEmission_ <= 0.f)
			{
				EmitInstant();
			}
		}
	}
}

#pragma endregion

#pragma region Emission Functions

void ParticleEmitterComponent::EmitDelay()
{
	EmitDelayByFloat(emitTime_);
}

void ParticleEmitterComponent::EmitDelayByFloat(float delay)
{
	if (emitOnTimer_ == false)
	{
		if (delay > 0.0f)
		{
			lastEmission_ = delay;
		}
		else
		{
			EmitInstant();
		}
	}
	else
	{
		EmitInstant();
	}
}

void ParticleEmitterComponent::EmitInstant()
{
	//TODO: Clean Up this funtion and Spawn particles so that its split into seperate functions

	//TODO: figure out init issue on compoennt add
	transform_ = GetGameObject()->GetComponent<TransformComponent>();
	Vector4 position(transform_->Position());
	position.w = 1.f;

	//Adjust if UseObject Rotation is true
	Vector2 direction;
	if (useObjectRotation_)
	{
		direction.x = direction_.x + (transform_->Rot() * 180.f / 3.14f);
		direction.y = direction_.y + (transform_->Rot() * 180.f / 3.14f);
	}
	else
	{
		direction = direction_;
	}

	emitter_->Position(position + emitterPositionOffset_);
	emitter_->Scale(emitterScale_);

	emitter_->SpawnParticles(
	emissionAmount_,
	lifetime_,			// Lifetime		[0] Min		[1] Max  
	scale_,				// Scale		[0] Min		[1] Max
	direction,			// Direction	[0] Min		[1] Max
	speed_,				// speed		[0] Min		[1] Max
	friction_,
	accel_,
	particleImageRotation_,
	useDirectionForRotation_
	);
}

#pragma endregion

#pragma region Getters Setters

const std::string &ParticleEmitterComponent::GetParticleTexture() const
{
	return pTexture_;
}

void ParticleEmitterComponent::SetParticleTexture(const std::string& name)
{

	if (name.size() > 0 && Window::Instance().Gfx().market.ResourceExists(pTexture_))
	{
		pTexture_ = name;
	}
	else
	{
		LOG_INFO("ParticleEmitter", "tried to set a texture that does not exist, Default set")
		pTexture_ = "WhiteParticle";

	}

	if (emitter_.get())
		emitter_->SetMainTexture(pTexture_);
}

const std::string& ParticleEmitterComponent::GetShapeTexture() const
{
	return emitterShape_;
}

void ParticleEmitterComponent::SetShapeTexture(const std::string& name)
{
	if (name.size() > 0 && Window::Instance().Gfx().market.ResourceExists(emitterShape_))
	{
		emitterShape_ = name;
	}
	else
	{
		LOG_INFO("ParticleEmitter", "tried to set a texture that does not exist, Default set")
			emitterShape_ = "WhiteParticle";

	}

	if (emitter_.get())
		emitter_->SetEmitterShapeTexture(emitterShape_);
}

void ParticleEmitterComponent::SetOwnedParticles(int amount)
{
	ownedParticles_ = amount;
	emitter_ = ParticleEngine::Engine::Instance().GetEmitterManager().CreateEmitter(ownedParticles_);
	SetParticleTexture(pTexture_);
	SetShapeTexture(emitterShape_);
	SetColors(ColorGradient_);
}

void ParticleEmitterComponent::SetColors(const std::vector<ParticleEngine::ColorGradientCPU>& colors)
{
	ColorGradient_ = colors;

	//If for some odd reason the colorgradient is empty add albedo
	if (ColorGradient_.size() == 0)
		ColorGradient_.push_back(ParticleEngine::ColorGradientCPU(albedo_, 0.f));

	//Makes sure front is alway the same as albedo
	//This is sorta a sanity check
	ColorGradient_.front().color = albedo_;

#ifdef _DEBUG

	gradient_->getMarks().clear();

	for (auto color : colors)
	{
		gradient_->addMark(color.location, ImColor(color.color));
	}

#endif // DEBUG


	emitter_->ColorsGradient(ColorGradient_);
}

#pragma endregion