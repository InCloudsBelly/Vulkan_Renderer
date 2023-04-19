#version 450

layout(std140, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
    vec4 cameraPos;
    int  lightsCount;
} ubo;


layout(std140, binding = 1) uniform SHcoefficents
{
    vec4 coefficent[25];
};


layout(binding = 2) uniform sampler2D   baseColorSampler;
layout(binding = 3) uniform sampler2D   metallicRoughnessSampler;
layout(binding = 4) uniform sampler2D   emissiveColorSampler;
layout(binding = 5) uniform sampler2D   AOsampler;
layout(binding = 6) uniform sampler2D   normalSampler;

// IBL Samplers
layout(binding = 7) uniform sampler2D   SHBRDFlutSampler;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec4 inShadowCoords;

layout(location = 0) out vec4 outColor;

struct Material
{
   vec3 albedo;
   float metallicFactor;
   float roughnessFactor;
   vec3 emissiveColor;
   float AO;
};

struct PBRinfo
{
   // cos angle between normal and light direction.
	float NdotL;          
   // cos angle between normal and view direction.
	float NdotV;          
   // cos angle between normal and half vector.
	float NdotH;          
   // cos angle between view direction and half vector.
	float VdotH;
   // Roughness value, as authored by the model creator.
    float perceptualRoughness;
   // Roughness mapped to a more linear value.
    float alphaRoughness;
   // color contribution from diffuse lighting.
	vec3 diffuseColor;    
   // color contribution from specular lighting.
	vec3 specularColor;

    // full reflectance color(normal incidence angle)
    vec3 reflectance0;
   // reflectance color at grazing angle
    vec3 reflectance90;
};


const float PI = 3.14159265359;


///////////////////////////////////////////////////////////////////////////////

vec3 calculateNormal();


vec3 getSHIrradianceContribution(vec3 normal, vec3 reflection, PBRinfo pbrInfo)
{
    return vec3(0.0f);
}

float ambient = 0.5;


