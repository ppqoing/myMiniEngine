//***************************************************************************************
// DrawNormals.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
	float4 PosL    : POSITION;
    float4 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float4 TangentL : TANGENT;
#ifdef SKINNED
    uint4 BoneIndices  : BONEINDICES;
    float4 BoneWeights : WEIGHTS;
#endif
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float4 NormalW  : NORMAL;
	float4 TangentW : TANGENT;
	float2 TexC     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
	//MaterialData matData = gMaterialData[gMaterialIndex];
	
#ifdef SKINNED
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        // Assume no nonuniform scaling when transforming normals, so 
        // that we do not have to use the inverse-transpose.

        posL += weights[i] * mul(vin.PosL, gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(vin.NormalL.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        tangentL += weights[i] * mul(vin.TangentL.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
    }

    vin.PosL = float4(posL,1.0f);
    vin.NormalL = float4(normalL,0);
    vin.TangentL.xyz = tangentL;
    //             float4x4 mxBonesTrans 
    //     = vin.BoneWeights[0] * gBoneTransforms[vin.BoneIndices[0]]
    //     + vin.BoneWeights[1] * gBoneTransforms[vin.BoneIndices[1]]
    //     + vin.BoneWeights[2] * gBoneTransforms[vin.BoneIndices[2]]
    //     + vin.BoneWeights[3] * gBoneTransforms[vin.BoneIndices[3]];
    // vin.PosL =mul(vin.PosL,mxBonesTrans);
    // vin.NormalL = mul(vin.NormalL,mxBonesTrans);
    // vin.TangentL = mul(vin.TangentL,mxBonesTrans);
#endif

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL,gWorld);
	vout.TangentW.xyz = mul(vin.TangentL.xyz, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    float4 posW = mul(vin.PosL, gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, MatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
	//MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = DiffuseAlbedo;
	uint diffuseMapIndex = DiffuseMapIndex;
	uint normalMapIndex = NormalMapIndex;
	
    // Dynamically look up the texture in the array.
    diffuseAlbedo *= DiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    // Discard pixel if texture alpha < 0.1.  We do this test as soon 
    // as possible in the shader so that we can potentially exit the
    // shader early, thereby skipping the rest of the shader code.
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW.xyz = normalize(pin.NormalW.xyz);
	
    // NOTE: We use interpolated vertex normal for SSAO.

    // Write normal in view space coordinates
    float3 normalV = mul(pin.NormalW.xyz,(float3x3) gView);
    return pin.NormalW;
}


