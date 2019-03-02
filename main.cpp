#include <iostream>

#ifdef _WIN32
#include "GL/glew.h"
#include "GL/wglew.h"
#elif (!defined(__APPLE__))
#include "GL/glxew.h"
#endif

// OpenGL Headers
#if defined(WIN32)
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

// MinVR header
#include <api/MinVR.h>
using namespace MinVR;

// Just included for some simple Matrix math used below
// This is not required for use of MinVR in general
#include <math/VRMath.h>

struct vec3 {
	float x;
	float y;
	float z;
};

struct vec4 {
	float x;
	float y;
	float z;
	float w;
};

/**
 * MyVRApp is an example of a modern OpenGL using VBOs, VAOs, and shaders.  MyVRApp inherits
 * from VRGraphicsApp, which allows you to override onVREvent to get input events, onRenderContext
 * to setup context sepecific objects, and onRenderScene that renders to each viewport.
 */
class MyVRApp : public VRApp {
public:
    MyVRApp(int argc, char** argv) : VRApp(argc, argv) {
    }


    void onAnalogChange(const VRAnalogEvent &state) {}
    
    void onButtonDown(const VRButtonEvent &state) {
        if (state.getName() == "KbdEsc_Down") {
            shutdown();
        }
    }
    
    void onButtonUp(const VRButtonEvent &state) {}
    
    void onCursorMove(const VRCursorEvent &state) {}
    
    void onTrackerMove(const VRTrackerEvent &state) {}

    
    