void main()
{
    vec3 normal = calculateNormal();
    vec3 view = normalize(vec3(ubo.cameraPos) - inPosition);
    vec3 reflection = - normalize(reflect(view, normal));

    Material material;
    {
        material.albedo = texture(baseColorSampler, inTexCoord).rgb;
        
        material.metallicFactor = texture(metallicRoughnessSampler, inTexCoord).b;
        material.roughnessFactor = texture(metallicRoughnessSampler, inTexCoord).g;
        
        material.AO = texture(AOsampler, inTexCoord).r;
        material.AO = (material.AO < 0.01) ? 1.0 : material.AO;
        material.emissiveColor = texture(emissiveColorSampler, inTexCoord).rgb;
    }

    PBRinfo pbrInfo;
    {
        float F0 = 0.04;

        pbrInfo.NdotV = clamp(dot(normal, view), 0.001, 1.0);
        
        pbrInfo.diffuseColor = material.albedo.rgb * (vec3(1.0) - vec3(F0));
        pbrInfo.diffuseColor *= 1.0 - material.metallicFactor;
      
        pbrInfo.specularColor = mix(vec3(F0),material.albedo,material.metallicFactor);

        pbrInfo.perceptualRoughness = clamp(material.roughnessFactor, 0.04, 1.0);
        //alpha = r*r
        pbrInfo.alphaRoughness = (pbrInfo.perceptualRoughness * pbrInfo.perceptualRoughness);

        // Reflectance
        float reflectance = max(max(pbrInfo.specularColor.r, pbrInfo.specularColor.g),pbrInfo.specularColor.b);
        // - For typical incident reflectance range (between 4% to 100%) set the
        // grazing reflectance to 100% for typical fresnel effect.
	    // - For very low reflectance range on highly diffuse objects (below 4%),
        // incrementally reduce grazing reflecance to 0%.
        pbrInfo.reflectance0 = pbrInfo.specularColor.rgb;
        pbrInfo.reflectance90 = vec3(clamp(reflectance * 25.0, 0.0, 1.0));
    }


    vec3 color = getSHIrradianceContribution(normal, reflection ,pbrInfo);

    // AO
    color = material.AO * color;

    // Emissive
    color = material.emissiveColor + color;

    color = pow(color,vec3(1.0 / 2.2));

    outColor = ambient * vec4(color, 1.0);




    float Basis[25];
	float x = normal.x;
	float y = normal.y;
	float z = normal.z;
	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;


    Basis[0] = 1.f / 2.f * sqrt(1.f / PI);
    Basis[1] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (-x);
    Basis[2] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (y);
    Basis[3] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (-z);
    Basis[4] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * x * z;
    Basis[5] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * (-x * y);
    Basis[6] = 1.0 / 4.0 * 1.f / 4.f * sqrt(5.f / PI) * (3 * y2 - 1);
    Basis[7] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * (-y * z);
    Basis[8] = 1.0 / 4.0 * 1.f / 4.f * sqrt(15.f / PI) * (z2 - x2);

    Basis[9] = 0;
	Basis[10] = 0;
	Basis[11] = 0;
	Basis[12] = 0;
	Basis[13] = 0;
	Basis[14] = 0;
	Basis[15] = 0;

    Basis[16] = -0.41667 * 2.503343 * z * x * (z * z - x * x) ;
	Basis[17] = -0.41667 * -1.770131 * x * y * (3.0 * z * z - x * x);
	Basis[18] = -0.41667 * -0.946175 * z * x * (7.0 * y * y - 1.0);
	Basis[19] = -0.41667 * -0.669047 * x * y * (7.0 * y * y - 3.0);
	Basis[20] = -0.41667 * 0.105786 * (35.0 * y*y * y*y - 30.0 * y*y + 3.0);
	Basis[21] = -0.41667 * -0.669047 * z * y * (7.0 * y * y - 3.0);
	Basis[22] = -0.41667 * 0.473087 * (z * z - x * x)* (7.0 * y * y - 1.0);
    Basis[23] = -0.41667 * -1.770131 * z * y * (z * z - 3.0 * x * x);
	Basis[24] = -0.41667 * 0.625836 * (z*z * (z*z - 3.0 * x*x) - x*x * (3.0 * z*z - x*x));

    vec3 Diffuse = vec3(0,0,0);
	for (int i = 0; i < 25; i++)
		Diffuse += coefficent[i].rgb * Basis[i];
//    Diffuse += Basis[0] *  coefficent[0].rgb ;


    vec3 r = - normalize(reflect(view, normal));
     x = r.x;
	 y = r.y;
	 z = r.z;
	 x2 = x * x;
	 y2 = y * y;
	 z2 = z * z;

    Basis[0] = 1.f / 2.f * sqrt(1.f / PI);
    Basis[1] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (-x);
    Basis[2] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (y);
    Basis[3] = 2.0 / 3.0 * sqrt(3.f / (4.f * PI)) * (-z);
    Basis[4] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * x * z;
    Basis[5] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * (-x * y);
    Basis[6] = 1.0 / 4.0 * 1.f / 4.f * sqrt(5.f / PI) * (3 * y2 - 1);
    Basis[7] = 1.0 / 4.0 * 1.f / 2.f * sqrt(15.f / PI) * (-y * z);
    Basis[8] = 1.0 / 4.0 * 1.f / 4.f * sqrt(15.f / PI) * (z2 - x2);

    Basis[9] = 0;
	Basis[10] = 0;
	Basis[11] = 0;
	Basis[12] = 0;
	Basis[13] = 0;
	Basis[14] = 0;
	Basis[15] = 0;

    Basis[16] = -0.41667 * 2.503343 * z * x * (z * z - x * x) ;
	Basis[17] = -0.41667 * -1.770131 * x * y * (3.0 * z * z - x * x);
	Basis[18] = -0.41667 * -0.946175 * z * x * (7.0 * y * y - 1.0);
	Basis[19] = -0.41667 * -0.669047 * x * y * (7.0 * y * y - 3.0);
	Basis[20] = -0.41667 * 0.105786 * (35.0 * y*y * y*y - 30.0 * y*y + 3.0);
	Basis[21] = -0.41667 * -0.669047 * z * y * (7.0 * y * y - 3.0);
	Basis[22] = -0.41667 * 0.473087 * (z * z - x * x)* (7.0 * y * y - 1.0);
    Basis[23] = -0.41667 * -1.770131 * z * y * (z * z - 3.0 * x * x);
	Basis[24] = -0.41667 * 0.625836 * (z*z * (z*z - 3.0 * x*x) - x*x * (3.0 * z*z - x*x));

    vec3 Specular = vec3(0,0,0);
	for (int i = 0; i < 25; i++)
		Specular += coefficent[i].rgb * Basis[i];

    vec2 SHBRDF  = texture(SHBRDFlutSampler, vec2( max(pbrInfo.NdotV, 0.0), pbrInfo.perceptualRoughness)).rg;
    
    vec3 SHLighting = Diffuse* pbrInfo.diffuseColor + Specular * (pbrInfo.specularColor *SHBRDF.x + SHBRDF.y);

    color =  Diffuse + Specular * (pbrInfo.specularColor *SHBRDF.x + SHBRDF.y);

     // AO
    color = material.AO * color;
//
//    color = pow(color,vec3(1.0 / 2.2));

    outColor =  vec4(color, 1.0);
}



vec3 calculateNormal()
{
    vec3 tangentNormal = texture(normalSampler,inTexCoord).xyz ;

	vec3 q1 = dFdx(inPosition);
	vec3 q2 = dFdy(inPosition);
	vec2 st1 = dFdx(inTexCoord);
	vec2 st2 = dFdy(inTexCoord);

    vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
