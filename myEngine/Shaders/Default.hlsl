//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
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
	float4 PosH    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH   : POSITION1;
    float3 PosW    : POSITION2;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
	//MaterialData matData = gMaterialData[gMaterialIndex];
	
#ifdef SKINNED
    // float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // weights[0] = vin.BoneWeights[0];
    // weights[1] = vin.BoneWeights[1];
    // weights[2] = vin.BoneWeights[2];
    // weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    // float4 posL = float4(0,0,0,0);
    // float4 normalL = float4(0,0,0,0);
    // float4 tangentL = float4(0,0,0,0);
    // for(int i = 0; i < 4; ++i)
    // {
    //     // Assume no nonuniform scaling when transforming normals, so 
    //     // that we do not have to use the inverse-transpose.

    //     posL += weights[i] * mul(vin.PosL, gBoneTransforms[vin.BoneIndices[i]]);
    //     normalL += weights[i] * mul(vin.NormalL, gBoneTransforms[vin.BoneIndices[i]]);
    //     tangentL += weights[i] * mul(vin.TangentL.xyz, gBoneTransforms[vin.BoneIndices[i]]);
    // }
            float4x4 mxBonesTrans 
        = vin.BoneWeights[0] * gBoneTransforms[vin.BoneIndices[0]]
        + vin.BoneWeights[1] * gBoneTransforms[vin.BoneIndices[1]]
        + vin.BoneWeights[2] * gBoneTransforms[vin.BoneIndices[2]]
        + vin.BoneWeights[3] * gBoneTransforms[vin.BoneIndices[3]];
    vin.PosL =mul(vin.PosL,mxBonesTrans);
    vin.NormalL =float4(mul(vin.NormalL.xyz,(float3x3)mxBonesTrans),0);
    vin.TangentL =float4(mul(vin.TangentL.xyz,(float3x3)mxBonesTrans),1.0f);
#endif

    // Transform to world space.
    float4 posW = mul(vin.PosL, gWorld);    
    vout.PosW = posW.xyz;
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = float4(mul(vin.NormalL.xyz, (float3x3)gWorld),0);
	
	vout.TangentW =float4(mul(vin.TangentL.xyz, (float3x3)gWorld),1.0f);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    // Generate projective tex-coords to project SSAO map onto scene.
    vout.SsaoPosH = mul(posW, gViewProjTex);
	
	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, MatTransform).xy;

    // Generate projective tex-coords to project shadow map onto scene.
    vout.ShadowPosH = mul(posW, gShadowTransform);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
	//MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = DiffuseAlbedo;
	float3 fresnelR0 = FresnelR0;
	float  roughness = Roughness;
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
    pin.NormalW = normalize(pin.NormalW);
    float4     normalMapSample = NormalMap.Sample(gsamAnisotropicWrap, pin.TexC);
#ifdef SKINNED
    return diffuseAlbedo;
#endif
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

	// Uncomment to turn off normal mapping.
    //bumpedNormalW = pin.NormalW;

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Finish texture projection and sample SSAO map.
    pin.SsaoPosH /= pin.SsaoPosH.w;
    float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;

    // Light terms.
    float4 ambient = ambientAccess*gAmbientLight*diffuseAlbedo;

    // Only the first light casts a shadow.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
	// Add in specular reflections.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
	
    litColor.a = diffuseAlbedo.a;
    //return float4(bumpedNormalW,1.0f);
    return litColor;
}


