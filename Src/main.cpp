#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/soil2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "Utils.h"
#include <algorithm>
#include <array>
#include <random>

#pragma region Setup
#define PI 3.1415926535

using namespace std;

#define numVAOs 1

//0:CubeMapVPos
//1:PlaneVPos
//2:PlaneVNormal
//3:PlaneVTc
#define numVBOs 4			
float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }
#pragma endregion

#pragma region Params
//引入自定义Utils，主要做读取shader并链接到program、读取材质...的操作
Utils util = Utils();

//定义渲染程序、vao、vbo
GLuint renderingProgram_SURFACE;		//水面
GLuint renderingProgram_FLOOR;			//泳池底部
GLuint renderingProgramCubeMap;			//天空盒
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

// UniformLocation的缓存
GLuint vLoc, mvLoc, projLoc, nLoc;

// 用于给水面添加反射和折射
GLuint refractTextureId;
GLuint reflectTextureId;
GLuint refractFrameBuffer;		//用于存储折射信息的自定义帧缓冲区
GLuint reflectFrameBuffer;		//用于存储反射信息的自定义帧缓冲区

//TODO: 使用逆转置矩阵处理法线的原因在于，当进行非均匀缩放时，简单的转动或平移操作并不能保证法线仍然指向面外。使用逆转置矩阵可以确保法线在经过变换后的方向正确(本人并未深究其数学原理)。
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;			//mpv变换矩阵

GLuint skyboxTexture;									//天空盒材质

//相机相关
int width, height;										//根据window的width,height设置
float aspect;											//视野，根据相机宽高比设置
float mouseSensitivity = .5f;							//用户操作相机视角转动的灵敏度
float moveSpeed = 5.0f;									//用户移动速度
//定义相机位置和旋转
glm::vec3 camPos = glm::vec3(0.0f, 2.0f, 0.0f);			//x, y, z
glm::vec3 camRot = glm::vec3(-15.0f, 0.0f, 0.0f);		//Pitch, Yaw, Roll(0)
//用户操作相关
float lastFrame_time = 0;								//GLFW居然没有内置的deltaTime o.0
float deltaTime = 0;
bool firstMouse = true;
float lastX;
float lastY;

//定义模型世界位置
float surfacePlaneHeight = 0.0f;
float floorPlaneHeight = -10.0f;						//泳池底部高度值

#pragma region 光照
//定义光照
//光源位置
glm::vec3 lightLoc = glm::vec3(0.0f, 4.0f, -8.0f);
//全局环境光、环境光、漫反射光、镜面反射光、光源、模型的环境光、模型的漫反射光、模型的镜面反射光、模型的光泽度、是否在相机上方的uniform变量位置
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mambLoc, mdiffLoc, mspecLoc, mshiLoc, aboveLoc;
glm::vec3 currentLightPos, transformed;
float lightPos[3];

