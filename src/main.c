#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GetWallTime.h"

#define NPARTICLES 1000000
// lorenz parameters
#define RHO 28.0
#define SIGMA 10.0
#define BETA (8.0/3.0)
#define STEPSIZE 0.001

const char *vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 pos;\n"
	"layout (location = 1) in vec3 vel;\n"
	"out vec4 colour;\n"
	"uniform float scaleFactor;\n"
	"uniform mat4 rotationMatrix;\n"
	"uniform mat4 translationMatrix;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = translationMatrix * rotationMatrix * vec4(pos/scaleFactor, 1.0);\n"
	"	float speed = length(vel);\n"
	"	colour = vec4(\n"
	"		+ vec3(40.0f/255.0f, 0.0f, 100.0f/255.0f)\n"
	"		+ 100.0/speed * vec3(225.0f/255.0f, 100.0f/255.0f, 0.0f)\n"
	"		, 0.1f);\n"
	"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec4 colour;\n"
	"void main()\n"
	"{\n"
	"   FragColor = colour;\n"
	"}\0";

int setupOpenGL(GLFWwindow **window, const unsigned int xres, const unsigned int yres,
                unsigned int *vertexShader, unsigned int *fragmentShader,
                unsigned int *shaderProgram, unsigned int *VAO, unsigned int *posVBO,
                unsigned int *velVBO, unsigned int *scaleFactorLocation, unsigned int *rotationMatrixLocation,
                unsigned int *translationMatrixLocation);
void updateGLData(unsigned int *posVBO, float *pos, unsigned int *velVBO, float *vel);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

void initializeParticlePositions(float* pos, const float volSize);
void updateParticlePositions(float *pos, float *vel);
void updateRotationMatrix(float *rotationMatrix, const float theta, const float phi);

int main(void)
{
	printf("attractors\n");

	const int xres = 1900;
	const int yres = 1180;
	GLFWwindow *window = NULL;
	unsigned int vertexShader, fragmentShader, shaderProgram, VAO, posVBO, velVBO, scaleFactorLocation, rotationMatrixLocation, translationMatrixLocation;
	setupOpenGL(&window, xres, yres, &vertexShader, &fragmentShader, &shaderProgram, &VAO, &posVBO, &velVBO, &scaleFactorLocation, &rotationMatrixLocation, &translationMatrixLocation);


	// point positions and velocities
	float *pos = malloc(NPARTICLES * 3 * sizeof(float));
	float *vel = malloc(NPARTICLES * 3 * sizeof(float));
	initializeParticlePositions(pos, 40.0f);

	float scaleFactor = 70.0f;
	glUniform1f(scaleFactorLocation, scaleFactor);

	float theta = 0.0f;
	float phi = 0.0f;
	float rotationMatrix[16] = {0.0f};
	updateRotationMatrix(rotationMatrix, theta, phi);
	glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);

	float translationMatrix[] =
		{1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
	glUniformMatrix4fv(translationMatrixLocation, 1, GL_FALSE, translationMatrix);


	double startTime = GetWallTime();
	unsigned int totalFrames = 0;
	// Start event loop
	while(!glfwWindowShouldClose(window)) {

		// User control
		if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
			scaleFactor *= 1.1f;
			glUniform1f(scaleFactorLocation, scaleFactor);
		}
		if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
			scaleFactor /= 1.1f;
			glUniform1f(scaleFactorLocation, scaleFactor);
		}

		if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			translationMatrix[12] += 0.01f;
			glUniformMatrix4fv(translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			translationMatrix[12] -= 0.01f;
			glUniformMatrix4fv(translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			translationMatrix[13] += 0.01f;
			glUniformMatrix4fv(translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			translationMatrix[13] -= 0.01f;
			glUniformMatrix4fv(translationMatrixLocation, 1, GL_FALSE, translationMatrix);
		}

		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			theta -= 0.01f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			theta += 0.01f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			phi += 0.01f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			phi -= 0.01f;
			updateRotationMatrix(rotationMatrix, theta, phi);
			glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, rotationMatrix);
		}

		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for(unsigned int i = 0; i < 10; i++) {
			updateParticlePositions(pos, vel);
		}
		updateGLData(&posVBO, pos, &velVBO, vel);

		glDrawArrays(GL_POINTS, 0, NPARTICLES);
		glfwSwapBuffers(window);
		glfwPollEvents();
		totalFrames++;
	}
	printf("fps: %lf\n", totalFrames/(GetWallTime()-startTime));


	free(pos);
	free(vel);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &posVBO);
	glDeleteBuffers(1, &velVBO);
	glfwTerminate();
	return EXIT_SUCCESS;
}



