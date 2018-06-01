#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define OGLLOGSIZE 512

#include "GetWallTime.h"

#define NPARTICLES 2500000
#define NPARAMETERS 10
#define ROTATIONDELTA 0.01f
#define MOVEMENTDELTA 0.01f
#define MOUSESENSITIVITY 0.005f
#define CUBESIZE 1.0f

const char *vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 pos;\n"
	"out vec3 posNew;\n"
	"out vec4 colour;\n"
	""
	"uniform float scaleFactor;\n"
	"uniform mat4 rotationMatrix;\n"
	"uniform mat4 translationMatrix;\n"
	"uniform mat4 cameraMatrix;\n"
	"uniform mat4 perspectiveMatrix;\n"
	""
	"uniform float X[10];\n"
	"uniform float Y[10];\n"
	"uniform float Z[10];\n"
	""
	"uniform float stepSize;\n"
	"uniform int updatesPerFrame;\n"
	""
	"void main()\n"
	"{\n"
	"	float velx;\n"
	"	float vely;\n"
	"	float velz;\n"
	"	float x = pos.x;\n"
	"	float y = pos.y;\n"
	"	float z = pos.z;\n"
	"	int i;\n"
	""
	"	for(i = 0; i < updatesPerFrame; i++) {\n"
	"		velx = X[0] + X[1]*x + X[2]*y + X[3]*z + X[4]*x*x + X[5]*x*y + X[6]*x*z + X[7]*y*y + X[8]*y*z + X[9]*z*z;\n"
	"		vely = Y[0] + Y[1]*x + Y[2]*y + Y[3]*z + Y[4]*x*x + Y[5]*x*y + Y[6]*x*z + Y[7]*y*y + Y[8]*y*z + Y[9]*z*z;\n"
	"		velz = Z[0] + Z[1]*x + Z[2]*y + Z[3]*z + Z[4]*x*x + Z[5]*x*y + Z[6]*x*z + Z[7]*y*y + Z[8]*y*z + Z[9]*z*z;\n"
	"		x += stepSize*velx;\n"
	"		y += stepSize*vely;\n"
	"		z += stepSize*velz;\n"
	"	};\n"
	""
	"	posNew.x = x;\n"
	"	posNew.y = y;\n"
	"	posNew.z = z;\n"
	""
	"	float speed = length(vec3(velx,vely,velz));\n"
	""
	"	gl_Position = cameraMatrix * translationMatrix * rotationMatrix * vec4(posNew/scaleFactor, 1.0);\n"
	"	float cameraDistance = -gl_Position.z;\n"
	"	gl_Position = perspectiveMatrix * gl_Position;\n"
	"	gl_PointSize = 4.0f/(1.0f+cameraDistance);\n"
	"	colour = vec4(\n"
	"		+ vec3(40.0f/255.0f, 0.0f, 100.0f/255.0f)\n"
	"		+ 100.0/speed * vec3(225.0f/255.0f, 100.0f/255.0f, 0.0f)\n"
	"		, 0.05f/(1.0f+cameraDistance));\n"
	"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec4 colour;\n"
	"void main()\n"
	"{\n"
	"   FragColor = colour;\n"
	"}\0";

const char *vertexShaderCubeSource = "#version 330 core\n"
	"layout (location = 0) in vec3 pos;\n"
	"uniform mat4 cameraMatrix;\n"
	"uniform mat4 perspectiveMatrix;\n"
	"out vec4 colour;\n"
	""
	"void main()\n"
	"{\n"
	"	gl_Position =  cameraMatrix * vec4(pos.x, pos.y, pos.z, 1.0);\n"
	"	float cameraDistance = -gl_Position.z;\n"
	" 	gl_Position = perspectiveMatrix * gl_Position;\n"
	"	colour = vec4(1.0f, 1.0f, 1.0f, 1.0f/(1.0f+cameraDistance/1.0f));\n"
	"}\0";

