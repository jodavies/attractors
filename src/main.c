#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GetWallTime.h"

#define NPARTICLES 2500000
// lorenz parameters
#define RHO 28.0
#define SIGMA 10.0
#define BETA (8.0/3.0)
#define STEPSIZE 0.001

const char *vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 pos;\n"
	"out vec3 posNew;\n"
	"out vec4 colour;\n"
	""
	"uniform float scaleFactor;\n"
	"uniform mat4 rotationMatrix;\n"
	"uniform mat4 translationMatrix;\n"
	""
	"uniform float xx;\n"
	"uniform float xy;\n"
	"uniform float xz;\n"
	"uniform float xxx;\n"
	"uniform float xxy;\n"
	"uniform float xxz;\n"
	"uniform float xyy;\n"
	"uniform float xyz;\n"
	"uniform float xzz;\n"
	"uniform float x0;\n"
	""
	"uniform float yx;\n"
	"uniform float yy;\n"
	"uniform float yz;\n"
	"uniform float yxx;\n"
	"uniform float yxy;\n"
	"uniform float yxz;\n"
	"uniform float yyy;\n"
	"uniform float yyz;\n"
	"uniform float yzz;\n"
	"uniform float y0;\n"
	""
	"uniform float zx;\n"
	"uniform float zy;\n"
	"uniform float zz;\n"
	"uniform float zxx;\n"
	"uniform float zxy;\n"
	"uniform float zxz;\n"
	"uniform float zyy;\n"
	"uniform float zyz;\n"
	"uniform float zzz;\n"
	"uniform float z0;\n"
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
	"		velx = xx*x + xy*y + xz*z + xxx*x*x + xxy*x*y + xxz*x*z + xyy*y*y + xyz*y*z + xzz*z*z + x0;\n"
	"		vely = yx*x + yy*y + yz*z + yxx*x*x + yxy*x*y + yxz*x*z + yyy*y*y + yyz*y*z + yzz*z*z + y0;\n"
	"		velz = zx*x + zy*y + zz*z + zxx*x*x + zxy*x*y + zxz*x*z + zyy*y*y + zyz*y*z + zzz*z*z + z0;\n"
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
	unsigned int xLocation[10];
	unsigned int yLocation[10];
	unsigned int zLocation[10];
	// for integration
	unsigned int stepSizeLocation;
	unsigned int updatesPerFrameLocation;
} openglObjects;


int setupOpenGL(openglObjects *oglo, const unsigned int xres, const unsigned int yres);
void updateGLData(unsigned int *pos1VBO, float *pos);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

void initializeParticlePositions(float* pos, const float volSize);
void updateParticlePositions(float *pos, float *vel);
void updateRotationMatrix(float *rotationMatrix, const float theta, const float phi);


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

	// for lorenz attractor
/*	glUniform1f(oglo.xLocation[0], -10.0f);
	glUniform1f(oglo.xLocation[1], 10.0f);
	glUniform1f(oglo.xLocation[2], 0.0f);
	glUniform1f(oglo.xLocation[3], 0.0f);
	glUniform1f(oglo.xLocation[4], 0.0f);
	glUniform1f(oglo.xLocation[5], 0.0f);
	glUniform1f(oglo.xLocation[6], 0.0f);
	glUniform1f(oglo.xLocation[7], 0.0f);
	glUniform1f(oglo.xLocation[8], 0.0f);
	glUniform1f(oglo.xLocation[9], 0.0f);
	glUniform1f(oglo.yLocation[0], 28.0f);
	glUniform1f(oglo.yLocation[1], -1.0f);
	glUniform1f(oglo.yLocation[2], 0.0f);
	glUniform1f(oglo.yLocation[3], 0.0f);
	glUniform1f(oglo.yLocation[4], 0.0f);
	glUniform1f(oglo.yLocation[5], -1.0f);
	glUniform1f(oglo.yLocation[6], 0.0f);
	glUniform1f(oglo.yLocation[7], 0.0f);
	glUniform1f(oglo.yLocation[8], 0.0f);
	glUniform1f(oglo.yLocation[9], 0.0f);
	glUniform1f(oglo.zLocation[0], 0.0f);
	glUniform1f(oglo.zLocation[1], 0.0f);
	glUniform1f(oglo.zLocation[2], -8.0f/3.0f);
	glUniform1f(oglo.zLocation[3], 0.0f);
	glUniform1f(oglo.zLocation[4], 1.0f);
	glUniform1f(oglo.zLocation[5], 0.0f);
	glUniform1f(oglo.zLocation[6], 0.0f);
	glUniform1f(oglo.zLocation[7], 0.0f);
	glUniform1f(oglo.zLocation[8], 0.0f);
	glUniform1f(oglo.zLocation[9], 0.0f);*/

	// for Roessler attractor
