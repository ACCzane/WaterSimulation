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
#include <stack>

#pragma region Setup
#define PI 3.1415926535

using namespace std;

#define numVAOs 10

//0:CubeMapVPos
//1:PlaneVPos
//2:PlaneVNormal
//3:PlaneVTc
#define numVBOs_PoolPlane 4
#define numVBOs_SpaceShip 3
#define numVBOs_Whale 3

GLuint vao[numVAOs];
GLuint vbo_poolPlane[numVBOs_PoolPlane];
GLuint vbo_spaceShip[numVBOs_SpaceShip];
GLuint vbo_whale[numVBOs_Whale];
GLuint vbo_sun[3];
GLuint vbo_earth[3];
GLuint vbo_moon[3];

//�ⲿ����ģ�͵���Ϣ
long long spaceshipModelVerticesNum = 0;
long long whaleModelVerticesNum = 0;
long long sunModelVerticesNum = 0;
long long earthModelVerticesNum = 0;
long long moonModelVerticesNum = 0;

float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }
#pragma endregion

#pragma region Params
//�����Զ���Utils����Ҫ����ȡshader�����ӵ�program����ȡ����...�Ĳ���
Utils util = Utils();

//������Ⱦ����
GLuint renderingProgram_SURFACE;		//ˮ��
GLuint renderingProgram_FLOOR;			//Ӿ�صײ�
GLuint renderingProgramCubeMap;			//��պ�
GLuint renderingProgramAboveSurfaceObj;	//ˮ���Ϸ�����
GLuint renderingProgramBelowSurfaceObj;	//ˮ���·�����


// UniformLocation�Ļ���
GLuint vLoc, mvLoc, projLoc, nLoc;

// ���ڸ�ˮ����ӷ��������
GLuint refractTextureId;
GLuint reflectTextureId;
GLuint refractFrameBuffer;		//���ڴ洢������Ϣ���Զ���֡������
GLuint reflectFrameBuffer;		//���ڴ洢������Ϣ���Զ���֡������

//TODO: ʹ����ת�þ������ߵ�ԭ�����ڣ������зǾ�������ʱ���򵥵�ת����ƽ�Ʋ��������ܱ�֤������Ȼָ�����⡣ʹ����ת�þ������ȷ�������ھ����任��ķ�����ȷ(���˲�δ�����ѧԭ��)��
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;			//mpv�任����

GLuint skyboxTexture;									//��պв���

//������
int width, height;										//����window��width,height����
float aspect;											//��Ұ�����������߱�����
float mouseSensitivity = 10.0f;							//�û���������ӽ�ת����������
float moveSpeed = 5.0f;									//�û��ƶ��ٶ�
//�������λ�ú���ת
glm::vec3 camPos = glm::vec3(0.0f, 10.0f, 0.0f);			//x, y, z
glm::vec3 camRot = glm::vec3(-15.0f, 0.0f, 0.0f);		//Pitch, Yaw, Roll(0)
//�û��������
float lastFrame_time = 0;								//GLFW��Ȼû�����õ�deltaTime o.0
float deltaTime = 0;
bool firstMouse = true;
float lastX;
float lastY;

//����ģ������λ��
float surfacePlaneHeight = 0.0f;
float floorPlaneHeight = -20.0f;						//Ӿ�صײ��߶�ֵ
glm::vec3 sunPos = glm::vec3(0.0f, 20.0f, -20.0f);		//����λ��
glm::vec3 sunScale = glm::vec3(0.01f, 0.01f, 0.01f);	
glm::vec3 earthPos = glm::vec3(10.0f, 0.0f, 0.0f);		//���̫��λ��
glm::vec3 moonPos = glm::vec3(8.0f, 0.0f, 0.0f);		//��Ե���λ��

std::stack<glm::mat4> mStack;

glm::mat4 varyingSunMMatrix;
glm::mat4 varyingEarthMMatrix;
glm::mat4 varyingMoonMMatrix;

