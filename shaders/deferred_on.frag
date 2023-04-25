#version 450

layout(std140, binding = 0) uniform NormalInfos
{
	mat4 lightSpace;
    vec4 cameraPos;
    int  lightsCount;
} uboInfo;

struct Light
{
    vec4    pos;
    vec4    dir;
    vec4    color;
    float   attenuation;
    float   radius;
    float   intensity;
    int     type;
};

layout(std140, binding = 1) uniform Lights
{
    Light lights[10];
};

layout (input_attachment_index = 0, binding = 2) uniform subpassInput samplerposition;
layout (input_attachment_index = 1, binding = 3) uniform subpassInput samplerNormal;
layout (input_attachment_index = 2, binding = 4) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 3, binding = 5) uniform subpassInput samplerMetallicRoughness;
layout (input_attachment_index = 4, binding = 6) uniform subpassInput samplerEmissiveColor;
layout (input_attachment_index = 5, binding = 7) uniform subpassInput samplerAO;


// IBL Samplers
layout(binding = 8) uniform samplerCube irradianceMapSampler;
layout(binding = 9) uniform sampler2D BRDFlutSampler;
layout(binding = 10) uniform samplerCube prefilteredEnvMapSampler;

layout(binding = 11) uniform sampler2D shadowMapSampler;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;


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

struct IBLinfo
{
   vec3 diffuseLight;
   vec3 specularLight;
//   vec3 brdf;
   vec2 brdf;
};

const float PI = 3.14159265359;



//////////////////////////////////////PBR//////////////////////////////////////

float distributionGGX(float nDotH, float rough);
float geometricOcclusion(PBRinfo pbrInfo);
vec3 fresnelSchlick(PBRinfo pbrInfo);

///////////////////////////////////////////////////////////////////////////////

vec3 calculateNormal();
vec3 calculateDirLight(int i,vec3 fragPos,vec3 normal,vec3 view,Material material,PBRinfo pbrInfo);
vec3 calculatePointLight(int i,vec3 fragPos,vec3 normal,vec3 view,Material material,PBRinfo pbrInfo);
vec3 calculateSpotLight(int i,vec3 fragPos,vec3 normal,vec3 view,Material material,PBRinfo pbrInfo);

vec3 calculateDirLight(int i, Material material, PBRinfo pbrInfo);
void calculatePointLight();

float filterPCF(vec3 shadowCoords);
float calculateShadow(vec3 shadowCoords, vec2 off);

vec3 getIBLcontribution(PBRinfo pbrInfo, IBLinfo iblInfo, Material material);
float ambient = 0.5;


