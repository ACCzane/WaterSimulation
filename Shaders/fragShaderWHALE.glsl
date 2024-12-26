#version 430

in vec3 varyingNormal;
in vec3 varyingLightDir;
in vec3 varyingVertPos;
in vec2 tc;
out vec4 color;

layout (binding = 0) uniform sampler3D noiseTex;
layout (binding = 1) uniform sampler2D mainTex;

struct PositionalLight
{	vec4 ambient;  
	vec4 diffuse;  
	vec4 specular;  
	vec3 position;
};

struct Material
{	vec4 ambient;  
	vec4 diffuse;  
	vec4 specular;  
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 mv_matrix;	 
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;
uniform int isAbove;
uniform float depthOffset;

vec3 estimateWaveNormal(float offset, float mapScale, float hScale)
{	
	// 使用噪声纹理中存储的高度值估算法向量
	// 通过查找这个片段周围指定偏差距离的3个高度值实现
	// 获取当前纹理坐标的三个相邻高度值
	float h1 = (texture(noiseTex, vec3(((tc.s)    )*mapScale, depthOffset, ((tc.t)+offset)*mapScale))).r * hScale;
	float h2 = (texture(noiseTex, vec3(((tc.s)-offset)*mapScale, depthOffset, ((tc.t)-offset)*mapScale))).r * hScale;
	float h3 = (texture(noiseTex, vec3(((tc.s)+offset)*mapScale, depthOffset, ((tc.t)-offset)*mapScale))).r * hScale;

	// 使用三个获取的高度值构造三维向量
	vec3 v1 = vec3(0, h1, -1);
	vec3 v2 = vec3(-1, h2, 1);
	vec3 v3 = vec3(1, h3, 1);

	// 计算两个向量的差值（两个边的向量）
	vec3 v4 = v2-v1;
	vec3 v5 = v3-v1;

	// 使用叉积计算法线方向，并归一化
	vec3 normEst = normalize(cross(v4,v5));
	return normEst;
}

vec3 distorted(vec2 tc)
{	
	//扭曲水面下的物体
	//噪声图衍生估算出来的法向量，但是高度小很多
	vec3 estN = estimateWaveNormal(0.05, 32.0, 0.05);

	float distortStrength = 0.1;
	if(isAbove != 1) distortStrength = 0.0;		//只有当玩家在水面上时才扭曲

	vec2 distorted = tc + estN.xz * distortStrength;

	return texture(mainTex, distorted).xyz; // 使用扭曲后的坐标进行纹理采样
}

void main(void)
{	
	vec4 fogColor = vec4(0.0, 0.0, 0.2, 1.0);
	float fogStart = 10.0;
	float fogEnd = 200.0;
	float dist = length(varyingVertPos.xyz);
	float fogFactor = clamp(((fogEnd-dist) / (fogEnd-fogStart)), 0.0, 1.0);

	// normalize the light, normal, and view vectors:
	vec3 L = normalize(varyingLightDir);
	vec3 N = normalize(varyingNormal);
	vec3 V = normalize(-varyingVertPos);
	
	// 扭曲水面下的光照
	vec3 estNlt = estimateWaveNormal(0.05, 32.0, 0.5);
	float distortStrength = 0.5;
	vec2 distort = estNlt.xz * distortStrength;
	N = normalize(N + vec3(distort.x, 0.0, distort.y));

	// get the angle between the light and surface normal:
	float cosTheta = dot(L,N);
	
	// compute light reflection vector, with respect N:
	vec3 R = normalize(reflect(-L, N));
	
	// angle between the view vector and reflected light:
	float cosPhi = dot(V,R);

	// compute ADS contributions (per pixel):
	vec3 ambient = ((globalAmbient * material.ambient) + (light.ambient * material.ambient)).xyz;
	vec3 diffuse = light.diffuse.xyz * material.diffuse.xyz * max(cosTheta,0.0);
	vec3 specular = light.specular.xyz * material.specular.xyz * pow(max(cosPhi,0.0), material.shininess);
	//vec3 checkers = checkerboard(tc);
	vec3 blueColor = vec3(0.0, 0.25, 1.0);
	vec3 mixColor;

	// 如果玩家在水面上，观察到的水面下物体为正常颜色
	if (isAbove == 1)
		mixColor = distorted(tc);
	// 如果在水面下，观察到的水中物体为与蓝色的混合
	else
		mixColor = (0.5 * blueColor) + (0.5 * distorted(tc));
	
	// ADS光照
	color = vec4((mixColor * (ambient + diffuse) + specular), 1.0);

	// 在水下添加雾效
	if (isAbove != 1) color = mix(fogColor, color, pow(fogFactor,5.0));
}