    void onRenderAudio(const VRAudioState& state) {}
    
    
    void onRenderConsole(const VRConsoleState& state) {}

    
    void onRenderGraphicsContext(const VRGraphicsState& state) {
        // If this is the inital call, initialize context variables
		if (state.isInitialRenderCall()) {
#ifndef __APPLE__
			glewExperimental = GL_TRUE;
			GLenum err = glewInit();
			if (GLEW_OK != err)
			{
				std::cout << "Error initializing GLEW." << std::endl;
			}
#endif
			// Init GL
			glClearColor(0, 0, 0, 0);

			std::vector<vec3> cpuVertexArray;
			std::vector<int> cpuIndexArray;

			cpuVertexArray.push_back({ -1, -1, 0 });
			cpuVertexArray.push_back({ 1, -1, 0 });
			cpuVertexArray.push_back({ -1, 1, 0 });
			cpuVertexArray.push_back({ 1, 1, 0 });

			cpuIndexArray.push_back(0);
			cpuIndexArray.push_back(1);
			cpuIndexArray.push_back(2);
			cpuIndexArray.push_back(3);

			const int cpuVertexByteSize = sizeof(float[3]) * cpuVertexArray.size();
			const int cpuIndexByteSize = sizeof(int) * cpuIndexArray.size();
			_numIndices = cpuIndexArray.size();

			glGenVertexArrays(1, &_vaoID);
			glBindVertexArray(_vaoID);

			// create the vbo
			glGenBuffers(1, &_vertexVBO);
			glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);

			// initialize size
			glBufferData(GL_ARRAY_BUFFER, cpuVertexByteSize, NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, cpuVertexByteSize, &cpuVertexArray[0]);

			// set up vertex attributes
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), (void*)0);

			// Create indexstream
			glGenBuffers(1, &_indexVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVBO);

			// copy data into the buffer object
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, cpuIndexByteSize, NULL, GL_STATIC_DRAW);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, cpuIndexByteSize, &cpuIndexArray[0]);
            
            // Create shaders
			GLuint vshader = loadAndCompileShader("../../bin/shader.vert", GL_VERTEX_SHADER);
			GLuint fshader = loadAndCompileShader("../../bin/shader.frag", GL_FRAGMENT_SHADER);
            
            // Create shader program
			_programHandle = glCreateProgram();
            glAttachShader(_programHandle, vshader);
            glAttachShader(_programHandle, fshader);
            linkShaderProgram(_programHandle);
			glUseProgram(_programHandle);

			iResolutionLocation = glGetUniformLocation(_programHandle, "iResolution");
			iTimeLocation = glGetUniformLocation(_programHandle, "iTime");
			iMouseLocation = glGetUniformLocation(_programHandle, "iMouse");
			matrixTransformLocation = glGetUniformLocation(_programHandle, "matrixTransform");

			userPosLocation = glGetUniformLocation(_programHandle, "userPos");
			userForwardDirLocation = glGetUniformLocation(_programHandle, "userForwardDir");
			userUpDirLocation = glGetUniformLocation(_programHandle, "userUpDir");
			userRightDirLocation = glGetUniformLocation(_programHandle, "userRightDir");
        }
    }
    
    void onRenderGraphicsScene(const VRGraphicsState& state) {
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glUniform3f(iResolutionLocation, 100, 100, 0);
		glUniform1f(iTimeLocation, 0);
		glUniform4f(iMouseLocation, 0, 0, 0, 0);

		glUniform4f(userPosLocation, userPos.x, userPos.y, userPos.z, userPos.w);
		glUniform4f(userForwardDirLocation, userForwardDir.x, userForwardDir.y, userForwardDir.z, userForwardDir.w);
		glUniform4f(userUpDirLocation, userUpDir.x, userUpDir.y, userUpDir.z, userUpDir.w);
		glUniform4f(userRightDirLocation, userRightDir.x, userRightDir.y, userRightDir.z, userRightDir.w);

		glDrawElements(GL_TRIANGLE_STRIP, _numIndices, GL_UNSIGNED_INT, 0);
        
    }
    
    
    void onRenderHaptics(const VRHapticsState& state) {}
    
    
	GLuint loadAndCompileShader(const std::string& pathToFile, GLuint shaderType) {
		std::ifstream inFile(pathToFile, std::ios::in);
		if (!inFile) {
			throw std::runtime_error("could not load file");
		}

		// Get file contents
		std::stringstream code;
		code << inFile.rdbuf();
		inFile.close();

		std::string asString = code.str();
		return compileShader(asString, shaderType);
	}

	/// Compiles shader
	GLuint compileShader(const std::string& shaderText, GLuint shaderType) {
		const char* source = shaderText.c_str();
		int length = (int)shaderText.size();
		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &source, &length);
		glCompileShader(shader);
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if(status == GL_FALSE) {
			GLint length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> log(length);
			glGetShaderInfoLog(shader, length, &length, &log[0]);
			std::cerr << &log[0];
		}

		return shader;
	}

	/// links shader program
	void linkShaderProgram(GLuint shaderProgram) {
		glLinkProgram(shaderProgram);
		GLint status;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
		if(status == GL_FALSE) {
			GLint length;
			glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> log(length);
			glGetProgramInfoLog(shaderProgram, length, &length, &log[0]);
			std::cerr << "Error compiling program: " << &log[0] << std::endl;
		}
	}
    

private:
	GLuint _vaoID;
	GLuint _vertexVBO;
	GLuint _indexVBO;
	GLsizei _numIndices;
	GLuint _programHandle;

	vec4 userPos = { 0, 1, 0, 0 };
	vec4 userForwardDir = { 1, 0, 0, 0 };
	vec4 userUpDir = { 0, 0, 0, 1 };
	vec4 userRightDir = { 0, 0, 1, 0 };

	float movementSpeed = 0.04;
	float lookAroundSensitivity = 0.003;

	GLint iResolutionLocation;
	GLint iTimeLocation;
	GLint iMouseLocation;
	GLint matrixTransformLocation;
	GLint userPosLocation;
	GLint userForwardDirLocation;
	GLint userUpDirLocation;
	GLint userRightDirLocation;
};



/// Main method which creates and calls application
int main(int argc, char **argv) {
	MyVRApp app(argc, argv);
	app.run();
    app.shutdown();
	return 0;
}