void main() 
{
	// 从以前的子通道读取G-Buffer值
	vec3 fragPos = subpassLoad(samplerposition).rgb;

	vec3 normal = subpassLoad(samplerNormal).rgb;
	vec3 view = normalize(uboInfo.cameraPos.xyz - fragPos);
    vec3 reflection = - normalize(reflect(view, normal));
    reflection.y = -reflection.y;

	 Material material;
    {
        material.albedo = subpassLoad(samplerAlbedo).rgb;
        
        material.metallicFactor = subpassLoad(samplerMetallicRoughness).b;
        material.roughnessFactor = subpassLoad(samplerMetallicRoughness).g;

		material.AO = subpassLoad(samplerAO).r;
        material.AO = (material.AO < 0.01) ? 1.0 : material.AO;

		material.emissiveColor = subpassLoad(samplerEmissiveColor).rgb;
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


     IBLinfo iblInfo;
    {
        // HDR textures are already linear
        iblInfo.diffuseLight = texture(irradianceMapSampler, vec3(normal.x, -normal.y, normal.z)).rgb;

        float mipCount = float(textureQueryLevels(prefilteredEnvMapSampler));
        float lod = pbrInfo.perceptualRoughness * mipCount;

        iblInfo.brdf = texture(BRDFlutSampler, vec2( max(pbrInfo.NdotV, 0.0), 1.0 - pbrInfo.perceptualRoughness)).rg;
        iblInfo.specularLight = textureLod(prefilteredEnvMapSampler,reflection.xyz,lod).rgb;
   }

	
     vec3 color = getIBLcontribution(pbrInfo, iblInfo, material);;

     for(int i = 0 ; i < uboInfo.lightsCount; ++i)
    {
        // Directional Light
        if (lights[i].type == 0)
        {
            vec4 ShadowCoords = uboInfo.lightSpace * vec4(fragPos, 1.0f);
            float shadow = (1.0 - filterPCF(ShadowCoords.xyz / ShadowCoords.w));
            color += calculateDirLight(i, fragPos, normal,view,material,pbrInfo) * shadow;

        // Point Light
        } 
        else if(lights[i].type == 1)
        {
            color += calculatePointLight(i,fragPos,normal,view,material,pbrInfo);
        } 
        else
        {
            color += calculateSpotLight(i,fragPos,normal, view, material, pbrInfo
         );
      }
    }

    // AO
    color = material.AO * color;

    // Emissive
    color = material.emissiveColor + color;

    color = pow(color,vec3(1.0 / 2.2));

    outColor = ambient * vec4(color, 1.0);
//    outColor =  vec4(iblInfo.specularLight, 1.0);
    

}

vec3 getIBLcontribution(PBRinfo pbrInfo, IBLinfo iblInfo, Material material)
{

   vec3 diffuse = iblInfo.diffuseLight * pbrInfo.diffuseColor;
   vec3 specular = (iblInfo.specularLight *(pbrInfo.specularColor * iblInfo.brdf.x + iblInfo.brdf.y));

   return diffuse + specular;
}

float filterPCF(vec3 shadowCoords)
{
    shadowCoords.xy = shadowCoords.xy * 0.5 + 0.5;

    vec2 texelSize = textureSize(shadowMapSampler, 0);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texelSize.x);
    float dy = scale * 1.0 / float(texelSize.y);

    float shadow = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadow += calculateShadow(shadowCoords,vec2(dx * x, dy * y));
            count++;
        }
    }
    return shadow / count;
}


float calculateShadow(vec3 shadowCoords, vec2 off)
{
    if (shadowCoords.z > -1.0 && shadowCoords.z < 1.0 && shadowCoords.x > 0.0 && shadowCoords.x < 1.0&& shadowCoords.y > 0.0 && shadowCoords.y < 1.0 )
    {
        float closestDepth = texture(shadowMapSampler, shadowCoords.xy + off).r;
        float currentDepth = shadowCoords.z;

        if (closestDepth> currentDepth)
            return 0.0;
    }
    return 1.0;
}
  
vec3 calculateDirLight(int i,vec3 fragPos, vec3 normal, vec3 view, Material material, PBRinfo pbrInfo) 
{
    ////////////////////////////////////////////////////////////////////////////
    // Fills the data left for PBR
    vec3 lightDir = normalize(-vec3(lights[i].dir));

    vec3 halfway = normalize(view + lightDir);
    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }
    ////////////////////////////////////////////////////////////////////////////
    vec3 inRadiance = lights[i].intensity * lights[i].color.rbg;

    //Cook-torrance brdf
    vec3 F = fresnelSchlick(pbrInfo);
    float D = distributionGGX(pbrInfo.NdotH, material.roughnessFactor);
    float G = geometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallicFactor;

//    vec3 numerator = D * G * F;
    vec3 numerator = D*G*F;
    float denominator = (4.0 *pbrInfo.NdotV * pbrInfo.NdotL);
    
    vec3 diffuse = kD * (pbrInfo.diffuseColor / PI);
    vec3 specular = numerator / max(denominator, 0.0001);

    vec3 Lo =  (diffuse + specular) * inRadiance * pbrInfo.NdotL ;

    return Lo;
}

vec3 calculatePointLight(int i, vec3 fragPos, vec3 normal, vec3 view, Material material, PBRinfo pbrInfo ) 
{
    ////////////////////////////////////////////////////////////////////////////
    // Fills the data left for PBR
    vec3 lightDir = normalize(vec3(lights[i].pos) - fragPos);
    vec3 halfway = normalize(view + lightDir);

    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }
    ////////////////////////////////////////////////////////////////////////////


    vec3 inRadiance = lights[i].intensity * lights[i].color.rgb;

    // Cook-torrance brdf
    vec3 F = fresnelSchlick(pbrInfo);
    float D = distributionGGX(pbrInfo.NdotH, material.roughnessFactor);
    float G = geometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallicFactor;

    vec3 numerator = D * G * F;
    float denominator = 4.0 * pbrInfo.NdotV * pbrInfo.NdotL;

    vec3 diffuse = kD * (pbrInfo.diffuseColor / PI);
    vec3 specular = numerator / max(denominator, 0.0001);

    // TODO: Make these const. adjustable by the GUI.
    // Distance of 50:
    float lightConst = 1.0;
    float lightLinear = 0.09;
    float lightQuadratic = 0.032;

    float distance = length(vec3(lights[i].pos) - fragPos);
    float attenuation = ( 1.0 /( lightConst + lightLinear * distance + lightQuadratic * (distance * distance)) );

   return (attenuation * (diffuse + specular) * inRadiance * pbrInfo.NdotL);
}

