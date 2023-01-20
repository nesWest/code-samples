/*******************************************************************************

    @file       CSParticleBehaviorsDefault.hlsl

    @date       01/0269/2021

    @authors    West Foulks (WestFoulks@digipen.edu)

    @brief      A compute shader used to calculate data assocaited with 
                particles

*******************************************************************************/


/// <summary>
/// Parameters that are given to every emitter 
/// </summary>
cbuffer GlobalParams : register( b0 )
{

    uint4   g_param;    // pcbCS->param[0]  = Nothing;
                        // pcbCS->param[1]  = Nothing;              
    float4  g_paramf;   // pcbCS->paramf[0] = Delta Time;
                        // pcbCS->paramf[1] = nothing; 

};

/// <summary>  
/// EmitterParams are paremeters for a specific emitter
/// sent to the compute shader dispatch call
/// </summary>  
cbuffer EmitterParams : register(b1)
{

    float4 epPosition;//Emitter position xyzw
    float2 epScale;   //[0] x scale of the emitter [1] y scale of the emitter
    float2 colorData;  //[0] # of colors [1]nothing

};

/// <summary>    
/// The data that is stored within each particle to determain its behavior vissualy.
/// </summary>   
struct BehaviorData
{
    float4 pos;             // Position
    float4 vel;             // peices combined
    float4 accel;           // Accleration 
    float4 physicsPieces;   // [0] speed        [1] friction     [3] Direction Radians [4] Mass
    float4 color;           // [0] red          [1] green        [2] blue              [4] alpha
    float2 imageRotation;   // [0] radians		[1] nothing
    float2 lifetime;        // [0] timeAlive    [1] max life
    float2 scale;           // [0] scale        [1] nothing 
    float4  seed;           // [0] random seed  [1] random seed
};

StructuredBuffer<BehaviorData>   BehavorDataOld: register(t23);//Data IN
RWStructuredBuffer<BehaviorData> BehavorDataNew;//Data Out

SamplerState sam;
Texture2D tex;    // The Spawn Texture


struct Color
{
    float4 color;
    float location; //from 0 to 1;
};

StructuredBuffer<Color>   ColorGradient: register(t24); //Color Gradient in

//-----------------------------------------------------------------------------
//Random Functions

// Maximum value that can be returned by the rand function
// Need to check
#define RAND_MAX 0x7fff

//XORShift random number generator
float Rand(float seed) 
{
    seed = frac(sin(dot(float2(-seed, seed), float2(12.9898, 78.233))) * 43758.5453);
    return seed;
}

//Random Min Max
float RandomRange( float Min, float Max, float seed)
{
    return lerp(Min, Max, Rand(seed));
}


//Generates a randome position within a box set at orgin
float2 RandPositionByBox(float2 scale, float2 seed)
{
    float2 pos = 0;
    pos.x = RandomRange(-scale.x / 2.f, scale.x / 2.f, seed.x);
    pos.y = RandomRange(-scale.y / 2.f, scale.y / 2.f, seed.y);

    return pos;
}

float Fit(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    //(value - omin) / (omax - omin) * (nmax - nmin) + nmin
    return ((value - oldMin) / (oldMax - oldMin) * (newMax - newMin) + newMin);
}

//-----------------------------------------------------------------------------
//Physics Functions
float4 PositionFormula(float4 oldPos, float4 velocity, float4 accel, float dt)
{
    float4 out_position = 0;

    //Position function x = x0 + (d * t) + (.5f * a * t^2) 
    out_position = oldPos;
    out_position += (velocity * dt);
    out_position += (.5 * accel * (dt * dt));

    return out_position;
}

//-----------------------------------------------------------------------------
//forward reference
void Init(uint3 Gid, uint3 id, uint3 GTid, uint GI);

//forward reference
void Update(uint3 Gid, uint3 id, uint3 GTid, uint GI);

//-----------------------------------------------------------------------------
//State Functions (Main Init Update)
[numthreads(100, 1, 1)]
void main( uint3 Gid : SV_GroupID, uint3 id : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    //On the off chance the particle is not alive skip calculations
    if( BehavorDataOld[id.x].lifetime.x < BehavorDataOld[id.x].lifetime.y)
    {

        //if the time alive is less that zero
        if (BehavorDataOld[id.x].lifetime.x < 0.f)
        {
            //Enters here when initilizing a particle
            Init( Gid, id, GTid, GI);
        }
        else
        {
            //Enters Here When Updating a particle
            Update(Gid, id, GTid, GI);
        }

    }
}