const char *fragmentShaderCubeSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec4 colour;\n"
	""
	"void main()\n"
	"{\n"
	"	FragColor = colour;\n"
	"}\0";



// Struct to hold opengl objects
typedef struct {
	GLFWwindow *window;
	unsigned int vertexShader, fragmentShader, shaderProgram;
	unsigned int VAO, pos1VBO, pos2VBO;
	unsigned int vertexShaderCube, fragmentShaderCube, shaderProgramCube;
	unsigned int cubeVAO, cubeVBO;

	// uniforms:
	unsigned int scaleFactorLocation;
	unsigned int rotationMatrixLocation;
	unsigned int translationMatrixLocation;
	unsigned int cameraMatrixLocation;
	unsigned int perspectiveMatrixLocation;
	// attractor parameters
	unsigned int XLocation;
	unsigned int YLocation;
	unsigned int ZLocation;
	// for integration
	unsigned int stepSizeLocation;
	unsigned int updatesPerFrameLocation;
	// for cube
	unsigned int cameraMatrixCubeLocation;
	unsigned int perspectiveMatrixCubeLocation;
} openglObjects;


int setupOpenGL(openglObjects *oglo, const unsigned int xres, const unsigned int yres);
void updateGLData(unsigned int *dstVBO, float *src, unsigned int size);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mousePointerCallback(GLFWwindow* window, double xpos, double ypos);
// global values required in the callback
float pitch = M_PI; //radians
float yaw = -M_PI/2.0f; //radians
double prevX = 0.0;
double prevY = 0.0;
unsigned int updateTransformationUniformsRequired = 0;

void initializeParticlePositions(float* pos, const float volSize);
void setAttractorParameters(openglObjects *oglo, unsigned int attractor);
void prepareCubeVertices(openglObjects *oglo);
void updateTransformationUniforms(openglObjects *oglo, float theta, float phi, unsigned int xres, unsigned int yres, glm::vec3 cameraPosition);


