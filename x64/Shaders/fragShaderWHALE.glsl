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
	// ʹ�����������д洢�ĸ߶�ֵ���㷨����
	// ͨ���������Ƭ����Χָ��ƫ������3���߶�ֵʵ��
	// ��ȡ��ǰ����������������ڸ߶�ֵ
	float h1 = (texture(noiseTex, vec3(((tc.s)    )*mapScale, depthOffset, ((tc.t)+offset)*mapScale))).r * hScale;
	float h2 = (texture(noiseTex, vec3(((tc.s)-offset)*mapScale, depthOffset, ((tc.t)-offset)*mapScale))).r * hScale;
	float h3 = (texture(noiseTex, vec3(((tc.s)+offset)*mapScale, depthOffset, ((tc.t)-offset)*mapScale))).r * hScale;

	// ʹ��������ȡ�ĸ߶�ֵ������ά����
	vec3 v1 = vec3(0, h1, -1);
	vec3 v2 = vec3(-1, h2, 1);
	vec3 v3 = vec3(1, h3, 1);

	// �������������Ĳ�ֵ�������ߵ�������
	vec3 v4 = v2-v1;
	vec3 v5 = v3-v1;

	// ʹ�ò�����㷨�߷��򣬲���һ��
	vec3 normEst = normalize(cross(v4,v5));
	return normEst;
}

vec3 distorted(vec2 tc)
{	
	//Ť��ˮ���µ�����
	//����ͼ������������ķ����������Ǹ߶�С�ܶ�
	vec3 estN = estimateWaveNormal(0.05, 32.0, 0.05);

	float distortStrength = 0.1;
	if(isAbove != 1) distortStrength = 0.0;		//ֻ�е������ˮ����ʱ��Ť��

	vec2 distorted = tc + estN.xz * distortStrength;

	return texture(mainTex, distorted).xyz; // ʹ��Ť�������������������
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
	
	// Ť��ˮ���µĹ���
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

	// ��������ˮ���ϣ��۲쵽��ˮ��������Ϊ������ɫ
	if (isAbove == 1)
		mixColor = distorted(tc);
	// �����ˮ���£��۲쵽��ˮ������Ϊ����ɫ�Ļ��
	else
		mixColor = (0.5 * blueColor) + (0.5 * distorted(tc));
	
	// ADS����
	color = vec4((mixColor * (ambient + diffuse) + specular), 1.0);

	// ��ˮ�������Ч
	if (isAbove != 1) color = mix(fogColor, color, pow(fogFactor,5.0));
}
