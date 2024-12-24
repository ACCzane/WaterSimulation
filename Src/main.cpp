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
using namespace std;

#define numVAOs 1
#define numVBOs 4

float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }

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

//相机相关
int width, height;										//根据window的width,height设置
float aspect;											//视角，根据相机宽高比设置
//TODO: 使用逆转置矩阵处理法线的原因在于，当进行非均匀缩放时，简单的转动或平移操作并不能保证法线仍然指向面外。使用逆转置矩阵可以确保法线在经过变换后的方向正确(本人并未深究其数学原理)。
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;			//mpv变换矩阵


GLuint skyboxTexture;									//天空盒材质

//定义模型世界位置
float cameraHeight = 2.0f, cameraPitch = 15.0f;			//相机位置和朝向
float surfacePlaneHeight = 0.0f;
float floorPlaneHeight = -10.0f;						//泳池底部高度值

#pragma region 光照
//定义光照
//光源位置
glm::vec3 lightLoc = glm::vec3(0.0f, 4.0f, -8.0f);
//全局环境光、环境光、漫反射光、镜面反射光、光源、模型的环境光、模型的漫反射光、模型的镜面反射光、模型的光泽度的uniform变量位置
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mambLoc, mdiffLoc, mspecLoc, mshiLoc;
glm::vec3 currentLightPos, transformed;
float lightPos[3];

// white light
float globalAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// gold material
float matAmb[4] = { 0.5f, 0.6f, 0.8f, 1.0f };
float matDif[4] = { 0.8f, 0.9f, 1.0f, 1.0f };
float matSpe[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float matShi = 250.0f;

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



void setupVertices(void) {
	//天空盒的顶点坐标
	float cubeVertexPositions[108] ={ 
		-1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, -1.0f,  1.0f, 1.0f,  1.0f, -1.0f,
		1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f, -1.0f,
		1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f
	};
	//泳池底部Plane的顶点坐标
	float PLANE_POSITIONS[18] = {
		-128.0f, 0.0f, -128.0f,  -128.0f, 0.0f, 128.0f,  128.0f, 0.0f, -128.0f,
		128.0f, 0.0f, -128.0f,  -128.0f, 0.0f, 128.0f,  128.0f, 0.0f, 128.0f
	};
	//泳池底部Plane的TEXTCOORD（UV）
	float PLANE_TEXCOORDS[12] = {
		0.0f, 0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
		1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f
	};

	//为顶面和底面光照添加法向量（全部指向上方）
	float PLANE_NORMALS[18] = {
		0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,	1.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f
	};

	//绑定vao、vbo
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexPositions), cubeVertexPositions, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_POSITIONS), PLANE_POSITIONS, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_TEXCOORDS), PLANE_TEXCOORDS, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PLANE_NORMALS), PLANE_NORMALS, GL_STATIC_DRAW);
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
	skyboxTexture = Utils::loadCubeMap("../Assets/cubeMap");
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);			//CubMap优化：消除缝隙
}

void display(GLFWwindow* window, double currentTime) {
	
	//清空缓存
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	//相机相关
	{
		glfwGetFramebufferSize(window, &width, &height);
		aspect = (float)width / (float)height;
		pMat = glm::perspective(toRadians(60.0), aspect, 0.1f, 1000.0f);

		vMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -cameraHeight, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(cameraPitch), glm::vec3(1.0f, 0.0f, 0.0f));
	}
	

	// Draw 天空盒 CubeMap
	{
		glUseProgram(renderingProgramCubeMap);

		vLoc = glGetUniformLocation(renderingProgramCubeMap, "v_matrix");
		projLoc = glGetUniformLocation(renderingProgramCubeMap, "p_matrix");

		glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(vMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);	// 立方体是顺时针方向（CW），但我们正在查看内部。
		glDisable(GL_DEPTH_TEST);	//需要关闭深度检测，这样保证天空盒的深度值为1.0，永远最后渲染 >_<
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Draw 水面 Plane
	{
		glUseProgram(renderingProgram_SURFACE);

		mvLoc = glGetUniformLocation(renderingProgram_SURFACE, "mv_matrix");
		projLoc = glGetUniformLocation(renderingProgram_SURFACE, "proj_matrix");
		nLoc = glGetUniformLocation(renderingProgram_SURFACE, "norm_matrix");

		mMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, surfacePlaneHeight, 0.0f));
		mvMat = vMat * mMat;
		invTrMat = glm::transpose(glm::inverse(mvMat));

		currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
		installLights(vMat, renderingProgram_SURFACE);

		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(2);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//如果相机高度大于等于水平面，渲染正面，否则渲染反面
		if (cameraHeight >= surfacePlaneHeight)
			glFrontFace(GL_CCW);
		else
			glFrontFace(GL_CW);

		glDrawArrays(GL_TRIANGLES, 0, 18);
	}

	// Draw 泳池底部 Plane
	{
		glUseProgram(renderingProgram_FLOOR);

		mvLoc = glGetUniformLocation(renderingProgram_FLOOR, "mv_matrix");
		projLoc = glGetUniformLocation(renderingProgram_FLOOR, "proj_matrix");
		nLoc = glGetUniformLocation(renderingProgram_FLOOR, "norm_matrix");

		mMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, floorPlaneHeight, 0.0f));
		mvMat = vMat * mMat;
		invTrMat = glm::transpose(glm::inverse(mvMat));

		currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
		installLights(vMat, renderingProgram_FLOOR);

		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);


		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glFrontFace(GL_CCW);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	
}

int main(void) {
	if (!glfwInit()) { exit(EXIT_FAILURE); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(600, 600, "Program 15.1 - surface setup", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	init(window);

	while (!glfwWindowShouldClose(window)) {
		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}