// 点光源
float globalAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// 金属材质
float matAmb[4] = { 0.5f, 0.6f, 0.8f, 1.0f };
float matDif[4] = { 0.8f, 0.9f, 1.0f, 1.0f };
float matSpe[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float matShi = 250.0f;
#pragma endregion

#pragma region 噪声

GLuint noiseTexture;									//噪声材质

const int noiseHeight = 256;
const int noiseWidth = 256;
const int noiseDepth = 256;

double noise[noiseHeight][noiseWidth][noiseDepth];
#pragma endregion

#pragma endregion

#pragma region 噪声相关函数

//平滑Noise
double smoothNoise(double zoom, double x1, double y1, double z1) {
	//取小数部分，表示百分比（用来插值）
	double fractX = x1 - (int)x1;
	double fractY = y1 - (int)y1;
	double fractZ = z1 - (int)z1;

	//相邻值索引，边界情况下考虑round到另一边界
	double x2 = x1 - 1; if (x2 < 0) x2 = (round(noiseHeight / zoom)) - 1;
	double y2 = y1 - 1; if (y2 < 0) y2 = (round(noiseWidth / zoom)) - 1;
	double z2 = z1 - 1; if (z2 < 0) z2 = (round(noiseDepth / zoom)) - 1;

	//三线性插值
	double value = 0.0;
	value += fractX * fractY * fractZ * noise[(int)x1][(int)y1][(int)z1];
	value += (1.0 - fractX) * fractY * fractZ * noise[(int)x2][(int)y1][(int)z1];
	value += fractX * (1.0 - fractY) * fractZ * noise[(int)x1][(int)y2][(int)z1];
	value += (1.0 - fractX) * (1.0 - fractY) * fractZ * noise[(int)x2][(int)y2][(int)z1];

	value += fractX * fractY * (1.0 - fractZ) * noise[(int)x1][(int)y1][(int)z2];
	value += (1.0 - fractX) * fractY * (1.0 - fractZ) * noise[(int)x2][(int)y1][(int)z2];
	value += fractX * (1.0 - fractY) * (1.0 - fractZ) * noise[(int)x1][(int)y2][(int)z2];
	value += (1.0 - fractX) * (1.0 - fractY) * (1.0 - fractZ) * noise[(int)x2][(int)y2][(int)z2];

	return value;
}

double turbulence(double x, double y, double z, double maxZoom) {
	double sum = 0.0, zoom = maxZoom;

	sum = (sin((1.0 / 512.0) * (8 * PI) * (x + z)) + 1) * 8.0;

	while (zoom >= 0.9) {
		sum = sum + smoothNoise(zoom, x / zoom, y / zoom, z / zoom) * zoom;
		zoom = zoom / 2.0;
	}

	sum = 128.0 * sum / maxZoom;
	return sum;
}

void fillDataArray(GLubyte data[]) {
	double maxZoom = 32.0;
	for (int i = 0; i < noiseHeight; i++) {
		for (int j = 0; j < noiseWidth; j++) {
			for (int k = 0; k < noiseDepth; k++) {
				data[i * (noiseWidth * noiseHeight * 4) + j * (noiseHeight * 4) + k * 4 + 0] = (GLubyte)turbulence(i, j, k, maxZoom);
				data[i * (noiseWidth * noiseHeight * 4) + j * (noiseHeight * 4) + k * 4 + 1] = (GLubyte)turbulence(i, j, k, maxZoom);
				data[i * (noiseWidth * noiseHeight * 4) + j * (noiseHeight * 4) + k * 4 + 2] = (GLubyte)turbulence(i, j, k, maxZoom);
				data[i * (noiseWidth * noiseHeight * 4) + j * (noiseHeight * 4) + k * 4 + 3] = (GLubyte)255;
			}
		}
	}
}

GLuint buildNoiseTexture() {
	GLuint textureID;
	GLubyte* data = new GLubyte[noiseHeight * noiseWidth * noiseDepth * 4];

	fillDataArray(data);

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_3D, textureID);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, noiseWidth, noiseHeight, noiseDepth);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, noiseWidth, noiseHeight, noiseDepth, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, data);

	delete[] data;
	return textureID;
}

void generateNoise() {
	for (int x = 0; x < noiseHeight; x++) {
		for (int y = 0; y < noiseWidth; y++) {
			for (int z = 0; z < noiseDepth; z++) {
				noise[x][y][z] = (double)rand() / (RAND_MAX + 1.0);			//[0, 1)的Double值
			}
		}
	}
}
#pragma endregion