vec3 calculateSpotLight(int i,vec3 fragPos, vec3 normal, vec3 view, Material material, PBRinfo pbrInfo) 
{
   ////////////////////////////////////////////////////////////////////////////
   // Fills the data left for PBR
   vec3 lightDir = normalize(vec3(lights[i].pos) - fragPos);
   vec3 halfway = normalize(view + lightDir);

   {
      pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
      pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
      pbrInfo.VdotH = max(dot(halfway, view), 0.0);
   }
   ////////////////////////////////////////////////////////////////////////////

   float theta = dot(lightDir, normalize(-vec3(lights[i].dir)));
   // TODO: Make these const. adjustable by the GUI.
   // 15 degrees
   float epsilon = 0.9978 - 0.953;
   float intensity = clamp((theta - 0.953) / epsilon, 0.0, 1.0);

   vec3 inRadiance = lights[i].intensity * lights[i].color.rgb;

   // Cook-torrance brdf
   vec3 F = fresnelSchlick(pbrInfo);
   float D = distributionGGX(pbrInfo.NdotH, material.roughnessFactor);
   float G = geometricOcclusion(pbrInfo);

   // Energy conservation
   // Specular and Diffuse
   vec3 kS = F;
   vec3 kD = vec3(1.0) - kS;
   kD *= 1.0 - material.metallicFactor;

   vec3 numerator = D * G * F;
   float denominator = 4.0 * pbrInfo.NdotV * pbrInfo.NdotL;

   vec3 diffuse = kD * (pbrInfo.diffuseColor / PI) * intensity;
   vec3 specular = numerator / max(denominator, 0.0001) * intensity;

   // TODO: Make these const. adjustable by the GUI.
   // Distance of 50:
   float lightConst = 1.0;
   float lightLinear = 0.09;
   float lightQuadratic = 0.032;

   float distance = length(vec3(lights[i].pos) - fragPos);
   float attenuation = (1.0 /(lightConst + lightLinear * distance + lightQuadratic * (distance * distance) ));

   return (attenuation * (diffuse + specular) * inRadiance * pbrInfo.NdotL);
}


///////////////////////////////PBR - Helper functions//////////////////////////

/*
 * Trowbridge-Reitz GGX approximation.
 */
float distributionGGX(float nDotH, float rough)
{
   float a = rough * rough;
   float a2 = a * a;

   float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
   denominator = 1 / (PI * denominator * denominator);

   return a2 * denominator;
}

float geometricOcclusion(PBRinfo pbrInfo)
{
   float alphaRoughness2 = pbrInfo.alphaRoughness * pbrInfo.alphaRoughness;
   float NdotL2 = pbrInfo.NdotL * pbrInfo.NdotL;
   float NdotV2 = pbrInfo.NdotV * pbrInfo.NdotV;

   float attenuationL = (
         2.0 * pbrInfo.NdotL /
         (
            pbrInfo.NdotL +
            sqrt(alphaRoughness2 + (1.0 - alphaRoughness2) * (NdotL2))
         )
   );

   float attenuationV = (
         2.0 * pbrInfo.NdotV /
         (
            pbrInfo.NdotV +
            sqrt(alphaRoughness2 + (1.0 - alphaRoughness2) * (NdotV2))
         )
   );

   return attenuationL * attenuationV;
}

/*
 * Fresnel Schlick approximation(for specular reflection).
 */
vec3 fresnelSchlick(PBRinfo pbrInfo)
{
    return (pbrInfo.reflectance0 + (pbrInfo.reflectance90 - pbrInfo.reflectance0) *pow(clamp(1.0 - pbrInfo.VdotH, 0.0, 1.0), 5.0));
}