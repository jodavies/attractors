#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GetWallTime.h"

#define NPARTICLES 2500000
#define NPARAMETERS 10

const char *vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 pos;\n"
	"out vec3 posNew;\n"
	"out vec4 colour;\n"
	""
	"uniform float scaleFactor;\n"
	"uniform mat4 rotationMatrix;\n"
	"uniform mat4 translationMatrix;\n"
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
	"	gl_Position = translationMatrix * rotationMatrix * vec4(posNew/scaleFactor, 1.0);\n"
	"	colour = vec4(\n"
	"		+ vec3(40.0f/255.0f, 0.0f, 100.0f/255.0f)\n"
	"		+ 100.0/speed * vec3(225.0f/255.0f, 100.0f/255.0f, 0.0f)\n"
	"		, 0.01f);\n"
	"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec4 colour;\n"
	"void main()\n"
	"{\n"
	"   FragColor = colour;\n"
	"}\0";


// Struct to hold opengl objects
typedef struct {
	GLFWwindow *window;
	unsigned int vertexShader, fragmentShader, shaderProgram;
	unsigned int VAO, pos1VBO, pos2VBO;

	// uniforms:
	unsigned int scaleFactorLocation;
	unsigned int rotationMatrixLocation;
	unsigned int translationMatrixLocation;
	// attractor parameters
	unsigned int XLocation;
	unsigned int YLocation;
	unsigned int ZLocation;
	// for integration
	unsigned int stepSizeLocation;
	unsigned int updatesPerFrameLocation;
} openglObjects;


int setupOpenGL(openglObjects *oglo, const unsigned int xres, const unsigned int yres);
void updateGLData(unsigned int *pos1VBO, float *pos);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

void initializeParticlePositions(float* pos, const float volSize);
void updateRotationMatrix(float *rotationMatrix, const float theta, const float phi);
void setAttractorParameters(openglObjects *oglo, unsigned int attractor);


int main(void)
{
	printf("attractors\n");


	const int xres = 1900;
	const int yres = 1180;
	openglObjects oglo;
	setupOpenGL(&oglo, xres, yres);


	// point positions and velocities
	float *pos = malloc(NPARTICLES * 3 * sizeof(float));
	float *vel = malloc(NPARTICLES * 3 * sizeof(float));
	initializeParticlePositions(pos, 40.0f);
	updateGLData(&(oglo.pos1VBO), pos);

	// shader uniforms
	float scaleFactor = 70.0f;
	glUniform1f(oglo.scaleFactorLocation, scaleFactor);

	float theta = 0.0f;
	float phi = 0.0f;
	float rotationMatrix[16] = {0.0f};
	updateRotationMatrix(rotationMatrix, theta, phi);
	glUniformMatrix4fv(oglo.rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);

	float translationMatrix[] =
		{1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
	glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, translationMatrix);

	// attractor
	setAttractorParameters(&oglo, 1);

	// for integration
	float stepSize = 0.001f;
	glUniform1f(oglo.stepSizeLocation, stepSize);
	int updatesPerFrame = 20;
	glUniform1i(oglo.updatesPerFrameLocation, updatesPerFrame);


	double startTime = GetWallTime();
	unsigned int totalFrames = 0;
	// Start event loop
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
			updateGLData(&(oglo.pos1VBO), pos);
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

		if(glfwGetKey(oglo.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			translationMatrix[12] += 0.01f;
			glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			translationMatrix[12] -= 0.01f;
			glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_UP) == GLFW_PRESS) {
			translationMatrix[13] += 0.01f;
			glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			translationMatrix[13] -= 0.01f;
			glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_W) == GLFW_PRESS) {
			theta -= 0.05f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(oglo.rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_S) == GLFW_PRESS) {
			theta += 0.05f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(oglo.rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_A) == GLFW_PRESS) {
			phi += 0.05f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(oglo.rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_D) == GLFW_PRESS) {
			phi -= 0.05f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(oglo.rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(oglo.window, 1);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

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
	}
	printf("fps: %lf\n", totalFrames/(GetWallTime()-startTime));


	free(pos);
	free(vel);
	glDeleteVertexArrays(1, &(oglo.VAO));
	glDeleteBuffers(1, &(oglo.pos1VBO));
	glDeleteBuffers(1, &(oglo.pos2VBO));
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

	// vsync? 0 disabled, 1 enabled
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(oglo->window, framebufferSizeCallback);
	glViewport(0, 0, xres, yres);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Error, failed to initialize GLEW. Line: %d\n", __LINE__);
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	oglo->vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(oglo->vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(oglo->vertexShader);
	int success;
	char compileLog[512];
	glGetShaderiv(oglo->vertexShader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->vertexShader, 512, NULL, compileLog);
		fprintf(stderr, "Error in vertex shader compilation:\n%s\n", compileLog);
	}

	oglo->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(oglo->fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(oglo->fragmentShader);
	glGetShaderiv(oglo->fragmentShader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->fragmentShader, 512, NULL, compileLog);
		fprintf(stderr, "Error in fragment shader compilation:\n%s\n", compileLog);
	}

	oglo->shaderProgram = glCreateProgram();
	glAttachShader(oglo->shaderProgram, oglo->vertexShader);
	glAttachShader(oglo->shaderProgram, oglo->fragmentShader);

	const char* varyings = "posNew";
	glTransformFeedbackVaryings(oglo->shaderProgram, 1, &varyings, GL_INTERLEAVED_ATTRIBS);

	glLinkProgram(oglo->shaderProgram);
	glGetProgramiv(oglo->shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(oglo->shaderProgram, 512, NULL, compileLog);
		fprintf(stderr, "Error in program compilation:\n%s\n", compileLog);
	}
	glDeleteShader(oglo->vertexShader);
	glDeleteShader(oglo->fragmentShader);
	oglo->scaleFactorLocation = glGetUniformLocation(oglo->shaderProgram, "scaleFactor");
	oglo->rotationMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "rotationMatrix");
	oglo->translationMatrixLocation = glGetUniformLocation(oglo->shaderProgram, "translationMatrix");

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

	return EXIT_SUCCESS;
}



void updateGLData(unsigned int *pos1VBO, float *pos)
{
	glBindBuffer(GL_ARRAY_BUFFER, *pos1VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*NPARTICLES, pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}



void initializeParticlePositions(float *pos, const float volSize)
{
	for(size_t i = 0; i < NPARTICLES; i++) {
		pos[3*i+0] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
		pos[3*i+1] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
		pos[3*i+2] = 2.0f * volSize * (rand()/(float)RAND_MAX-0.5f);
	}
}



void updateRotationMatrix(float *rotationMatrix, const float theta, const float phi)
{
	rotationMatrix[0]  = cos(phi);
	rotationMatrix[1]  = sin(phi)*sin(theta);
	rotationMatrix[2]  = -cos(theta)*sin(phi);
	rotationMatrix[3]  = 0.0f;
	rotationMatrix[4]  = 0.0f;
	rotationMatrix[5]  = cos(theta);
	rotationMatrix[6]  = sin(theta);
	rotationMatrix[7]  = 0.0f;
	rotationMatrix[8]  = sin(phi);
	rotationMatrix[9]  = -cos(phi)*sin(theta);
	rotationMatrix[10] = cos(phi)*cos(theta);
	rotationMatrix[11] = 0.0f;
	rotationMatrix[12] = 0.0f;
	rotationMatrix[13] = 0.0f;
	rotationMatrix[14] = 0.0f;
	rotationMatrix[15] = 1.0f;
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