#pragma region ����
//�������
//��Դλ��
glm::vec3 lightLoc = glm::vec3(0.0f, 4.0f, -8.0f);
//ȫ�ֻ����⡢�����⡢������⡢���淴��⡢��Դ��ģ�͵Ļ����⡢ģ�͵�������⡢ģ�͵ľ��淴��⡢ģ�͵Ĺ���ȡ��Ƿ�������Ϸ���uniform����λ��
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mambLoc, mdiffLoc, mspecLoc, mshiLoc, aboveLoc;
glm::vec3 currentLightPos, transformed;		//��ͼת����Ĺ�Դ
float lightPos[3];

// ���Դ
float globalAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// ��������
float matAmb[4] = { 0.5f, 0.6f, 0.8f, 1.0f };
float matDif[4] = { 0.8f, 0.9f, 1.0f, 1.0f };
float matSpe[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float matShi = 250.0f;
#pragma endregion

#pragma region ����

GLuint noiseTexture;									//��������

const int noiseHeight = 256;
const int noiseWidth = 256;
const int noiseDepth = 256;

double noise[noiseHeight][noiseWidth][noiseDepth];
#pragma endregion

#pragma region ����
float depthLookup = 0.0f;
GLuint dOffsetLoc;
#pragma endregion

#pragma region ����

GLuint spaceShipMainTex;
GLuint whaleMainTex;
GLuint sunMainTex;
GLuint earthMainTex;
GLuint moonMainTex;

#pragma endregion

#pragma endregion

#pragma region ������غ���

//ƽ��Noise
double smoothNoise(double x1, double y1, double z1, double zoom) {
	//ȡС�����֣���ʾ�ٷֱȣ�������ֵ��
	double fractX = x1 - (int)x1;
	double fractY = y1 - (int)y1;
	double fractZ = z1 - (int)z1;

	//����ֵ�������߽�����¿���round����һ�߽�
	double x2 = x1 - 1; if (x2 < 0) x2 = (round(noiseHeight / zoom)) - 1;
	double y2 = y1 - 1; if (y2 < 0) y2 = (round(noiseWidth / zoom)) - 1;
	double z2 = z1 - 1; if (z2 < 0) z2 = (round(noiseDepth / zoom)) - 1;

	//�����Բ�ֵ
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

	sum = (sin((1.0 / 512.0) * (8 * PI) * (x + z- 4*y)) + 1) * 8.0;				//�����Ҳ���������ͼ

	while (zoom >= 0.9) {
		sum = sum + smoothNoise(x / zoom, y / zoom, z / zoom, zoom) * zoom;
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

	//�Զ�����������
	fillDataArray(data);

	//�̶�������3D�����д��
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
				noise[x][y][z] = (double)rand() / (RAND_MAX + 1.0);			//[0, 1)��Doubleֵ
			}
		}
	}
}
#pragma endregion

