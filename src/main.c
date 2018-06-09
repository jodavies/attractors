#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

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
#define MAXTEXTLENGTH 256

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
	"	colour = vec4(1.0f, 1.0f, 1.0f, 1.0f/(1.0f+cameraDistance));\n"
	"}\0";

const char *fragmentShaderCubeSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec4 colour;\n"
	""
	"void main()\n"
	"{\n"
	"	FragColor = colour;\n"
	"}\0";

const char *vertexShaderTextSource = "#version 330 core\n"
	"layout (location = 0) in vec4 vertex;\n"
	"out vec2 TextureCoords;\n"
	""
	"void main()\n"
	"{\n"
	"	gl_Position = vec4(vertex.xy, 0.0f, 1.0f);\n"
	"	TextureCoords = vertex.zw;\n"
	"}\0";

const char *fragmentShaderTextSource = "#version 330 core\n"
	"in vec2 TextureCoords;\n"
	"out vec4 FragColour;\n"
	"uniform sampler2D text;\n"
	""
	"void main()\n"
	"{\n"
	"	FragColour = vec4(1.0f) * vec4(vec3(1.0f), texture(text, TextureCoords).r);\n"
	"}\0";



// Struct to hold opengl objects
typedef struct {
	GLFWwindow *window;
	unsigned int vertexShader, fragmentShader, shaderProgram;
	unsigned int VAO, pos1VBO, pos2VBO;
	unsigned int vertexShaderCube, fragmentShaderCube, shaderProgramCube;
	unsigned int cubeVAO, cubeVBO;
	unsigned int vertexShaderText, fragmentShaderText, shaderProgramText;
	unsigned int textVAO, textVBO;

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

	// texture for font
	unsigned int fontTex;
	unsigned int fontTexWidth;
	unsigned int fontTexHeight;
} openglObjects;

// Struct for freetype glyph information
typedef struct {
	unsigned int width;
	unsigned int rows;
	int left;
	int top;
	unsigned int advancex;
	unsigned int advancey;
	float xTexCoord;
} glyphInfo;

// Struct for variables used in glfw callback
typedef struct {
	float pitch; // radians
	float yaw; //radians
	double prevX;
	double prevY;
	unsigned int updateTransformationUniformsRequired;
} callbackVariables;


int setupOpenGL(openglObjects *oglo, callbackVariables *cbVars, const unsigned int xres, const unsigned int yres);
void updateGLData(unsigned int *dstVBO, float *src, unsigned int size);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mousePointerCallback(GLFWwindow* window, double xpos, double ypos);
void initializeParticlePositions(float* pos, const float volSize);
void setAttractorParameters(openglObjects *oglo, unsigned int attractor);
void prepareCubeVertices(openglObjects *oglo);
void updateTransformationUniforms(openglObjects *oglo, callbackVariables *cbVars, float theta, float phi, unsigned int xres, unsigned int yres, glm::vec3 cameraPosition);
void ftLoadGlyphs(openglObjects *oglo, FT_Face ftFace, glyphInfo *glyphs);
void renderText(openglObjects *oglo, glyphInfo *glyphs, std::string text, float posx, float posy, int xres, int yres);



