#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NPARTICLES 500000
// lorenz parameters
#define RHO 28.0
#define SIGMA 10.0
#define BETA (8.0/3.0)
#define STEPSIZE 0.001

const char *vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 aPos;\n"
	"out vec4 colour;\n"
	"uniform float scaleFactor;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4(aPos/scaleFactor, 1.0);\n"
	"	colour = vec4(normalize(aPos), 0.1);\n"
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
                unsigned int *shaderProgram, unsigned int *vao, unsigned int *vbo,
                unsigned int *scaleFactorLocation);
void updateGLData();
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

void initializeParticlePositions(float* pos, const float volSize);
void updateParticlePositions(float *pos, float *vel);

int main(void)
{
	printf("attractors\n");

	const int xres = 1900;
	const int yres = 1180;
	GLFWwindow *window = NULL;
	unsigned int vertexShader, fragmentShader, shaderProgram, vao, vbo, scaleFactorLocation;
	setupOpenGL(&window, xres, yres, &vertexShader, &fragmentShader, &shaderProgram, &vao, &vbo, &scaleFactorLocation);


	// point positions and velocities
	float *pos = malloc(NPARTICLES * 3 * sizeof(float));
	float *vel = malloc(NPARTICLES * 3 * sizeof(float));
	initializeParticlePositions(pos, 20.0f);
	updateGLData(&vbo, pos);


	float scaleFactor = 70.0f;
	glUniform1f(scaleFactorLocation, scaleFactor);


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
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		updateParticlePositions(pos, vel);
		updateGLData(&vbo, pos);

		glDrawArrays(GL_POINTS, 0, NPARTICLES);
		glfwSwapBuffers(window);
		glfwPollEvents();

	}


	free(pos);
	free(vel);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glfwTerminate();
	return EXIT_SUCCESS;
}



int setupOpenGL(GLFWwindow **window, const unsigned int xres, const unsigned int yres,
                unsigned int *vertexShader, unsigned int *fragmentShader,
                unsigned int *shaderProgram, unsigned int *vao, unsigned int *vbo,
                unsigned int *scaleFactorLocation)
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
		fprintf(stderr, "Error in vertex shader compilation:\n%s", compileLog);
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
		fprintf(stderr, "Error in program compilation:\n%s", compileLog);
	}
	glDeleteShader(*vertexShader);
	glDeleteShader(*fragmentShader);
	*scaleFactorLocation = glGetUniformLocation(*shaderProgram, "scaleFactor");
	glUseProgram(*shaderProgram);

	//gen
	glGenVertexArrays(1, vao);
	glGenBuffers(1, vbo);
	// bind
	glBindVertexArray(*vao);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	return EXIT_SUCCESS;
}



void updateGLData(unsigned int *vbo, float *pos)
{
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*NPARTICLES, pos, GL_STREAM_DRAW);
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
	for(size_t i = 0; i < NPARTICLES; i++) {
		vel[3*i+0] = (SIGMA*(pos[3*i+1]-pos[3*i+0]));
		vel[3*i+1] = (pos[3*i+0]*(RHO-pos[3*i+2])-pos[3*i+1]);
		vel[3*i+2] = (pos[3*i+0]*pos[3*i+1]-BETA*pos[3*i+2]);
		pos[3*i+0] += STEPSIZE*vel[3*i+0];
		pos[3*i+1] += STEPSIZE*vel[3*i+1];
		pos[3*i+2] += STEPSIZE*vel[3*i+2];
	}
}