#pragma region ������غ���
void installLights(glm::mat4 vMatrix, GLuint renderingProgram) {
	transformed = glm::vec3(vMatrix * glm::vec4(currentLightPos, 1.0));
	lightPos[0] = transformed.x;
	lightPos[1] = transformed.y;
	lightPos[2] = transformed.z;

	// ȡLoc
	globalAmbLoc = glGetUniformLocation(renderingProgram, "globalAmbient");
	ambLoc = glGetUniformLocation(renderingProgram, "light.ambient");
	diffLoc = glGetUniformLocation(renderingProgram, "light.diffuse");
	specLoc = glGetUniformLocation(renderingProgram, "light.specular");
	posLoc = glGetUniformLocation(renderingProgram, "light.position");
	mambLoc = glGetUniformLocation(renderingProgram, "material.ambient");
	mdiffLoc = glGetUniformLocation(renderingProgram, "material.diffuse");
	mspecLoc = glGetUniformLocation(renderingProgram, "material.specular");
	mshiLoc = glGetUniformLocation(renderingProgram, "material.shininess");

	//  ����Shader���й�light��material��uniform����
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

#pragma region �û�������غ���
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
	xoffset *= mouseSensitivity /40.0f;
	yoffset *= mouseSensitivity /40.0f;

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

	// ��ȡShift��״̬
	bool isShiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

	// �����סShift������������ƶ��ٶ�
	if (isShiftPressed) {
		cameraSpeedAdjusted *= 2.0f; // ���谴��Shift���ٶȼӱ�
	}

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

	// ���Shift��û�а��£��ָ�ԭ�����ٶ�
	if (!isShiftPressed) {
		cameraSpeedAdjusted /= 2.0f;
	}


	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		mouseSensitivity -= 1;
		if (mouseSensitivity <= 0) {
			mouseSensitivity = 1;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		mouseSensitivity += 1;
#pragma endregion
}
#pragma endregion

#pragma region Buffer�󶨺���
void setupVertices(void) {

	// ����VAO
	glGenVertexArrays(numVAOs, vao);
	
	// ��VAO
	glBindVertexArray(vao[0]);
#pragma region �ڲ�ģ�ͼ���
	std::vector<glm::vec3> cubMapVertices;
	std::vector<glm::vec3> planeVertices;
	std::vector<glm::vec3> planeNormals;
	std::vector<glm::vec2> planeTexCoords;

	// ���� Utils::SetupPoolVertices ����� std::vector ����
	Utils::SetupPoolVertices(cubMapVertices, planeVertices, planeNormals, planeTexCoords);

	glGenBuffers(numVBOs_PoolPlane, vbo_poolPlane);

	// ��պж���λ��
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[0]);
	glBufferData(GL_ARRAY_BUFFER, cubMapVertices.size() * sizeof(glm::vec3), cubMapVertices.data(), GL_STATIC_DRAW);

	// ƽ�涥��λ��
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[1]);
	glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(glm::vec3), planeVertices.data(), GL_STATIC_DRAW);

	// ƽ�涥�� UV ����
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[2]);
	glBufferData(GL_ARRAY_BUFFER, planeTexCoords.size() * sizeof(glm::vec2), planeTexCoords.data(), GL_STATIC_DRAW);

	// ƽ�涥�㷨����
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[3]);
	glBufferData(GL_ARRAY_BUFFER, planeNormals.size() * sizeof(glm::vec3), planeNormals.data(), GL_STATIC_DRAW);
#pragma endregion