int setupOpenGL(GLFWwindow **window, const unsigned int xres, const unsigned int yres,
                unsigned int *vertexShader, unsigned int *fragmentShader,
                unsigned int *shaderProgram, unsigned int *VAO, unsigned int *posVBO,
                unsigned int *velVBO, unsigned int *scaleFactorLocation, unsigned int *rotationMatrixLocation,
                unsigned int *translationMatrixLocation)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	*window = glfwCreateWindow(xres, yres, "attractors", NULL, NULL);
	if(*window == NULL) {
		fprintf(stderr, "Error in glfwCreateWindow\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(*window);

	// vsync
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(*window, framebufferSizeCallback);
	glViewport(0, 0, xres, yres);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Error, failed to initialize GLEW. Line: %d\n", __LINE__);
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	*vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(*vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(*vertexShader);
	int success;
	char compileLog[512];
	glGetShaderiv(*vertexShader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(*vertexShader, 512, NULL, compileLog);
		fprintf(stderr, "Error in vertex shader compilation:\n%s\n", compileLog);
	}

	*fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(*fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(*fragmentShader);

	*shaderProgram = glCreateProgram();
	glAttachShader(*shaderProgram, *vertexShader);
	glAttachShader(*shaderProgram, *fragmentShader);
	glLinkProgram(*shaderProgram);
	glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(*shaderProgram, 512, NULL, compileLog);
		fprintf(stderr, "Error in program compilation:\n%s\n", compileLog);
	}
	glDeleteShader(*vertexShader);
	glDeleteShader(*fragmentShader);
	*scaleFactorLocation = glGetUniformLocation(*shaderProgram, "scaleFactor");
	*rotationMatrixLocation = glGetUniformLocation(*shaderProgram, "rotationMatrix");
	*translationMatrixLocation = glGetUniformLocation(*shaderProgram, "translationMatrix");
	glUseProgram(*shaderProgram);


	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);

	glGenBuffers(1, posVBO);
	glBindBuffer(GL_ARRAY_BUFFER, *posVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*NPARTICLES, 0, GL_STREAM_DRAW);

	glGenBuffers(1, velVBO);
	glBindBuffer(GL_ARRAY_BUFFER, *velVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*NPARTICLES, 0, GL_STREAM_DRAW);

	return EXIT_SUCCESS;
}



void updateGLData(unsigned int *posVBO, float *pos, unsigned int *velVBO, float *vel)
{
	glBindBuffer(GL_ARRAY_BUFFER, *posVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*NPARTICLES, pos);
	glBindBuffer(GL_ARRAY_BUFFER, *velVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*NPARTICLES, vel);
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



void updateParticlePositions(float *pos, float *vel) {
	#pragma omp parallel for shared(vel,pos)
	for(size_t i = 0; i < NPARTICLES; i++) {
		vel[3*i+0] = (SIGMA*(pos[3*i+1]-pos[3*i+0]));
		vel[3*i+1] = (pos[3*i+0]*(RHO-pos[3*i+2])-pos[3*i+1]);
		vel[3*i+2] = (pos[3*i+0]*pos[3*i+1]-BETA*pos[3*i+2]);
		pos[3*i+0] += STEPSIZE*vel[3*i+0];
		pos[3*i+1] += STEPSIZE*vel[3*i+1];
		pos[3*i+2] += STEPSIZE*vel[3*i+2];
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