void Init(uint3 Gid, uint3 id, uint3 GTid, uint GI)
{

    float4 out_Position = 0;
    float out_TimeAlive = 0;
    float alpha = 0;

    //Complete particle Initization steps
    out_TimeAlive = g_paramf[0];

    if (epScale.x > 0.1f && epScale.x > 0.1f)
    {

        //Declare sample location for fiting int scale range
        float2 sampleLocation = 0;

        //declare seed for altering during loop
        float2 seed = BehavorDataOld[id.x].seed.xy;

        while (alpha < .7f)
        {
            //zero to one is uv coords
            sampleLocation.x = RandomRange(0.f, 1.f, seed.x);
            sampleLocation.y = RandomRange(0.f, 1.f, seed.y);

            alpha = tex.SampleLevel(sam, sampleLocation, 0).a;

            //alters the seed so I dont get dupes
            //when looping
            seed.x++;
            seed.y++;
        }

        //Fit from old range of uv coords to new range half scale
        out_Position.x = Fit(sampleLocation.x, 0.0f, 1.0f, -epScale.x / 2.f, epScale.x / 2.f);
        out_Position.y = Fit(sampleLocation.y, 1.0f, 0.0f, -epScale.y / 2.f, epScale.y / 2.f);

    }
    else
    {
        //if scale x or y is 0 then just random position by box
        out_Position.xy = RandPositionByBox(epScale, BehavorDataOld[id.x].seed.xy);
    }

    //Use position formula to move particle correctly
    out_Position = PositionFormula(epPosition + out_Position, BehavorDataOld[id.x].vel
        , BehavorDataOld[id.x].accel, g_paramf[0]);

    //-----------------------------------------------------------------------------------
    //Sets All New Output data

    //vel function v = v0 + (a * t)
    BehavorDataNew[id.x].vel = BehavorDataOld[id.x].vel + (BehavorDataOld[id.x].accel * g_paramf[0]);

    //Applies friction to the velocity
    BehavorDataNew[id.x].vel *= (1.0f - BehavorDataOld[id.x].physicsPieces[2] * g_paramf[0]);
    BehavorDataNew[id.x].seed = BehavorDataOld[id.x].seed;

    //Sets Data
    BehavorDataNew[id.x].pos = out_Position;
    BehavorDataNew[id.x].lifetime.x = out_TimeAlive;
    
    BehavorDataNew[id.x].color = ColorGradient[0].color;

}

void Update(uint3 Gid, uint3 id, uint3 GTid, uint GI)
{
    float4 out_Position = 0;
    float out_TimeAlive = 0;
    float alpha = 0;

    //Adjust time alive 
    out_TimeAlive = BehavorDataOld[id.x].lifetime.x + g_paramf[0];

    //Calculate Position
    out_Position = PositionFormula(BehavorDataOld[id.x].pos, BehavorDataOld[id.x].vel
        , BehavorDataOld[id.x].accel, g_paramf[0]);

    //-------------------------------------------------------------------------
    //Sets All New Output data

    //vel function v = v0 + (a * t)
    BehavorDataNew[id.x].vel = BehavorDataOld[id.x].vel + (BehavorDataOld[id.x].accel * g_paramf[0]);

    //Applies friction to the velocity
    BehavorDataNew[id.x].vel *= (1.0f - BehavorDataOld[id.x].physicsPieces[2] * g_paramf[0]);
    BehavorDataNew[id.x].seed = BehavorDataOld[id.x].seed;

    //Sets Data
    BehavorDataNew[id.x].pos = out_Position;
    BehavorDataNew[id.x].lifetime.x = out_TimeAlive;


    //Finds color for this time
    if (colorData.x > 1)
    {
        //Finds the percentage of life passed
        float lifePercentage = BehavorDataOld[id.x].lifetime.x / BehavorDataOld[id.x].lifetime.y;

        //Finds the next color
        if (ColorGradient[0].location < lifePercentage)
        {

            int i = 1;
            while (i < colorData.x - 1)
            {
                if (ColorGradient[i].location >= lifePercentage)
                    break;

                i++;
            }

            //Calculates the amount to lerp a color by.
            float lerpAmount = g_paramf[0] / (ColorGradient[i].location * BehavorDataOld[id.x].lifetime.y - ColorGradient[i - 1].location * BehavorDataOld[id.x].lifetime.y);

            //Sets output for Color;
            BehavorDataNew[id.x].color = lerp(BehavorDataOld[id.x].color, ColorGradient[i].color, lerpAmount);
        }
    }


}