#version 430

in vec3 varyingNormal;
in vec3 varyingLightDir;
in vec3 varyingVertPos;
in vec2 tc;
in vec4 glp;
out vec4 color;

layout (binding=0) uniform sampler2D reflectTex;
layout (binding=1) uniform sampler2D refractTex;
layout (binding=2) uniform sampler3D noiseTex;

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

vec3 estimateWaveNormal(float offset, float mapScale, float hScale)
{	
	// 使用噪声纹理中存储的高度值估算法向量
	// 通过查找这个片段周围指定偏差距离的3个高度值实现
	// 获取当前纹理坐标的三个相邻高度值
	float h1 = (texture(noiseTex, vec3(((tc.s)    )*mapScale, 0.5, ((tc.t)+offset)*mapScale))).r * hScale;
	float h2 = (texture(noiseTex, vec3(((tc.s)-offset)*mapScale, 0.5, ((tc.t)-offset)*mapScale))).r * hScale;
	float h3 = (texture(noiseTex, vec3(((tc.s)+offset)*mapScale, 0.5, ((tc.t)-offset)*mapScale))).r * hScale;

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

void main(void)
{	
	// 添加雾效果
	vec4 fogColor = vec4(0.0, 0.0, 0.2, 1.0);
	float fogStart = 10.0;
	float fogEnd = 300.0;
	float dist = length(varyingVertPos.xyz);
	float fogFactor = clamp(((fogEnd-dist) / (fogEnd-fogStart)), 0.0, 1.0);

	// normalize the light, normal, and view vectors:
	vec3 L = normalize(varyingLightDir);
	vec3 V = normalize(-varyingVertPos);
	vec3 N = estimateWaveNormal(.0002, 32.0, 16.0);

	// 添加菲涅尔效应
	vec3 NFres = normalize(varyingNormal);
	float cosFres = dot(V, NFres);
	float fresnel = acos(cosFres);
	fresnel = pow(clamp(fresnel-0.3,0.0,1.0),3);		//需要调参
			
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

	vec4 mixColor, reflectColor, refractColor, blueColor;

	blueColor = vec4(0.0, 0.25, 1.0, 1.0);

	if (isAbove == 1)
	{	
		//如果在水面上，分别计算反射和折射效果，然后混合
		refractColor = texture(refractTex, (vec2(glp.x,glp.y))/(2.0*glp.w)+0.5);		//规范屏幕坐标，将坐标范围[-1,1]修正为[0,1]
		reflectColor = texture(reflectTex, (vec2(glp.x,-glp.y))/(2.0*glp.w)+0.5);
		reflectColor = vec4((reflectColor.xyz * (ambient + diffuse) + 0.75 * specular), 1.0);
		color = mix(refractColor, reflectColor, fresnel);
		//mixColor = (0.2 * refractColor) + (1.0 * reflectColor);
	}
	else
	{	
		//如果在水面下，只计算折射效果，并加上雾效
		refractColor = texture(refractTex, (vec2(glp.x,glp.y))/(2.0*glp.w)+0.5);
		mixColor = (0.5 * blueColor) + (0.6 * refractColor);
		color = vec4((mixColor.xyz * (ambient + diffuse) + 0.75*specular), 1.0);
		color = mix(fogColor, color, pow(fogFactor,5));
	}

}