#pragma region �ⲿģ�ͼ���
	//������
	{
		glBindVertexArray(vao[1]);

		//std::vector<glm::vec3> cubMapVertices;
		std::vector<glm::vec3> spaceShipVertices;
		std::vector<glm::vec3> spaceShipNormals;
		std::vector<glm::vec2> spaceShipTexCoords;

		// ���� Utils::SetupPoolVertices ����� std::vector ����
		Utils::LoadObj("../Assets/spaceShip/Intergalactic_Spaceship-(Wavefront).obj", spaceShipVertices, spaceShipNormals, spaceShipTexCoords);

		std::cout << "Spaceship loaded: " << spaceShipVertices.size() << "vertices" << std::endl;

		glGenBuffers(numVBOs_SpaceShip, vbo_spaceShip);

		spaceshipModelVerticesNum = spaceShipVertices.size();

		// ƽ�涥��λ��
		glBindBuffer(GL_ARRAY_BUFFER, vbo_spaceShip[0]);
		glBufferData(GL_ARRAY_BUFFER, spaceShipVertices.size() * sizeof(glm::vec3), spaceShipVertices.data(), GL_STATIC_DRAW);

		// ƽ�涥�� UV ����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_spaceShip[1]);
		glBufferData(GL_ARRAY_BUFFER, spaceShipTexCoords.size() * sizeof(glm::vec2), spaceShipTexCoords.data(), GL_STATIC_DRAW);

		// ƽ�涥�㷨����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_spaceShip[2]);
		glBufferData(GL_ARRAY_BUFFER, spaceShipNormals.size() * sizeof(glm::vec3), spaceShipNormals.data(), GL_STATIC_DRAW);
	}
	//����
	{
		glBindVertexArray(vao[2]);

		//std::vector<glm::vec3> cubMapVertices;
		std::vector<glm::vec3> whaleVertices;
		std::vector<glm::vec3> whaleNormals;
		std::vector<glm::vec2> whaleTexCoords;

		// ���� Utils::SetupPoolVertices ����� std::vector ����
		//Utils::LoadObj("../Assets/fish/12265_Fish_v1_L2.obj", whaleVertices, whaleNormals, whaleTexCoords);
		Utils::LoadObj("../Assets/fish/fish.obj", whaleVertices, whaleNormals, whaleTexCoords);

		glGenBuffers(numVBOs_SpaceShip, vbo_whale);

		std::cout << "Whale loaded: " << whaleVertices.size() << "vertices" << std::endl;

		whaleModelVerticesNum = whaleVertices.size();

		// ƽ�涥��λ��
		glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[0]);
		glBufferData(GL_ARRAY_BUFFER, whaleVertices.size() * sizeof(glm::vec3), whaleVertices.data(), GL_STATIC_DRAW);

		// ƽ�涥�� UV ����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[1]);
		glBufferData(GL_ARRAY_BUFFER, whaleTexCoords.size() * sizeof(glm::vec2), whaleTexCoords.data(), GL_STATIC_DRAW);

		// ƽ�涥�㷨����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[2]);
		glBufferData(GL_ARRAY_BUFFER, whaleNormals.size() * sizeof(glm::vec3), whaleNormals.data(), GL_STATIC_DRAW);
	}
	//̫��
	{
		glBindVertexArray(vao[1]);

		//std::vector<glm::vec3> cubMapVertices;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texcoords;

		// ���� Utils::SetupPoolVertices ����� std::vector ����
		//Utils::LoadObj("../Assets/fish/12265_Fish_v1_L2.obj", whaleVertices, whaleNormals, whaleTexCoords);
		Utils::LoadObj("../Assets/sun/Sun.obj", vertices, normals, texcoords);

		glGenBuffers(3, vbo_sun);

		std::cout << "sun loaded: " << vertices.size() << "vertices" << std::endl;

		sunModelVerticesNum = vertices.size();

		// ƽ�涥��λ��
		glBindBuffer(GL_ARRAY_BUFFER, vbo_sun[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

		// ƽ�涥�� UV ����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_sun[1]);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);

		// ƽ�涥�㷨����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_sun[2]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
	}
	//����
	{
		glBindVertexArray(vao[1]);

		//std::vector<glm::vec3> cubMapVertices;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texcoords;

		// ���� Utils::SetupPoolVertices ����� std::vector ����
		//Utils::LoadObj("../Assets/fish/12265_Fish_v1_L2.obj", whaleVertices, whaleNormals, whaleTexCoords);
		Utils::LoadObj("../Assets/earth/Earth 2K.obj", vertices, normals, texcoords);

		glGenBuffers(3, vbo_earth);

		std::cout << "earth loaded: " << vertices.size() << "vertices" << std::endl;

		earthModelVerticesNum = vertices.size();

		// ƽ�涥��λ��
		glBindBuffer(GL_ARRAY_BUFFER, vbo_earth[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

		// ƽ�涥�� UV ����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_earth[1]);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);

		// ƽ�涥�㷨����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_earth[2]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
	}
	//����
	{
		glBindVertexArray(vao[1]);

		//std::vector<glm::vec3> cubMapVertices;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texcoords;

		// ���� Utils::SetupPoolVertices ����� std::vector ����
		//Utils::LoadObj("../Assets/fish/12265_Fish_v1_L2.obj", whaleVertices, whaleNormals, whaleTexCoords);
		Utils::LoadObj("../Assets/moon/Moon 2K.obj", vertices, normals, texcoords);

		glGenBuffers(3, vbo_moon);

		std::cout << "moon loaded: " << vertices.size() << "vertices" << std::endl;

		moonModelVerticesNum = vertices.size();

		// ƽ�涥��λ��
		glBindBuffer(GL_ARRAY_BUFFER, vbo_moon[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

		// ƽ�涥�� UV ����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_moon[1]);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);

		// ƽ�涥�㷨����
		glBindBuffer(GL_ARRAY_BUFFER, vbo_moon[2]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
	}
#pragma endregion

}

void createReflectRefractBuffers(GLFWwindow* window) {
	/*--------------------------------------------------����-------------------------------------------------------*/
	//���ɲ��󶨻�����
	GLuint bufferId[1];
	glGenBuffers(1, bufferId);
	glfwGetFramebufferSize(window, &width, &height);

	//���ɲ��󶨷����֡������
	glGenFramebuffers(1, bufferId);
	reflectFrameBuffer = bufferId[0];
	glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);

	//���ɲ����÷�������
	glGenTextures(1, bufferId);
	reflectTextureId = bufferId[0];
	glBindTexture(GL_TEXTURE_2D, reflectTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//�������ӵ�֡������
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectTextureId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	//���ɲ������������
	glGenTextures(1, bufferId);
	glBindTexture(GL_TEXTURE_2D, bufferId[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferId[0], 0);

	/*--------------------------------------------------����-------------------------------------------------------*/
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
#pragma endregion

#pragma region ��Ⱦ׼������
void prepForSkyBoxRender() {
	glBindVertexArray(vao[0]);

	glUseProgram(renderingProgramCubeMap);

	vLoc = glGetUniformLocation(renderingProgramCubeMap, "v_matrix");
	projLoc = glGetUniformLocation(renderingProgramCubeMap, "p_matrix");

	glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(vMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	//vbo[0]���ڴ洢��պж���
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
}

void prepForTopSurfaceRender() {
	glBindVertexArray(vao[1]);

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

	dOffsetLoc = glGetUniformLocation(renderingProgram_SURFACE, "depthOffset");			//ʵ�ֶ���
	glUniform1f(dOffsetLoc, depthLookup);

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

	if (camPos.y > surfacePlaneHeight)
		glUniform1i(aboveLoc, 1);
	else
		glUniform1i(aboveLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[2]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[3]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflectTextureId);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, refractTextureId);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
}

void prepForFloorRender() {
	glBindVertexArray(vao[1]);

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

	dOffsetLoc = glGetUniformLocation(renderingProgram_SURFACE, "depthOffset");
	glUniform1f(dOffsetLoc, depthLookup);

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

	if (camPos.y >= surfacePlaneHeight)
		glUniform1i(aboveLoc, 1);
	else
		glUniform1i(aboveLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[2]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_poolPlane[3]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
}

//ˮ���Ϸ����������Ⱦ׼��
void prepForAboveSurfaceObj(GLuint* targetVbo, GLuint targetMainTex, glm::vec3 trans) {
	//�ɴ�
	{
		glBindVertexArray(vao[1]);

		glUseProgram(renderingProgramAboveSurfaceObj);

		mvLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "mv_matrix");
		projLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "proj_matrix");
		nLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "norm_matrix");
		aboveLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "isAbove");

		mMat = glm::translate(glm::mat4(1.0f), trans);
		mvMat = vMat * mMat;
		invTrMat = glm::transpose(glm::inverse(mvMat));

		currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
		installLights(vMat, renderingProgramAboveSurfaceObj);

		dOffsetLoc = glGetUniformLocation(renderingProgram_SURFACE, "depthOffset");
		glUniform1f(dOffsetLoc, depthLookup);

		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

		if (camPos.y >= surfacePlaneHeight)
			glUniform1i(aboveLoc, 1);
		else
			glUniform1i(aboveLoc, 0);

		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[1]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[2]);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, noiseTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, targetMainTex);
	}
}

//ˮ���Ϸ����������Ⱦ׼�����ع�
void prepForAboveSurfaceObj(GLuint* targetVbo, GLuint targetMainTex, glm::mat4 mMatrix) {
	//�ɴ�
	{
		glBindVertexArray(vao[1]);

		glUseProgram(renderingProgramAboveSurfaceObj);

		mvLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "mv_matrix");
		projLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "proj_matrix");
		nLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "norm_matrix");
		aboveLoc = glGetUniformLocation(renderingProgramAboveSurfaceObj, "isAbove");

		mvMat = vMat * mMatrix;
		invTrMat = glm::transpose(glm::inverse(mvMat));

		currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
		installLights(vMat, renderingProgramAboveSurfaceObj);

		dOffsetLoc = glGetUniformLocation(renderingProgram_SURFACE, "depthOffset");
		glUniform1f(dOffsetLoc, depthLookup);

		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

		if (camPos.y >= surfacePlaneHeight)
			glUniform1i(aboveLoc, 1);
		else
			glUniform1i(aboveLoc, 0);

		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[1]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, targetVbo[2]);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, noiseTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, targetMainTex);
	}
}