int main(void)
{
	printf("attractors\n");

	printf("Controls:\n"
		"   wasd --- move camera\n"
		"   mouse -- aim camera\n"
		"   r ------ reset particle positions\n"
		"   p,o ---- pause,resume evolution\n"
		"   l ------ manually advance evolution\n"
		"   z,x ---- scale attractor smaller,larger\n"
		"   arrows - rotate attractor\n"
		"   1 ------ Lorenz attractor\n"
		"   2 ------ Roessler attractor\n"
		"   3 ------ Lu Chen attractor\n"
	);

	const int xres = 1900;
	const int yres = 1180;
	prevX = xres/2.0f;
	prevY = yres/2.0f;
	openglObjects oglo;
	if (setupOpenGL(&oglo, xres, yres)) {
		printf("Error in setupOpenGL.\n");
		return EXIT_FAILURE;
	}


	// allocate and initialise point position array
	float *pos = (float*)malloc(NPARTICLES * 3 * sizeof(float));
	initializeParticlePositions(pos, 40.0f);
	glUseProgram(oglo.shaderProgram);
	updateGLData(&(oglo.pos1VBO), pos, 3*NPARTICLES);


	// shader uniforms
	// to bring attractor within viewable volume
	float scaleFactor = 40.0f;
	glUniform1f(oglo.scaleFactorLocation, scaleFactor);

	// choose default attractor
	setAttractorParameters(&oglo, 1);

	// for integration
	float stepSize = 0.001f;
	glUniform1f(oglo.stepSizeLocation, stepSize);
	int updatesPerFrame = 10;
	glUniform1i(oglo.updatesPerFrameLocation, updatesPerFrame);

	// for cube
	prepareCubeVertices(&oglo);

	// rotation, camera and perspective projection. Uniform for both shaders
	float theta = 0.0f; //radians
	float phi = 0.0f; //radians
	glm::vec3 cameraPosition = glm::vec3(0.0f,0.0f,-2.0f);
	glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);

	// translation to move points relative to cube -- try to centre the attractors
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f));
	glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, glm::value_ptr(translationMatrix));


	// Start event loop
	double startTime = GetWallTime();
	unsigned int totalFrames = 0;
	unsigned int updateAttractorOnce = 0;

	while(!glfwWindowShouldClose(oglo.window)) {

		// User control
		if(glfwGetKey(oglo.window, GLFW_KEY_Z) == GLFW_PRESS) {
			scaleFactor *= 1.1f;
			glUniform1f(oglo.scaleFactorLocation, scaleFactor);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_X) == GLFW_PRESS) {
			scaleFactor /= 1.1f;
			glUniform1f(oglo.scaleFactorLocation, scaleFactor);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_R) == GLFW_PRESS) {
			initializeParticlePositions(pos, 40.0f);
			updateGLData(&(oglo.pos1VBO), pos, 3*NPARTICLES);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_P) == GLFW_PRESS) {
			glUniform1f(oglo.stepSizeLocation, 0.0f);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_O) == GLFW_PRESS) {
			glUniform1f(oglo.stepSizeLocation, stepSize);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_L) == GLFW_PRESS) {
			glUniform1f(oglo.stepSizeLocation, stepSize);
			updateAttractorOnce = 1;
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_1) == GLFW_PRESS) {
			setAttractorParameters(&oglo, 1);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_2) == GLFW_PRESS) {
			setAttractorParameters(&oglo, 2);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_3) == GLFW_PRESS) {
			setAttractorParameters(&oglo, 3);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_W) == GLFW_PRESS) {
			cameraPosition += MOVEMENTDELTA * glm::vec3(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw));
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_S) == GLFW_PRESS) {
			cameraPosition -= MOVEMENTDELTA * glm::vec3(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw));
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_A) == GLFW_PRESS) {
			cameraPosition += MOVEMENTDELTA * glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw)));
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_D) == GLFW_PRESS) {
			cameraPosition -= MOVEMENTDELTA * glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw)));
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_UP) == GLFW_PRESS) {
			theta -= ROTATIONDELTA;
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			theta += ROTATIONDELTA;
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			phi += ROTATIONDELTA;
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			phi -= ROTATIONDELTA;
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(oglo.window, 1);
		}

		// this update is triggered by the cursor movement callback
		if(updateTransformationUniformsRequired) {
			updateTransformationUniforms(&oglo, theta, phi, xres, yres, cameraPosition);
			updateTransformationUniformsRequired = 0;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw cube
		glUseProgram(oglo.shaderProgramCube);
		glBindBuffer(GL_ARRAY_BUFFER, oglo.cubeVBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_LINES, 0, 24);

		// draw particles
		glUseProgram(oglo.shaderProgram);
		glBindBuffer(GL_ARRAY_BUFFER, oglo.pos1VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, oglo.pos2VBO);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, NPARTICLES);
		glEndTransformFeedback();

		// swap buffers 1 and 2: output becomes input
		unsigned int tmp = oglo.pos1VBO;
		oglo.pos1VBO = oglo.pos2VBO;
		oglo.pos2VBO = tmp;

		glfwSwapBuffers(oglo.window);
		glfwPollEvents();
		totalFrames++;

		// if manually advancing, set stepSize to zero to halt evolution
		if(updateAttractorOnce) {
			glUniform1f(oglo.stepSizeLocation, 0.0f);
			updateAttractorOnce = 0;
		}
	}
	printf("fps: %lf\n", totalFrames/(GetWallTime()-startTime));


	// Clean up allocations
	free(pos);
	glDeleteVertexArrays(1, &(oglo.VAO));
	glDeleteBuffers(1, &(oglo.pos1VBO));
	glDeleteBuffers(1, &(oglo.pos2VBO));
	glDeleteVertexArrays(1, &(oglo.cubeVAO));
	glDeleteBuffers(1, &(oglo.cubeVBO));
	glfwTerminate();
	return EXIT_SUCCESS;
}



