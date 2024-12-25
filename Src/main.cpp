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

//�����Զ���Utils����Ҫ����ȡshader�����ӵ�program����ȡ����...�Ĳ���
Utils util = Utils();

//������Ⱦ����vao��vbo
GLuint renderingProgram_SURFACE;		//ˮ��
GLuint renderingProgram_FLOOR;			//Ӿ�صײ�
GLuint renderingProgramCubeMap;			//��պ�
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

// UniformLocation�Ļ���
GLuint vLoc, mvLoc, projLoc, nLoc;

//TODO: ʹ����ת�þ������ߵ�ԭ�����ڣ������зǾ�������ʱ���򵥵�ת����ƽ�Ʋ��������ܱ�֤������Ȼָ�����⡣ʹ����ת�þ������ȷ�������ھ����任��ķ�����ȷ(���˲�δ�����ѧԭ��)��
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;			//mpv�任����

GLuint skyboxTexture;									//��պв���

//������
int width, height;										//����window��width,height����
float aspect;											//��Ұ�����������߱�����
float mouseSensitivity = .5f;							//�û���������ӽ�ת����������
float moveSpeed = 5.0f;									//�û��ƶ��ٶ�
//�������λ�ú���ת
glm::vec3 camPos = glm::vec3(0.0f, 2.0f, 0.0f);			//x, y, z
glm::vec3 camRot = glm::vec3(-15.0f, 0.0f, 0.0f);		//Pitch, Yaw, Roll(0)
//�û��������
float lastFrame_time = 0;								//GLFW��Ȼû�����õ�deltaTime o.0
float deltaTime = 0;
bool firstMouse = true;
float lastX;
float lastY;

//����ģ������λ��
float surfacePlaneHeight = 0.0f;
float floorPlaneHeight = -10.0f;						//Ӿ�صײ��߶�ֵ

#pragma region ����
//�������
//��Դλ��
glm::vec3 lightLoc = glm::vec3(0.0f, 4.0f, -8.0f);
//ȫ�ֻ����⡢�����⡢������⡢���淴��⡢��Դ��ģ�͵Ļ����⡢ģ�͵�������⡢ģ�͵ľ��淴��⡢ģ�͵Ĺ���ȵ�uniform����λ��
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


//TODO
void processInput(GLFWwindow* window, float deltaTime) {
#pragma region ���
	// ��ȡ��ǰ���λ��
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// ����ǵ�һ�λ�ȡ���λ�ã���ʼ��
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// �������ƫ��
	float xoffset = lastX - xpos;
	float yoffset = lastY - ypos; // ���Ƿ����
	lastX = xpos;
	lastY = ypos;

	// ����ƫ����
	xoffset *= mouseSensitivity;
	yoffset *= mouseSensitivity;

	// ���������ת
	camRot.y += xoffset; // Yaw
	camRot.x += yoffset; // Pitch

	// ���� pitch ֵ�����ⷭת
	if (camRot.x > 80.0f) camRot.x = 80.0f;
	if (camRot.x < -80.0f) camRot.x = -80.0f;
#pragma endregion

#pragma region ����
	float cameraSpeedAdjusted = moveSpeed * deltaTime;

	// �������ǰ����
	glm::vec3 forwardDirection = glm::vec3(
		sin(glm::radians(camRot.y)) * cos(glm::radians(camRot.x)),  // X�᷽��
		sin(glm::radians(-camRot.x)),  // Y�᷽�����£�
		cos(glm::radians(camRot.y)) * cos(glm::radians(camRot.x))   // Z�᷽��
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

void setupVertices(void) {
	//��պеĶ�������
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
	//Ӿ�صײ�Plane�Ķ�������
	float PLANE_POSITIONS[18] = {
		-32.0f, 0.0f, -32.0f,  -32.0f, 0.0f, 32.0f,  32.0f, 0.0f, -32.0f,
		32.0f, 0.0f, -32.0f,  -32.0f, 0.0f, 32.0f,  32.0f, 0.0f, 32.0f
	};
	//Ӿ�صײ�Plane��TEXTCOORD��UV��
	float PLANE_TEXCOORDS[12] = {
		0.0f, 0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
		1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f
	};

	//Ϊ����͵��������ӷ�������ȫ��ָ���Ϸ���
	float PLANE_NORMALS[18] = {
		0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, 0.0f
	};

	//��vao��vbo
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
	//������ɫ������
	renderingProgram_FLOOR = Utils::createShaderProgram("../Shaders/vertShaderFLOOR.glsl", "../Shaders/fragShaderFLOOR.glsl");
	renderingProgram_SURFACE = Utils::createShaderProgram("../Shaders/vertShaderSURFACE.glsl", "../Shaders/fragShaderSURFACE.glsl");
	renderingProgramCubeMap = Utils::createShaderProgram("../Shaders/vertCShader.glsl", "../Shaders/fragCShader.glsl");

	//��ʼ������
	lightLoc = glm::vec3(-10.0f, 10.0f, -50.0f);

	//��ʼ��ģ�Ͷ���
	setupVertices();

	//��ȡ��պв���
	skyboxTexture = Utils::loadCubeMap("../Assets/cubeMap");
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);			//CubMap�Ż���������϶
}

void display(GLFWwindow* window, double currentTime) {
	
	//��ջ���
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	

	//������
	{

		glfwGetFramebufferSize(window, &width, &height);
		aspect = (float)width / (float)height;
		pMat = glm::perspective(toRadians(60.0), aspect, 0.1f, 1000.0f);

		//�ȿӣ����ռ��� M = T * R2 * R1 (* Model)������glm��˳�������Ĳ���������ǰ����������߼������Ĳ���ȷʵ��Ӧ��������ģ�
		vMat = glm::rotate(glm::mat4(1.0f), toRadians(-camRot.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -camPos.y, -camPos.z));
	}
	

	// Draw ��պ� CubeMap
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
		glFrontFace(GL_CCW);	// ��������˳ʱ�뷽��CW�������������ڲ鿴�ڲ���
		glDisable(GL_DEPTH_TEST);	//��Ҫ�ر���ȼ�⣬������֤��պе����ֵΪ1.0����Զ�����Ⱦ >_<
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Draw ˮ�� Plane
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

		//�������߶ȴ��ڵ���ˮƽ�棬��Ⱦ���棬������Ⱦ����
		if (camPos.y >= surfacePlaneHeight)
			glFrontFace(GL_CCW);
		else
			glFrontFace(GL_CW);

		glDrawArrays(GL_TRIANGLES, 0, 18);
	}

	// Draw Ӿ�صײ� Plane
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// ÿ�δ��ڴ�С�仯ʱ������ OpenGL �ӿ�
	glViewport(0, 0, width, height);
}

int main(void) {
	if (!glfwInit()) { exit(EXIT_FAILURE); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(600, 600, "Program 15.1 - surface setup", NULL, NULL);
	glfwMakeContextCurrent(window);

	// Hide cursor and enable raw mouse motion
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//��ʼ��
	lastFrame_time = 0;

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	init(window);

	while (!glfwWindowShouldClose(window)) {

		//�û��ٿ����
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