//ˮ���·�������Ⱦ׼��
void prepForUnderSurfaceObj() {
	glBindVertexArray(vao[2]);

	glUseProgram(renderingProgramBelowSurfaceObj);

	mvLoc = glGetUniformLocation(renderingProgramBelowSurfaceObj, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgramBelowSurfaceObj, "proj_matrix");
	nLoc = glGetUniformLocation(renderingProgramBelowSurfaceObj, "norm_matrix");
	aboveLoc = glGetUniformLocation(renderingProgramBelowSurfaceObj, "isAbove");

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, -8.0f, -10.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));

	mvMat = vMat * mMat;
	invTrMat = glm::transpose(glm::inverse(mvMat));

	currentLightPos = glm::vec3(lightLoc.x, lightLoc.y, lightLoc.z);
	installLights(vMat, renderingProgramBelowSurfaceObj);

	dOffsetLoc = glGetUniformLocation(renderingProgram_SURFACE, "depthOffset");
	glUniform1f(dOffsetLoc, depthLookup);

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));

	if (camPos.y >= surfacePlaneHeight)
		glUniform1i(aboveLoc, 1);
	else
		glUniform1i(aboveLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_whale[2]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, whaleMainTex);
}
#pragma endregion


void init(GLFWwindow* window) {
	//������ɫ������
	renderingProgram_FLOOR = Utils::createShaderProgram("../Shaders/vertShaderFLOOR.glsl", "../Shaders/fragShaderFLOOR.glsl");
	renderingProgram_SURFACE = Utils::createShaderProgram("../Shaders/vertShaderSURFACE.glsl", "../Shaders/fragShaderSURFACE.glsl");
	renderingProgramCubeMap = Utils::createShaderProgram("../Shaders/vertCShader.glsl", "../Shaders/fragCShader.glsl");
	renderingProgramAboveSurfaceObj = Utils::createShaderProgram("../Shaders/vertShaderSPACESHIP.glsl", "../Shaders/fragShaderSPACESHIP.glsl");
	renderingProgramBelowSurfaceObj = Utils::createShaderProgram("../Shaders/vertShaderWHALE.glsl", "../Shaders/fragShaderWHALE.glsl");		//��Floor����һ�¼���

	//��ʼ������
	lightLoc = glm::vec3(-10.0f, 10.0f, -50.0f);

	//��ʼ��ģ�Ͷ���
	setupVertices();

	//��ȡ��պв���
	skyboxTexture = Utils::loadCubeMap("../Assets/cubeMap/cubeMap1");
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);			//CubMap�Ż���������϶
	
	//�����������仺����
	createReflectRefractBuffers(window);

	//�������
	generateNoise();
	noiseTexture = buildNoiseTexture();

	spaceShipMainTex = Utils::loadTexture("../Assets/spaceShip/Intergalactic Spaceship_color_4.jpg");
	whaleMainTex = Utils::loadTexture("../Assets/fish/fish.png");
	sunMainTex = Utils::loadTexture("../Assets/sun/Sun.jpg");
	earthMainTex = Utils::loadTexture("../Assets/earth/Textures/Diffuse_2K.png");
	moonMainTex = Utils::loadTexture("../Assets/moon/Textures/Diffuse_2K.png");
}

