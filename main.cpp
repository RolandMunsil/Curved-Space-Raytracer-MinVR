#include <iostream>
#include <mutex>  // For std::unique_lock
#include <shared_mutex>

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

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/rotate_vector.hpp>
using namespace glm;

#include "VRMultithreadedApp.h"
#include "4DUtils.h"

struct CameraInfo {
	mat4 previousRealWorldViewMatrix;

	vec4 pos;
	vec4 forwardDir;
	vec4 upDir;
	vec4 rightDir;
};

/**
 * MyVRApp is an example of a modern OpenGL using VBOs, VAOs, and shaders.  MyVRApp inherits
 * from VRGraphicsApp, which allows you to override onVREvent to get input events, onRenderContext
 * to setup context sepecific objects, and onRenderScene that renders to each viewport.
 */
class MyVRApp : public VRMultithreadedApp {
public:
    MyVRApp(int argc, char** argv) : VRMultithreadedApp(argc, argv) {
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

	void updateWorld(double currentTime) {
		//std::cout << "test";
	}

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
			GLuint vshader = loadAndCompileShader("shader.vert", GL_VERTEX_SHADER);
			GLuint fshader = loadAndCompileShader("shader.frag", GL_FRAGMENT_SHADER);
            
            // Create shader program
			_programHandle = glCreateProgram();
            glAttachShader(_programHandle, vshader);
            glAttachShader(_programHandle, fshader);
            linkShaderProgram(_programHandle);
			glUseProgram(_programHandle);

			viewportResolutionLocation = glGetUniformLocation(_programHandle, "viewportResolution");
			projectionMatLocation = glGetUniformLocation(_programHandle, "projectionMat");

			userPosLocation = glGetUniformLocation(_programHandle, "userPos");
			userForwardDirLocation = glGetUniformLocation(_programHandle, "userForwardDir");
			userUpDirLocation = glGetUniformLocation(_programHandle, "userUpDir");
			userRightDirLocation = glGetUniformLocation(_programHandle, "userRightDir");

			testRotationMethods();
        }
    }
    
	void onRenderGraphicsScene(const VRGraphicsState& state) {
		int windowId = state.getWindowId();
		if (((VRString)state.index().getValue("Eye")) == "Left") {
			// So that left and right eye will get treated separately
			windowId += 0x100;
		}

		std::unique_lock<std::shared_mutex> lock(map_mutex);
		if (state.isInitialRenderCall()) {
			CameraInfo inf = { mat4(1.0), vec4(0,1,0,0), vec4(1,0,0,0), vec4(0,0,0,1), vec4(0,0,1,0) };
			cameraInfos[windowId] = inf;
		}

		CameraInfo* thisCameraInfo = &cameraInfos.at(windowId);
		lock.unlock();

		//changeMatrix is a view matrix from the old matrix to the new one
		mat4 curViewMatrix = make_mat4(state.getViewMatrix());
		mat4 changeMatrix = curViewMatrix * inverse(thisCameraInfo->previousRealWorldViewMatrix);

		quat changeMat_rotation;
		vec3 changeMat_translation;
		glm::decompose(changeMatrix, vec3(0), changeMat_rotation, changeMat_translation, vec3(0), vec4(0));



		// Move position in virtual world
		vec4 moveDirection = normalize(
			(thisCameraInfo->rightDir * changeMat_translation.x) +
			(thisCameraInfo->upDir * changeMat_translation.y) +
			(thisCameraInfo->forwardDir * changeMat_translation.z));
		float moveAmount = USER_SCALE * length(changeMat_translation);
	
		rotate4DSinglePlaneSpecificAngle(thisCameraInfo->pos, moveDirection, moveAmount,
			{ &thisCameraInfo->pos, &thisCameraInfo->rightDir, &thisCameraInfo->upDir, &thisCameraInfo->forwardDir });

		// Rotate view in virtual world
		vec3 rotatedRightVector = changeMat_rotation * vec3(1, 0, 0);
		vec3 rotatedUpVector = changeMat_rotation * vec3(0, 1, 0);
		vec3 rotatedForwardVector = changeMat_rotation * vec3(0, 0, 1);

		mat3x4 matWithDirsAsBases(thisCameraInfo->rightDir, thisCameraInfo->upDir, thisCameraInfo->forwardDir);

		thisCameraInfo->rightDir = matWithDirsAsBases * rotatedRightVector;
		thisCameraInfo->upDir = matWithDirsAsBases * rotatedUpVector;
		thisCameraInfo->forwardDir = matWithDirsAsBases * rotatedForwardVector;



		// Do some checks to make sure rotation and position didn't mess anything up
		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->forwardDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->upDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->rightDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->rightDir, thisCameraInfo->upDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->upDir, thisCameraInfo->forwardDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->forwardDir, thisCameraInfo->rightDir)) > 0.0001) {
			throw std::exception();
		}

		// Update camera info
		thisCameraInfo->previousRealWorldViewMatrix = curViewMatrix;

		// Setup uniforms
		GLfloat windowHeight = state.index().getValue("FramebufferHeight");
		GLfloat windowWidth = state.index().getValue("FramebufferWidth");
		glUniform2f(viewportResolutionLocation, windowWidth, windowHeight);

		mat4 projectionMat = make_mat4(state.getProjectionMatrix());
		setUniform(projectionMatLocation, projectionMat, GL_FALSE);

		setUniform(userPosLocation, thisCameraInfo->pos);
		setUniform(userForwardDirLocation, thisCameraInfo->forwardDir);
		setUniform(userUpDirLocation, thisCameraInfo->upDir);
		setUniform(userRightDirLocation, thisCameraInfo->rightDir);

		// Render
		glDrawElements(GL_TRIANGLE_STRIP, _numIndices, GL_UNSIGNED_INT, 0);

	}
    
    
    void onRenderHaptics(const VRHapticsState& state) {}

	void setUniform(GLint location, glm::mat4& matrix, GLboolean transpose) {
		glUniformMatrix4fv(location, 1, transpose, &(matrix[0][0]));
	}

	void setUniform(GLint location, vec4 vector) {
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
	}
    
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

	void moveInDir(vec4& position, vec4& direction, float angle)
	{
		vec4 newPosition = (cos(angle) * position) + (sin(angle) * direction);
		vec4 newDirection = (cos(angle) * direction) + (sin(angle) * -position);

		position = newPosition;
		direction = newDirection;
	}
    

private:
	GLuint _vaoID;
	GLuint _vertexVBO;
	GLuint _indexVBO;
	GLsizei _numIndices;
	GLuint _programHandle;

	mutable std::shared_mutex map_mutex;

	std::map<int, CameraInfo> cameraInfos = std::map<int, CameraInfo>();

	GLint viewportResolutionLocation;
	GLint projectionMatLocation;
	GLint userPosLocation;
	GLint userForwardDirLocation;
	GLint userUpDirLocation;
	GLint userRightDirLocation;

	float USER_SCALE = 1;
};

/// Main method which creates and calls application
int main(int argc, char **argv) {
	MyVRApp app(argc, argv);
	app.run();
    app.shutdown();
	return 0;
}