int main(void)
{
	printf("attractors\n");

	printf("Controls:\n"
		"   w,a,s,d - move camera\n"
		"   mouse --- aim camera\n"
		"   r ------- reset particle positions\n"
		"   t ------- reset particle positions, small initial volume\n"
		"   p,o ----- pause,resume evolution\n"
		"   l ------- manually advance evolution\n"
		"   z,x ----- scale attractor smaller,larger\n"
		"   arrows -- rotate attractor\n"
		"   1 ------- Lorenz attractor\n"
		"   2 ------- Roessler attractor\n"
		"   3 ------- Lu Chen attractor\n"
	);

	const int xres = 1920;
	const int yres = 1200;
	openglObjects oglo;

	callbackVariables cbVars;
	cbVars.pitch = M_PI;
	cbVars.yaw = -M_PI/2.0f;
	cbVars.prevX = xres/2.0f;
	cbVars.prevY = yres/2.0f;
	cbVars.updateTransformationUniformsRequired = 0;

	if (setupOpenGL(&oglo, &cbVars, xres, yres)) {
		printf("Error in setupOpenGL.\n");
		return EXIT_FAILURE;
	}


	// Freetype
	FT_Library ftLib;
	if(FT_Init_FreeType(&ftLib)) {
		fprintf(stderr, "Error initializing FreeType library\n");
		return EXIT_FAILURE;
	}
	FT_Face ftFace;
	if(FT_New_Face(ftLib, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 0, &ftFace)) {
		fprintf(stderr, "Error opeing font\n");
		return EXIT_FAILURE;
	}
	FT_Set_Pixel_Sizes(ftFace, 0,60);
	glyphInfo glyphs[128];
	ftLoadGlyphs(&oglo, ftFace, glyphs);
	FT_Done_Face(ftFace);
	FT_Done_FreeType(ftLib);


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
	//glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);

	// translation to move points relative to cube -- try to centre the attractors
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f));
	glUniformMatrix4fv(oglo.translationMatrixLocation, 1, GL_FALSE, glm::value_ptr(translationMatrix));


	// Start event loop
	double startTime = GetWallTime();
	double fpsUpdate = 0;
	char fpsString[MAXTEXTLENGTH];
	unsigned int totalFrames = 0;
	unsigned int fpsUpdateFrames = 0;
	unsigned int updateAttractorOnce = 0;

	while(!glfwWindowShouldClose(oglo.window)) {

		// User control
		if(glfwGetKey(oglo.window, GLFW_KEY_Z) == GLFW_PRESS) {
			scaleFactor *= 1.1f;
			glUseProgram(oglo.shaderProgram);
			glUniform1f(oglo.scaleFactorLocation, scaleFactor);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_X) == GLFW_PRESS) {
			scaleFactor /= 1.1f;
			glUseProgram(oglo.shaderProgram);
			glUniform1f(oglo.scaleFactorLocation, scaleFactor);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_R) == GLFW_PRESS) {
			initializeParticlePositions(pos, 40.0f);
			updateGLData(&(oglo.pos1VBO), pos, 3*NPARTICLES);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_T) == GLFW_PRESS) {
			initializeParticlePositions(pos, 0.5f);
			updateGLData(&(oglo.pos1VBO), pos, 3*NPARTICLES);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_P) == GLFW_PRESS) {
			glUseProgram(oglo.shaderProgram);
			glUniform1f(oglo.stepSizeLocation, 0.0f);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_O) == GLFW_PRESS) {
			glUseProgram(oglo.shaderProgram);
			glUniform1f(oglo.stepSizeLocation, stepSize);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_L) == GLFW_PRESS) {
			glUseProgram(oglo.shaderProgram);
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
			cameraPosition += MOVEMENTDELTA * glm::vec3(cos(cbVars.pitch)*cos(cbVars.yaw), sin(cbVars.pitch), cos(cbVars.pitch)*sin(cbVars.yaw));
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_S) == GLFW_PRESS) {
			cameraPosition -= MOVEMENTDELTA * glm::vec3(cos(cbVars.pitch)*cos(cbVars.yaw), sin(cbVars.pitch), cos(cbVars.pitch)*sin(cbVars.yaw));
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_A) == GLFW_PRESS) {
			cameraPosition += MOVEMENTDELTA * glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(cos(cbVars.pitch)*cos(cbVars.yaw), sin(cbVars.pitch), cos(cbVars.pitch)*sin(cbVars.yaw)));
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_D) == GLFW_PRESS) {
			cameraPosition -= MOVEMENTDELTA * glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(cos(cbVars.pitch)*cos(cbVars.yaw), sin(cbVars.pitch), cos(cbVars.pitch)*sin(cbVars.yaw)));
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_UP) == GLFW_PRESS) {
			theta -= ROTATIONDELTA;
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			theta += ROTATIONDELTA;
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			phi += ROTATIONDELTA;
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}
		if(glfwGetKey(oglo.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			phi -= ROTATIONDELTA;
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
		}

		if(glfwGetKey(oglo.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(oglo.window, 1);
		}

		// this update is triggered by the cursor movement callback
		if(cbVars.updateTransformationUniformsRequired) {
			updateTransformationUniforms(&oglo, &cbVars, theta, phi, xres, yres, cameraPosition);
			cbVars.updateTransformationUniformsRequired = 0;
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

		// update fps counter every second
		if(GetWallTime()-fpsUpdate > 1.0) {
			fpsUpdateFrames = totalFrames-fpsUpdateFrames;
			float fps = (float)fpsUpdateFrames/(GetWallTime()-fpsUpdate);
			sprintf(fpsString, "FPS: %.1f", fps);
			fpsUpdate = GetWallTime();
			fpsUpdateFrames = totalFrames;
		}
		renderText(&oglo, glyphs, fpsString, -1.0f, -1.0f, xres, yres);

		glfwSwapBuffers(oglo.window);
		glfwPollEvents();
		totalFrames++;

		// if manually advancing, set stepSize to zero to halt evolution
		if(updateAttractorOnce) {
			glUseProgram(oglo.shaderProgram);
			glUniform1f(oglo.stepSizeLocation, 0.0f);
			updateAttractorOnce = 0;
		}
	}
	printf("Average fps: %lf\n", totalFrames/(GetWallTime()-startTime));


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



int setupOpenGL(openglObjects *oglo, callbackVariables *cbVars, const unsigned int xres, const unsigned int yres)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef FOROSX
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_SAMPLES, 4);

	//oglo->window = glfwCreateWindow(xres, yres, "attractors", NULL, NULL);
	// For fullscreen:
	oglo->window = glfwCreateWindow(xres, yres, "attractors", glfwGetPrimaryMonitor(), NULL);
	if(oglo->window == NULL) {
		fprintf(stderr, "Error in glfwCreateWindow\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(oglo->window);
	glfwSetInputMode(oglo->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// set initial cursor position
	glfwGetCursorPos(oglo->window, &(cbVars->prevX), &(cbVars->prevY));

	// vsync? 0 disabled, 1 enabled
	glfwSwapInterval(0);
	glfwSetFramebufferSizeCallback(oglo->window, framebufferSizeCallback);
	glfwSetCursorPosCallback(oglo->window, mousePointerCallback);
	glfwSetWindowUserPointer(oglo->window, cbVars);
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


	// shaders and buffers for text
	oglo->vertexShaderText = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(oglo->vertexShaderText, 1, &vertexShaderTextSource, NULL);
	glCompileShader(oglo->vertexShaderText);
	glGetShaderiv(oglo->vertexShaderText, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->vertexShaderText, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in Text vertex shader compilation:\n%s\n", compileLog);
	}

	oglo->fragmentShaderText = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(oglo->fragmentShaderText, 1, &fragmentShaderTextSource, NULL);
	glCompileShader(oglo->fragmentShaderText);
	glGetShaderiv(oglo->fragmentShaderText, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(oglo->fragmentShaderText, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in Text fragment shader compilation:\n%s\n", compileLog);
	}

	oglo->shaderProgramText = glCreateProgram();
	glAttachShader(oglo->shaderProgramText, oglo->vertexShaderText);
	glAttachShader(oglo->shaderProgramText, oglo->fragmentShaderText);

	glLinkProgram(oglo->shaderProgramText);
	glGetProgramiv(oglo->shaderProgramText, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(oglo->shaderProgramText, OGLLOGSIZE, NULL, compileLog);
		fprintf(stderr, "Error in Text program compilation:\n%s\n", compileLog);
	}
	glDeleteShader(oglo->vertexShaderText);
	glDeleteShader(oglo->fragmentShaderText);

	glUseProgram(oglo->shaderProgramText);
	glGenVertexArrays(1, &(oglo->textVAO));
	glBindVertexArray(oglo->textVAO);

	glGenBuffers(1, &(oglo->textVBO));
	glBindBuffer(GL_ARRAY_BUFFER, oglo->textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*MAXTEXTLENGTH, 0, GL_DYNAMIC_DRAW);

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
	callbackVariables *cbVars = (callbackVariables*)glfwGetWindowUserPointer(window);
	float xoffset = xpos - cbVars->prevX;
	float yoffset = ypos - cbVars->prevY;
	cbVars->prevX = xpos;
	cbVars->prevY = ypos;

	xoffset *= MOUSESENSITIVITY;
	yoffset *= MOUSESENSITIVITY;

	cbVars->yaw += xoffset;
	cbVars->pitch += yoffset;

	cbVars->updateTransformationUniformsRequired = 1;
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
	glUseProgram(oglo->shaderProgram);
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



void updateTransformationUniforms(openglObjects *oglo, callbackVariables *cbVars, float theta, float phi, unsigned int xres, unsigned int yres, glm::vec3 cameraPosition)
{
	// rotation matrix, rotate by theta w.r.t. x axis and phi w.r.t. t axis:
	glm::mat4 rotationMatrix = glm::mat4(1.0f);
	rotationMatrix = glm::rotate(rotationMatrix, theta, glm::vec3(1.0f, 0.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, phi, glm::vec3(0.0f, 1.0f, 0.0f));

	// camera projection
	glm::vec3 cameraDirection = glm::vec3(cos(cbVars->pitch)*cos(cbVars->yaw), sin(cbVars->pitch), cos(cbVars->pitch)*sin(cbVars->yaw));
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



void ftLoadGlyphs(openglObjects *oglo, FT_Face ftFace, glyphInfo *glyphs)
{
	FT_GlyphSlot ftGS = ftFace->glyph;

	// Find total width, and maximum height of glyphs
	unsigned int totalWidth = 0;
	unsigned int maxHeight = 0;
	// i = 32: start of drawable characters in ascii table
	for(int i = 32; i < 128; i++) {
		if(FT_Load_Char(ftFace, i, FT_LOAD_RENDER)) {
			fprintf(stderr, "Error loading character %d\n", i);
			continue;
		}
		totalWidth += ftGS->bitmap.width;
		maxHeight = (ftGS->bitmap.rows > maxHeight) ? ftGS->bitmap.rows : maxHeight;
	}
	oglo->fontTexWidth = totalWidth;
	oglo->fontTexHeight = maxHeight;

	// Make a texture for all glyphs
	glUseProgram(oglo->shaderProgramText);
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &(oglo->fontTex));
	glBindTexture(GL_TEXTURE_2D, oglo->fontTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, oglo->fontTexWidth, oglo->fontTexHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	// Copy glyphs to texture, save parameters
	int x = 0;
	for(int i = 32; i < 128; i++) {
		if(FT_Load_Char(ftFace, i, FT_LOAD_RENDER)) {
			printf("Warning: skipping %d in FT_Load_Char\n", i);
			continue;
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, ftGS->bitmap.width, ftGS->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, ftGS->bitmap.buffer);

		glyphs[i].width = ftGS->bitmap.width;
		glyphs[i].rows = ftGS->bitmap.rows;
		glyphs[i].left = ftGS->bitmap_left;
		glyphs[i].top = ftGS->bitmap_top;
		glyphs[i].advancex = (ftGS->advance.x>>6);
		glyphs[i].advancey = (ftGS->advance.y>>6);
		glyphs[i].xTexCoord = (float)x/(float)oglo->fontTexWidth;

		x += ftGS->bitmap.width;
	}
}



void renderText(openglObjects *oglo, glyphInfo *glyphs, std::string text, float posx, float posy, int xres, int yres)
{
	unsigned int nCharacters = text.length();
	float *vertices = (float*)malloc(4 * 6 * nCharacters * sizeof(float));
	for(unsigned int i = 0; i < nCharacters; i++) {
		size_t index = (size_t)text[i];
		float scalex = 1.0f/2048.0f;
		float scaley = scalex * (float)xres/(float(yres));
		float x = posx + glyphs[index].left * scalex;
		float y = posy - (glyphs[index].rows - glyphs[index].top) * scaley;
		float tx0 = (float)glyphs[index].xTexCoord;
		float tx1 = (float)(glyphs[index].xTexCoord)+glyphs[index].width/(float)oglo->fontTexWidth;
		float ty1 = 0.0f;
		float ty0 = (float)glyphs[index].rows/(float)oglo->fontTexHeight;
		float width = glyphs[index].width * scalex;
		float height = glyphs[index].rows * scaley;
		//printf("to draw: %c: %lf %lf  --  %lf %lf  --  %lf %lf\n", text[i], x, y, tx0, tx1, ty0, ty1);
		vertices[i*4*6+0] = x;
		vertices[i*4*6+1] = y + height;
		vertices[i*4*6+2] = tx0;
		vertices[i*4*6+3] = ty1;
		vertices[i*4*6+4] = x;
		vertices[i*4*6+5] = y;
		vertices[i*4*6+6] = tx0;
		vertices[i*4*6+7] = ty0;
		vertices[i*4*6+8] = x + width;
		vertices[i*4*6+9] = y;
		vertices[i*4*6+10] = tx1;
		vertices[i*4*6+11] = ty0;
		vertices[i*4*6+12] = x;
		vertices[i*4*6+13] = y + height;
		vertices[i*4*6+14] = tx0;
		vertices[i*4*6+15] = ty1;
		vertices[i*4*6+16] = x + width;
		vertices[i*4*6+17] = y;
		vertices[i*4*6+18] = tx1;
		vertices[i*4*6+19] = ty0;
		vertices[i*4*6+20] = x + width;
		vertices[i*4*6+21] = y + height;
		vertices[i*4*6+22] = tx1;
		vertices[i*4*6+23] = ty1;
		// advance "cursor"
		posx += glyphs[index].advancex * scalex;
	}

	glUseProgram(oglo->shaderProgramText);
	glBindBuffer(GL_ARRAY_BUFFER, oglo->textVBO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, oglo->fontTex);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*4*6*nCharacters, vertices);

	glDrawArrays(GL_TRIANGLES, 0, 6*nCharacters);

	free(vertices);
}