void display(GLFWwindow* window, double currentTime) {
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);	//����Ĭ�ϻ�������Ⱦ����

	//��ջ���
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	// ��������
	{

		glfwGetFramebufferSize(window, &width, &height);
		aspect = (float)width / (float)height;
		pMat = glm::perspective(toRadians(60.0), aspect, 0.1f, 1000.0f);

		//�ȿӣ����ռ��� M = T * R2 * R1 (* Model)������glm��˳�������Ĳ���������ǰ����������߼������Ĳ���ȷʵ��Ӧ��������ģ�
		vMat = glm::rotate(glm::mat4(1.0f), toRadians(-camRot.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -camPos.y, -camPos.z));
	}

	//�㼶��ģ��أ���������˶���
	{
		glm::mat4 initialMat = glm::mat4(1.0f);
		//��ģ�;���ѹ��ջ
		mStack.push(initialMat);
		//̫��
		mStack.push(mStack.top());
		mStack.top() *= glm::translate(glm::mat4(1.0f), sunPos);
		mStack.top() *= glm::scale(glm::mat4(1.0f), sunScale);			//Ҫ�ȸı�Scale

		varyingSunMMatrix = mStack.top();
		mStack.pop();													//���Ǹı��Scale��Ӧ�ô��ݸ�������
		mStack.push(mStack.top());
		mStack.top() *= glm::translate(glm::mat4(1.0f), sunPos);

		//����
		mStack.push(mStack.top());
		mStack.top() *= glm::translate(glm::mat4(1.0f), 10.0f * glm::vec3(sin((float)currentTime /5.0f) * 15.0f, 0.0f, cos((float)currentTime /5.0f) * 15.0f));
			
		varyingEarthMMatrix = mStack.top();

		//����
		mStack.push(mStack.top());
		mStack.top() *= glm::translate(glm::mat4(1.0f), glm::vec3(sin((float)currentTime * 1.0f) * 10.0f, 0.0f, cos((float)currentTime * 1.0f) * 10.0f));

		varyingMoonMMatrix = mStack.top();
	}

	// �����������䷴�䡢���仺����
	{
		// ����
		// ��������ˮ���ϣ������䳡����Ⱦ�����仺����
		if (camPos.y > surfacePlaneHeight) {
			vMat = glm::rotate(glm::mat4(1.0f), toRadians(-(-camRot.x)), glm::vec3(1.0f, 0.0f, 0.0f))							//��ת�ϣ�����ƽ��Գ�(��pitch�й���)
				* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -(2*surfacePlaneHeight - camPos.y), -camPos.z));			//λ���ϣ������ˮ��Գ�(��y�й���)

			glBindFramebuffer(GL_FRAMEBUFFER, reflectFrameBuffer);
			glClear(GL_DEPTH_BUFFER_BIT);
			glClear(GL_COLOR_BUFFER_BIT);

			//������պ�
			prepForSkyBoxRender();
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);	// ��պ���CW����Ϊ�淶����������Ⱦ�ڲ�
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glEnable(GL_DEPTH_TEST);

			//����SpaceShip
			prepForAboveSurfaceObj(vbo_spaceShip, spaceShipMainTex, glm::vec3(0.0f, 5.0f, -10.0f));
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, spaceshipModelVerticesNum);

			//TODO: ��������
			//����̫��
			prepForAboveSurfaceObj(vbo_sun, sunMainTex, varyingSunMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, sunModelVerticesNum);

			//�������
			prepForAboveSurfaceObj(vbo_earth, earthMainTex, varyingEarthMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, earthModelVerticesNum);

			//��������
			prepForAboveSurfaceObj(vbo_moon, moonMainTex, varyingMoonMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, moonModelVerticesNum);
		}

		//����
		//�������λ���������һ��
		vMat = glm::rotate(glm::mat4(1.0f), toRadians(-camRot.x), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), toRadians(-camRot.y), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(-camPos.x, -camPos.y, -camPos.z));

		glBindFramebuffer(GL_FRAMEBUFFER, refractFrameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);

		if (camPos.y >= surfacePlaneHeight) {
			//�����ש
			prepForFloorRender();
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			//���侨��
			prepForUnderSurfaceObj();
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, whaleModelVerticesNum);
		}
		else {
			//������պ�
			prepForSkyBoxRender();
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);	
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glEnable(GL_DEPTH_TEST);

			//����SpaceShip
			prepForAboveSurfaceObj(vbo_spaceShip, spaceShipMainTex, glm::vec3(0.0f, 5.0f, -10.0f));
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, spaceshipModelVerticesNum);

			//����̫��
			prepForAboveSurfaceObj(vbo_sun, sunMainTex, varyingSunMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, sunModelVerticesNum);

			//�������
			prepForAboveSurfaceObj(vbo_earth, earthMainTex, varyingEarthMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, earthModelVerticesNum);

			//��������
			prepForAboveSurfaceObj(vbo_moon, moonMainTex, varyingMoonMMatrix);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, moonModelVerticesNum);

			//TODO: �������ˮ�²�������ˮ�ϲ��ֵ�����
			prepForUnderSurfaceObj();
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawArrays(GL_TRIANGLES, 0, whaleModelVerticesNum);
		}
	}

	// �л��ر�׼��������׼����װ��������
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	depthLookup += deltaTime * 0.05f;

	glBindVertexArray(vao[0]);

	// Draw ��պ� CubeMap
	{
		prepForSkyBoxRender();	// �����ع����󲿷ִ����ƶ����˸ú����У������DrawCallͬ��

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);	// ��������˳ʱ�뷽��CW�������������ڲ鿴�ڲ���
		glDisable(GL_DEPTH_TEST);	//��Ҫ�ر���ȼ�⣬������֤��պе����ֵΪ1.0����Զ�����Ⱦ >_<
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Draw ˮ�� Plane
	{
		prepForTopSurfaceRender();

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
		prepForFloorRender();


		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glFrontFace(GL_CCW);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	
	glBindVertexArray(vao[1]);
	// Draw SpaceShip
	{
		prepForAboveSurfaceObj(vbo_spaceShip, spaceShipMainTex, glm::vec3(0.0f, 5.0f, -10.0f));

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glDrawArrays(GL_TRIANGLES, 0, spaceshipModelVerticesNum);
	}
	// Draw ����
	{
		prepForUnderSurfaceObj();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glDrawArrays(GL_TRIANGLES, 0, whaleModelVerticesNum);
	}
	// Draw̫��
	{
		prepForAboveSurfaceObj(vbo_sun, sunMainTex, varyingSunMMatrix);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDrawArrays(GL_TRIANGLES, 0, sunModelVerticesNum);
	}
	// Draw ����
	{
		prepForAboveSurfaceObj(vbo_earth, earthMainTex, varyingEarthMMatrix);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDrawArrays(GL_TRIANGLES, 0, earthModelVerticesNum);
	}
	// Draw ����
	{
		prepForAboveSurfaceObj(vbo_moon, moonMainTex, varyingMoonMMatrix);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDrawArrays(GL_TRIANGLES, 0, moonModelVerticesNum);
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
	GLFWwindow* window = glfwCreateWindow(1200, 1200, "WaterSimulator", NULL, NULL);
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