/*	glUniform1f(oglo.xLocation[0], 0.0f);
	glUniform1f(oglo.xLocation[1], -1.0f);
	glUniform1f(oglo.xLocation[2], -1.0f);
	glUniform1f(oglo.xLocation[3], 0.0f);
	glUniform1f(oglo.xLocation[4], 0.0f);
	glUniform1f(oglo.xLocation[5], 0.0f);
	glUniform1f(oglo.xLocation[6], 0.0f);
	glUniform1f(oglo.xLocation[7], 0.0f);
	glUniform1f(oglo.xLocation[8], 0.0f);
	glUniform1f(oglo.xLocation[9], 0.0f);
	glUniform1f(oglo.yLocation[0], 1.0f);
	glUniform1f(oglo.yLocation[1], 0.1f);
	glUniform1f(oglo.yLocation[2], 0.0f);
	glUniform1f(oglo.yLocation[3], 0.0f);
	glUniform1f(oglo.yLocation[4], 0.0f);
	glUniform1f(oglo.yLocation[5], 0.0f);
	glUniform1f(oglo.yLocation[6], 0.0f);
	glUniform1f(oglo.yLocation[7], 0.0f);
	glUniform1f(oglo.yLocation[8], 0.0f);
	glUniform1f(oglo.yLocation[9], 0.0f);
	glUniform1f(oglo.zLocation[0], 0.0f);
	glUniform1f(oglo.zLocation[1], 0.0f);
	glUniform1f(oglo.zLocation[2], -14.0f);
	glUniform1f(oglo.zLocation[3], 0.0f);
	glUniform1f(oglo.zLocation[4], 0.0f);
	glUniform1f(oglo.zLocation[5], 1.0f);
	glUniform1f(oglo.zLocation[6], 0.0f);
	glUniform1f(oglo.zLocation[7], 0.0f);
	glUniform1f(oglo.zLocation[8], 0.0f);
	glUniform1f(oglo.zLocation[9], 0.1f);*/

	// for Lu-Chen
	glUniform1f(oglo.xLocation[0], -36.0f);
	glUniform1f(oglo.xLocation[1], 36.0f);
	glUniform1f(oglo.xLocation[2], 0.0f);
	glUniform1f(oglo.xLocation[3], 0.0f);
	glUniform1f(oglo.xLocation[4], 0.0f);
	glUniform1f(oglo.xLocation[5], 0.0f);
	glUniform1f(oglo.xLocation[6], 0.0f);
	glUniform1f(oglo.xLocation[7], 0.0f);
	glUniform1f(oglo.xLocation[8], 0.0f);
	glUniform1f(oglo.xLocation[9], 0.0f);
	glUniform1f(oglo.yLocation[0], 1.0f);
	glUniform1f(oglo.yLocation[1], 20.0f);
	glUniform1f(oglo.yLocation[2], 0.0f);
	glUniform1f(oglo.yLocation[3], 0.0f);
	glUniform1f(oglo.yLocation[4], 0.0f);
	glUniform1f(oglo.yLocation[5], -1.0f);
	glUniform1f(oglo.yLocation[6], 0.0f);
	glUniform1f(oglo.yLocation[7], 0.0f);
	glUniform1f(oglo.yLocation[8], 0.0f);
	glUniform1f(oglo.yLocation[9], 7.0f);
	glUniform1f(oglo.zLocation[0], 0.0f);
	glUniform1f(oglo.zLocation[1], 0.0f);
	glUniform1f(oglo.zLocation[2], -3.0f);
	glUniform1f(oglo.zLocation[3], 0.0f);
	glUniform1f(oglo.zLocation[4], 1.0f);
	glUniform1f(oglo.zLocation[5], 0.0f);
	glUniform1f(oglo.zLocation[6], 0.0f);
	glUniform1f(oglo.zLocation[7], 0.0f);
	glUniform1f(oglo.zLocation[8], 0.0f);
	glUniform1f(oglo.zLocation[9], 0.0f);

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

	oglo->xLocation[0] = glGetUniformLocation(oglo->shaderProgram, "xx");
	oglo->xLocation[1] = glGetUniformLocation(oglo->shaderProgram, "xy");
	oglo->xLocation[2] = glGetUniformLocation(oglo->shaderProgram, "xz");
	oglo->xLocation[3] = glGetUniformLocation(oglo->shaderProgram, "xxz");
	oglo->xLocation[4] = glGetUniformLocation(oglo->shaderProgram, "xxy");
	oglo->xLocation[5] = glGetUniformLocation(oglo->shaderProgram, "xxz");
	oglo->xLocation[6] = glGetUniformLocation(oglo->shaderProgram, "xyy");
	oglo->xLocation[7] = glGetUniformLocation(oglo->shaderProgram, "xyz");
	oglo->xLocation[8] = glGetUniformLocation(oglo->shaderProgram, "xzz");
	oglo->xLocation[9] = glGetUniformLocation(oglo->shaderProgram, "x0");

	oglo->yLocation[0] = glGetUniformLocation(oglo->shaderProgram, "yx");
	oglo->yLocation[1] = glGetUniformLocation(oglo->shaderProgram, "yy");
	oglo->yLocation[2] = glGetUniformLocation(oglo->shaderProgram, "yz");
	oglo->yLocation[3] = glGetUniformLocation(oglo->shaderProgram, "yxz");
	oglo->yLocation[4] = glGetUniformLocation(oglo->shaderProgram, "yxy");
	oglo->yLocation[5] = glGetUniformLocation(oglo->shaderProgram, "yxz");
	oglo->yLocation[6] = glGetUniformLocation(oglo->shaderProgram, "yyy");
	oglo->yLocation[7] = glGetUniformLocation(oglo->shaderProgram, "yyz");
	oglo->yLocation[8] = glGetUniformLocation(oglo->shaderProgram, "yzz");
	oglo->yLocation[9] = glGetUniformLocation(oglo->shaderProgram, "y0");

	oglo->zLocation[0] = glGetUniformLocation(oglo->shaderProgram, "zx");
	oglo->zLocation[1] = glGetUniformLocation(oglo->shaderProgram, "zy");
	oglo->zLocation[2] = glGetUniformLocation(oglo->shaderProgram, "zz");
	oglo->zLocation[3] = glGetUniformLocation(oglo->shaderProgram, "zxz");
	oglo->zLocation[4] = glGetUniformLocation(oglo->shaderProgram, "zxy");
	oglo->zLocation[5] = glGetUniformLocation(oglo->shaderProgram, "zxz");
	oglo->zLocation[6] = glGetUniformLocation(oglo->shaderProgram, "zyy");
	oglo->zLocation[7] = glGetUniformLocation(oglo->shaderProgram, "zyz");
	oglo->zLocation[8] = glGetUniformLocation(oglo->shaderProgram, "zzz");
	oglo->zLocation[9] = glGetUniformLocation(oglo->shaderProgram, "z0");

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