#pragma region 光照相关函数
void installLights(glm::mat4 vMatrix, GLuint renderingProgram) {
	transformed = glm::vec3(vMatrix * glm::vec4(currentLightPos, 1.0));
	lightPos[0] = transformed.x;
	lightPos[1] = transformed.y;
	lightPos[2] = transformed.z;

	// get the locations of the light and material fields in the shader
	globalAmbLoc = glGetUniformLocation(renderingProgram, "globalAmbient");
	ambLoc = glGetUniformLocation(renderingProgram, "light.ambient");
	diffLoc = glGetUniformLocation(renderingProgram, "light.diffuse");
	specLoc = glGetUniformLocation(renderingProgram, "light.specular");
	posLoc = glGetUniformLocation(renderingProgram, "light.position");
	mambLoc = glGetUniformLocation(renderingProgram, "material.ambient");
	mdiffLoc = glGetUniformLocation(renderingProgram, "material.diffuse");
	mspecLoc = glGetUniformLocation(renderingProgram, "material.specular");
	mshiLoc = glGetUniformLocation(renderingProgram, "material.shininess");

	//  set the uniform light and material values in the shader
	glProgramUniform4fv(renderingProgram, globalAmbLoc, 1, globalAmbient);
	glProgramUniform4fv(renderingProgram, ambLoc, 1, lightAmbient);
	glProgramUniform4fv(renderingProgram, diffLoc, 1, lightDiffuse);
	glProgramUniform4fv(renderingProgram, specLoc, 1, lightSpecular);
	glProgramUniform3fv(renderingProgram, posLoc, 1, lightPos);
	glProgramUniform4fv(renderingProgram, mambLoc, 1, matAmb);
	glProgramUniform4fv(renderingProgram, mdiffLoc, 1, matDif);
	glProgramUniform4fv(renderingProgram, mspecLoc, 1, matSpe);
	glProgramUniform1f(renderingProgram, mshiLoc, matShi);
}
#pragma endregion

#pragma region 用户输入相关函数
void processInput(GLFWwindow* window, float deltaTime) {
#pragma region 鼠标
	// 获取当前鼠标位置
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// 如果是第一次获取鼠标位置，初始化
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// 计算鼠标偏移
	float xoffset = lastX - xpos;
	float yoffset = lastY - ypos; // 都是反向的
	lastX = xpos;
	lastY = ypos;

	// 调整偏移量
	xoffset *= mouseSensitivity;
	yoffset *= mouseSensitivity;

	// 更新相机旋转
	camRot.y += xoffset; // Yaw
	camRot.x += yoffset; // Pitch

	// 限制 pitch 值，避免翻转
	if (camRot.x > 80.0f) camRot.x = 80.0f;
	if (camRot.x < -80.0f) camRot.x = -80.0f;
#pragma endregion

#pragma region 键盘
	float cameraSpeedAdjusted = moveSpeed * deltaTime;

	// 摄像机的前向量
	glm::vec3 forwardDirection = glm::vec3(
		sin(glm::radians(camRot.y)) * cos(glm::radians(camRot.x)),  // X轴方向
		sin(glm::radians(-camRot.x)),  // Y轴方向（上下）
		cos(glm::radians(camRot.y)) * cos(glm::radians(camRot.x))   // Z轴方向
	);


	glm::vec3 rightDirection = glm::normalize(glm::cross(forwardDirection, glm::vec3(0.0f, 1.0f, 0.0f)));

	// WASD keys for forward/backward and left/right movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camPos -= cameraSpeedAdjusted * forwardDirection;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camPos += cameraSpeedAdjusted * forwardDirection;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camPos += cameraSpeedAdjusted * rightDirection;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camPos -= cameraSpeedAdjusted * rightDirection;

	// QE keys for vertical movement (up/down)
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camPos += glm::vec3(0.0f, cameraSpeedAdjusted, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camPos -= glm::vec3(0.0f, cameraSpeedAdjusted, 0.0f);


	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		mouseSensitivity -= 0.1;
		if (mouseSensitivity <= 0) {
			mouseSensitivity = 1;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		mouseSensitivity += 0.1;
#pragma endregion
}
#pragma endregion

void setupVertices(void) {
	std::vector<glm::vec3> cubMapVertices;
	std::vector<glm::vec3> planeVertices;
	std::vector<glm::vec3> planeNormals;
	std::vector<glm::vec2> planeTexCoords;

	// 调用 Utils::SetupPoolVertices 来填充 std::vector 数据
	Utils::SetupPoolVertices(cubMapVertices, planeVertices, planeNormals, planeTexCoords);

	// 绑定 VAO 和 VBO
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	// 天空盒顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, cubMapVertices.size() * sizeof(glm::vec3), cubMapVertices.data(), GL_STATIC_DRAW);

	// 平面顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(glm::vec3), planeVertices.data(), GL_STATIC_DRAW);

	// 平面顶点 UV 坐标
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, planeTexCoords.size() * sizeof(glm::vec2), planeTexCoords.data(), GL_STATIC_DRAW);

	// 平面顶点法向量
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, planeNormals.size() * sizeof(glm::vec3), planeNormals.data(), GL_STATIC_DRAW);
}