int setupOpenGL(openglObjects *oglo, const unsigned int xres, const unsigned int yres)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef FOROSX
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_SAMPLES, 4);

	oglo->window = glfwCreateWindow(xres, yres, "attractors", NULL, NULL);
	// For fullscreen:
	//*window = glfwCreateWindow(xres, yres, "attractors", glfwGetPrimaryMonitor(), NULL);
	if(oglo->window == NULL) {
		fprintf(stderr, "Error in glfwCreateWindow\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(oglo->window);
	glfwSetInputMode(oglo->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// set initial cursor position
	glfwGetCursorPos(oglo->window, &prevX, &prevY);

	// vsync? 0 disabled, 1 enabled
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(oglo->window, framebufferSizeCallback);
	glfwSetCursorPosCallback(oglo->window, mousePointerCallback);
	glViewport(0, 0, xres, yres);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Error, failed to initialize GLEW. Line: %d\n", __LINE__);
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);


	// shaders and buffers for particles
	oglo->vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(oglo->vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(oglo->vertexShader);
	int success;
	char compileLog[OGLLOGSIZE];
	glGetShaderiv(oglo->vertexShader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->vertexShader, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in particles vertex shader compilation:\n%s\n", compileLog);
	}

	oglo->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(oglo->fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(oglo->fragmentShader);
	glGetShaderiv(oglo->fragmentShader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->fragmentShader, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in particles fragment shader compilation:\n%s\n", compileLog);
	}

	oglo->shaderProgram = glCreateProgram();
	glAttachShader(oglo->shaderProgram, oglo->vertexShader);
	glAttachShader(oglo->shaderProgram, oglo->fragmentShader);

	const char* varyings = "posNew";
	glTransformFeedbackVaryings(oglo->shaderProgram, 1, &varyings, GL_INTERLEAVED_ATTRIBS);

	glLinkProgram(oglo->shaderProgram);
	glGetProgramiv(oglo->shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(oglo->shaderProgram, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in particles program compilation:\n%s\n", compileLog);
	}
	glDeleteShader(oglo->vertexShader);
	glDeleteShader(oglo->fragmentShader);
	oglo->scaleFactorLocation = glGetUniformLocation(oglo->shaderProgram, "scaleFactor");
	oglo->translationMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "translationMatrix");
	oglo->rotationMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "rotationMatrix");
	oglo->cameraMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "cameraMatrix");
	oglo->perspectiveMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "perspectiveMatrix");

	oglo->XLocation = glGetUniformLocation(oglo->shaderProgram, "X");
	oglo->YLocation = glGetUniformLocation(oglo->shaderProgram, "Y");
	oglo->ZLocation = glGetUniformLocation(oglo->shaderProgram, "Z");

	oglo->stepSizeLocation = glGetUniformLocation(oglo->shaderProgram, "stepSize");
	oglo->updatesPerFrameLocation = glGetUniformLocation(oglo->shaderProgram, "updatesPerFrame");
	glUseProgram(oglo->shaderProgram);

	glGenVertexArrays(1, &(oglo->VAO));
	glBindVertexArray(oglo->VAO);

	glGenBuffers(1, &(oglo->pos1VBO));
	glBindBuffer(GL_ARRAY_BUFFER, oglo->pos1VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*NPARTICLES, 0, GL_STREAM_DRAW);

	glGenBuffers(1, &(oglo->pos2VBO));
	glBindBuffer(GL_ARRAY_BUFFER, oglo->pos2VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*NPARTICLES, 0, GL_STREAM_DRAW);


	// shaders and buffers for cube
	oglo->vertexShaderCube = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(oglo->vertexShaderCube, 1, &vertexShaderCubeSource, NULL);
	glCompileShader(oglo->vertexShaderCube);
	glGetShaderiv(oglo->vertexShaderCube, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->vertexShaderCube, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in cube vertex shader compilation:\n%s\n", compileLog);
	}

	oglo->fragmentShaderCube = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(oglo->fragmentShaderCube, 1, &fragmentShaderCubeSource, NULL);
	glCompileShader(oglo->fragmentShaderCube);
	glGetShaderiv(oglo->fragmentShaderCube, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->fragmentShaderCube, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in cube fragment shader compilation:\n%s\n", compileLog);
	}

	oglo->shaderProgramCube = glCreateProgram();
	glAttachShader(oglo->shaderProgramCube, oglo->vertexShaderCube);
	glAttachShader(oglo->shaderProgramCube, oglo->fragmentShaderCube);

	glLinkProgram(oglo->shaderProgramCube);
	glGetProgramiv(oglo->shaderProgramCube, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(oglo->shaderProgramCube, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in cube program compilation:\n%s\n", compileLog);
	}
	glDeleteShader(oglo->vertexShaderCube);
	glDeleteShader(oglo->fragmentShaderCube);
	oglo->cameraMatrixCubeLocation = glGetUniformLocation(oglo->shaderProgramCube, "cameraMatrix");
	oglo->perspectiveMatrixCubeLocation = glGetUniformLocation(oglo->shaderProgramCube, "perspectiveMatrix");

	glUseProgram(oglo->shaderProgramCube);
	glGenVertexArrays(1, &(oglo->cubeVAO));
	glBindVertexArray(oglo->cubeVAO);

	glGenBuffers(1, &(oglo->cubeVBO));
	glBindBuffer(GL_ARRAY_BUFFER, oglo->cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*24, 0, GL_STATIC_DRAW);

	return EXIT_SUCCESS;
}



void updateGLData(unsigned int *dstVBO, float *src, unsigned int size)
{
	glBindBuffer(GL_ARRAY_BUFFER, *dstVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*size, src);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}



void mousePointerCallback(GLFWwindow* window, double xpos, double ypos)
{
	float xoffset = xpos - prevX;
	float yoffset = ypos - prevY;
	prevX = xpos;
	prevY = ypos;

	xoffset *= MOUSESENSITIVITY;
	yoffset *= MOUSESENSITIVITY;

	yaw += xoffset;
	pitch += yoffset;

	updateTransformationUniformsRequired = 1;
}



void initializeParticlePositions(float *pos, const float volSize)
{
	for(size_t i = 0; i < NPARTICLES; i++) {
		pos[3*i+0] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
		pos[3*i+1] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
		pos[3*i+2] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
	}
}



void setAttractorParameters(openglObjects *oglo, unsigned int attractor)
{
	float X[NPARAMETERS];
	float Y[NPARAMETERS];
	float Z[NPARAMETERS];

	// initialize all to zero
	for(size_t i = 0; i < NPARAMETERS; i++) {
		X[i] = 0.0f;
		Y[i] = 0.0f;
		Z[i] = 0.0f;
	}

	// now set non-zero parameters of each attractor
	switch(attractor) {
		// Lorenz
		case 1:
			X[1] = -10.0f;
			X[2] = 10.0f;
			Y[1] = 28.0f;
			Y[2] = -1.0f;
			Y[6] = -1.0f;
			Z[3] = -8.0f/3.0f;
			Z[5] = 1.0f;
			break;
		// Roessler
		case 2:
			X[2] = -1.0f;
			X[3] = -1.0f;
			Y[1] = 1.0f;
			Y[2] = 0.1f;
			Z[0] = 0.1f;
			Z[3] = -14.0f;
			Z[6] = 1.0f;
			break;
		// Lu Chen
		case 3:
			X[1] = -36.0f;
			X[2] = 36.0f;
			Y[0] = 7.0f;
			Y[1] = 1.0f;
			Y[2] = 20.0f;
			Y[6] = -1.0f;
			Z[3] = -3.0f;
			Z[5] = 1.0f;
			break;
		default:
			printf("Error, unrecognised attractor %u\n", attractor);
	}

	// update values in shader
	glUniform1fv(oglo->XLocation, NPARAMETERS, X);
	glUniform1fv(oglo->YLocation, NPARAMETERS, Y);
	glUniform1fv(oglo->ZLocation, NPARAMETERS, Z);
}



void prepareCubeVertices(openglObjects *oglo)
{
	// vertex pairs for lines
	const float cubeVertices[] =
		{-CUBESIZE, -CUBESIZE, -CUBESIZE,
		-CUBESIZE, -CUBESIZE, CUBESIZE,
		-CUBESIZE, -CUBESIZE, -CUBESIZE,
		-CUBESIZE, CUBESIZE, -CUBESIZE,
		-CUBESIZE, -CUBESIZE, -CUBESIZE,
		CUBESIZE, -CUBESIZE, -CUBESIZE,
		-CUBESIZE, -CUBESIZE, CUBESIZE,
		-CUBESIZE, CUBESIZE, CUBESIZE,
		-CUBESIZE, -CUBESIZE, CUBESIZE,
		CUBESIZE, -CUBESIZE, CUBESIZE,
		-CUBESIZE, CUBESIZE, -CUBESIZE,
		-CUBESIZE, CUBESIZE, CUBESIZE,
		-CUBESIZE, CUBESIZE, -CUBESIZE,
		CUBESIZE, CUBESIZE, -CUBESIZE,
		-CUBESIZE, CUBESIZE, CUBESIZE,
		CUBESIZE, CUBESIZE, CUBESIZE,
		CUBESIZE, CUBESIZE, -CUBESIZE,
		CUBESIZE, CUBESIZE, CUBESIZE,
		CUBESIZE, CUBESIZE, -CUBESIZE,
		CUBESIZE, -CUBESIZE, -CUBESIZE,
		CUBESIZE, CUBESIZE, CUBESIZE,
		CUBESIZE, -CUBESIZE, CUBESIZE,
		CUBESIZE, -CUBESIZE, -CUBESIZE,
		CUBESIZE, -CUBESIZE, CUBESIZE};

	glUseProgram(oglo->shaderProgramCube);
	glBindBuffer(GL_ARRAY_BUFFER, oglo->cubeVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*24, cubeVertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void updateTransformationUniforms(openglObjects *oglo, float theta, float phi, unsigned int xres, unsigned int yres, glm::vec3 cameraPosition)
{
	// rotation matrix, rotate by theta w.r.t. x axis and phi w.r.t. t axis:
	glm::mat4 rotationMatrix = glm::mat4(1.0f);
	rotationMatrix = glm::rotate(rotationMatrix, theta, glm::vec3(1.0f, 0.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, phi, glm::vec3(0.0f, 1.0f, 0.0f));

	// camera projection
	glm::vec3 cameraDirection = glm::vec3(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw));
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 cameraMatrix = glm::lookAt(cameraPosition, cameraPosition+cameraDirection, cameraUp);

	// perspective transformation: fov, aspect ratio, near plane, far plane
	glm::mat4 perspectiveMatrix = glm::perspective(45.0f, (float)xres/(float)yres, 0.0f, 100.0f);

	// set uniforms in both vertex shaders
	glUseProgram(oglo->shaderProgramCube);
	glUniformMatrix4fv(oglo->cameraMatrixCubeLocation, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
	glUniformMatrix4fv(oglo->perspectiveMatrixCubeLocation, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
	glUseProgram(oglo->shaderProgram);
	glUniformMatrix4fv(oglo->rotationMatrixLocation, 1, GL_FALSE, glm::value_ptr(rotationMatrix));
	glUniformMatrix4fv(oglo->cameraMatrixLocation, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
	glUniformMatrix4fv(oglo->perspectiveMatrixLocation, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
}
