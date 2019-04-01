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

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

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

			viewportResolutionLocation = glGetUniformLocation(_programHandle, "viewportResolution");
			projectionMatLocation = glGetUniformLocation(_programHandle, "projectionMat");

			userPosLocation = glGetUniformLocation(_programHandle, "userPos");
			userForwardDirLocation = glGetUniformLocation(_programHandle, "userForwardDir");
			userUpDirLocation = glGetUniformLocation(_programHandle, "userUpDir");
			userRightDirLocation = glGetUniformLocation(_programHandle, "userRightDir");
        }

		/*
		if (keyDown(GLFW_KEY_W)) {
			moveInDir(userPos, userForwardDir, movementSpeed);
		}
		if (keyDown(GLFW_KEY_S)) {
			moveInDir(userPos, userForwardDir, -movementSpeed);
		}

		if (keyDown(GLFW_KEY_A)) {
			moveInDir(userPos, userRightDir, -movementSpeed);
		}
		if (keyDown(GLFW_KEY_D)) {
			moveInDir(userPos, userRightDir, movementSpeed);
		}

		vec2 cursorDiff = getCursorDiff();
		if (cursorDiff.x != 0) {
			if (mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT) || mouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
				moveInDir(userUpDir, userRightDir, cursorDiff.x * lookAroundSensitivity);
			}
			else {
				moveInDir(userForwardDir, userRightDir, cursorDiff.x * lookAroundSensitivity);
			}

		}

		if (cursorDiff.y != 0) {
			if (mouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT) || mouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
				//Do nothing so that when the button is held down the user can't also look up/down
			}
			else {
				moveInDir(userForwardDir, userUpDir, -cursorDiff.y * lookAroundSensitivity);
			}
		}
		*/
    }
    
    void onRenderGraphicsScene(const VRGraphicsState& state) {
        // clear screen
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		mat4 curViewMatrix = make_mat4(state.getViewMatrix());

		if (state.isInitialRenderCall()) {
			CameraInfo inf = { mat4(1.0), vec4(0,1,0,0), vec4(1,0,0,0), vec4(0,0,0,1), vec4(0,0,1,0) };
			cameraInfos[state.getWindowId()] = inf;
		}

		CameraInfo* thisCameraInfo = &cameraInfos.at(state.getWindowId());

		/*
		D * O * p = N * p
		D * O = N
		D = N * O^-1
		*/
		//changeMatrix is a view matrix from the old matrix to the new one
		mat4 changeMatrix = curViewMatrix * inverse(thisCameraInfo->previousRealWorldViewMatrix);


		vec3 changeMat_scale;
		quat changeMat_rotation;
		vec3 changeMat_translation;
		vec3 changeMat_skew;
		vec4 changeMat_perspective;
		glm::decompose(changeMatrix, changeMat_scale, changeMat_rotation, changeMat_translation, changeMat_skew, changeMat_perspective);

		//// http://www.euclideanspace.com/maths/geometry/rotations/theory/nDimensions/index.htm
		//// http://marctenbosch.com/news/2011/05/4d-rotations-and-the-4d-equivalent-of-quaternions/
		//// https://en.wikipedia.org/wiki/Plane_of_rotation
		//// https://www.youtube.com/watch?v=PNlgMPzj-7Q&list=PLpzmRsG7u_gqaTo_vEseQ7U8KFvtiJY4K
		//// http://wscg.zcu.cz/wscg2004/Papers_2004_Short/N29.pdf
		//vec4 rotDir = normalize((thisCameraInfo->rightDir * posRelativeToPreviousPos.x) +
		//	(thisCameraInfo->upDir * posRelativeToPreviousPos.y) +
		//	(thisCameraInfo->forwardDir * posRelativeToPreviousPos.z));
		//float rotAmnt = length(posRelativeToPreviousPos);

		// Move position in virtual world
		moveInDir(thisCameraInfo->pos, thisCameraInfo->rightDir, changeMat_translation.x);
		moveInDir(thisCameraInfo->pos, thisCameraInfo->upDir, changeMat_translation.y);
		moveInDir(thisCameraInfo->pos, thisCameraInfo->forwardDir, changeMat_translation.z);

		// Rotate in virtual world
		vec3 changeMat_eulerAngles = eulerAngles(changeMat_rotation);
		moveInDir(thisCameraInfo->rightDir, thisCameraInfo->upDir, changeMat_eulerAngles.z);
		moveInDir(thisCameraInfo->forwardDir, thisCameraInfo->rightDir, changeMat_eulerAngles.y);
		moveInDir(thisCameraInfo->upDir, thisCameraInfo->forwardDir, changeMat_eulerAngles.x);

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

	std::map<int, CameraInfo> cameraInfos = std::map<int, CameraInfo>();

	GLint viewportResolutionLocation;
	GLint projectionMatLocation;
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