void createReflectRefractBuffers(GLFWwindow* window) {
	/*--------------------------------------------------反射-------------------------------------------------------*/
	//生成并绑定缓冲区
	GLuint bufferId[1];
	glGenBuffers(1, bufferId);
	glfwGetFramebufferSize(window, &width, &height);

	//生成并绑定反射的帧缓冲区
	glGenFramebuffers(1, bufferId);
	reflectFrameBuffer = bufferId[0];
	glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);

	//生成并配置反射纹理
	glGenTextures(1, bufferId);
	reflectTextureId = bufferId[0];
	glBindTexture(GL_TEXTURE_2D, reflectTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//将纹理附加到帧缓冲区
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectTextureId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	//生成并配置深度纹理
	glGenTextures(1, bufferId);
	glBindTexture(GL_TEXTURE_2D, bufferId[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferId[0], 0);

	/*--------------------------------------------------折射-------------------------------------------------------*/
	glGenFramebuffers(1, bufferId);
	refractFrameBuffer = bufferId[0];
	glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
	glGenTextures(1, bufferId);
	refractTextureId = bufferId[0];
	glBindTexture(GL_TEXTURE_2D, refractTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractTextureId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glGenTextures(1, bufferId);
	glBindTexture(GL_TEXTURE_2D, bufferId[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferId[0], 0);
}

void prepForSkyBoxRender() {
	glUseProgram(renderingProgramCubeMap);

	vLoc = glGetUniformLocation(renderingProgramCubeMap, "v_matrix");
	projLoc = glGetUniformLocation(renderingProgramCubeMap, "p_matrix");

	glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(vMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	//vbo[0]用于存储天空盒顶点
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
}

void prepForTopSurfaceRender() {
	glUseProgram(renderingProgram_SURFACE);

	mvLoc = glGetUniformLocation(renderingProgram_SURFACE, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram_SURFACE, "proj_matrix");
	nLoc = glGetUniformLocation(renderingProgram_SURFACE, "norm_matrix");
	aboveLoc = glGetUniformLocation(renderingProgram_SURFACE, "isAbove");

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, surfacePlaneHeight, 0.0f));
	mvMat = vMat * mMat;
	invTrMat = glm::transpose(glm::inverse(mvMat));

	currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
	installLights(vMat, renderingProgram_SURFACE);

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

	if (camPos.y > surfacePlaneHeight)
		glUniform1i(aboveLoc, 1);
	else
		glUniform1i(aboveLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);
}

void prepForFloorRender() {
	glUseProgram(renderingProgram_FLOOR);

	mvLoc = glGetUniformLocation(renderingProgram_FLOOR, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram_FLOOR, "proj_matrix");
	nLoc = glGetUniformLocation(renderingProgram_FLOOR, "norm_matrix");
	aboveLoc = glGetUniformLocation(renderingProgram_FLOOR, "isAbove");

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, floorPlaneHeight, 0.0f));
	mvMat = vMat * mMat;
	invTrMat = glm::transpose(glm::inverse(mvMat));

	currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
	installLights(vMat, renderingProgram_FLOOR);

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

	if (camPos.y >= surfacePlaneHeight)
		glUniform1i(aboveLoc, 1);
	else
		glUniform1i(aboveLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);
}

void init(GLFWwindow* window) {
	//创建着色器程序
	renderingProgram_FLOOR = Utils::createShaderProgram("../Shaders/vertShaderFLOOR.glsl", "../Shaders/fragShaderFLOOR.glsl");
	renderingProgram_SURFACE = Utils::createShaderProgram("../Shaders/vertShaderSURFACE.glsl", "../Shaders/fragShaderSURFACE.glsl");
	renderingProgramCubeMap = Utils::createShaderProgram("../Shaders/vertCShader.glsl", "../Shaders/fragCShader.glsl");

	//初始化光照
	lightLoc = glm::vec3(-10.0f, 10.0f, -50.0f);

	//初始化模型顶点
	setupVertices();

	//读取天空盒材质
	skyboxTexture = Utils::loadCubeMap("../Assets/cubeMap/cubeMap1");
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);			//CubMap优化：消除缝隙
	
	//创建反射折射缓冲区
	createReflectRefractBuffers(window);

	//添加噪声
	generateNoise();
	noiseTexture = buildNoiseTexture();
}

void display(GLFWwindow* window, double currentTime) {
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);	//启用默认缓冲区渲染场景

	//清空缓存
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	// 主相机相关
	{

		glfwGetFramebufferSize(window, &width, &height);
		aspect = (float)width / (float)height;
		pMat = glm::perspective(toRadians(60.0), aspect, 0.1f, 1000.0f);

		//踩坑：最终计算 M = T * R2 * R1 (* Model)，但是glm的顺序是最后的参数是最往前的项（可能是逻辑上最后的参数确实是应该最后计算的）
		vMat = glm::rotate(glm::mat4(1.0f), toRadians(-camRot.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -camPos.y, -camPos.z));
	}

	// 虚拟相机，填充反射、折射缓冲区
	{
		// 反射
		// 如果相机在水面上，将反射场景渲染给反射缓冲区
		if (camPos.y > surfacePlaneHeight) {
			vMat = glm::rotate(glm::mat4(1.0f), toRadians(-(-camRot.x)), glm::vec3(1.0f, 0.0f, 0.0f))							//旋转上，关于平面对称(仅pitch有关联)
				* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -(2*surfacePlaneHeight - camPos.y), -camPos.z));			//位置上，相对于水面对称(仅y有关联)

			glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);
			glClear(GL_DEPTH_BUFFER_BIT);
			glClear(GL_COLOR_BUFFER_BIT);
			prepForSkyBoxRender();
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);	// 天空盒是CW（人为规范），但是渲染内部
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glEnable(GL_DEPTH_TEST);
		}

		//折射
		//虚拟相机位置与主相机一致
		vMat = glm::rotate(glm::mat4(1.0f), toRadians(-camRot.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -camPos.y, -camPos.z));

		glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);

		if (camPos.y >= surfacePlaneHeight) {
			prepForFloorRender();
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else {
			prepForSkyBoxRender();
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);	
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glEnable(GL_DEPTH_TEST);
		}
	}

	// 切换回标准缓冲区，准备组装完整场景
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw 天空盒 CubeMap
	{
		prepForSkyBoxRender();	// 经过重构，大部分代码移动到了该函数中，下面的DrawCall同理

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);	// 立方体是顺时针方向（CW），但我们正在查看内部。
		glDisable(GL_DEPTH_TEST);	//需要关闭深度检测，这样保证天空盒的深度值为1.0，永远最后渲染 >_<
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Draw 水面 Plane
	{
		prepForTopSurfaceRender();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, reflectTextureId);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, refractTextureId);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, noiseTexture);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//如果相机高度大于等于水平面，渲染正面，否则渲染反面
		if (camPos.y >= surfacePlaneHeight)
			glFrontFace(GL_CCW);
		else
			glFrontFace(GL_CW);

		glDrawArrays(GL_TRIANGLES, 0, 18);
	}

	// Draw 泳池底部 Plane
	{
		prepForFloorRender();


		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glFrontFace(GL_CCW);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// 每次窗口大小变化时，更新 OpenGL 视口
	glViewport(0, 0, width, height);
}

int main(void) {
	if (!glfwInit()) { exit(EXIT_FAILURE); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(1200, 1200, "WaterSimulator", NULL, NULL);
	glfwMakeContextCurrent(window);

	// Hide cursor and enable raw mouse motion
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//初始化
	lastFrame_time = 0;

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	init(window);

	while (!glfwWindowShouldClose(window)) {

		//用户操控相关
		{
			deltaTime = glfwGetTime() - lastFrame_time;
			lastFrame_time = glfwGetTime();

			//std::cout << deltaTime << std::endl;
			processInput(window, deltaTime);
		}

		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}