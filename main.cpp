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
#include <glm/gtx/projection.hpp>
#include <glm/gtx/rotate_vector.hpp>
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



			//Testing
			float test_epsilon = 0.00001;
			//////////// Rotating A from A to B ////////////
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = vec4(1, 0, 0, 0);

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = vec4(0, 1, 0, 0);

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					vec4 diff = testVec - desiredVec;
					float len = length(testVec - desiredVec);
					throw std::exception();
				}
			}
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = vec4(0, 0, 1, 0);

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = vec4(0, 0, 0, 1);

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(4, 3, 2, 1));

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(-3, 0, 10, 0.36));

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 testVec = normalize(vec4(4, 3, 2, 1));
				vec4 desiredVec = normalize(vec4(-3, 0, 10, 0.36));

				rotate4DSinglePlane(testVec, desiredVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}

			//////////// Rotate vectors in plane ////////////
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 1, 0, 0);
				vec4 desiredVec = vec4(-1, 0, 0, 0);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(-1, 0, 0, 0);
				vec4 desiredVec = vec4(0, -1, 0, 0);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = normalize(vec4(1, 1, 0, 0));
				vec4 testVec = vec4(0, 1, 0, 0);
				vec4 desiredVec = normalize(vec4(-1, 1, 0, 0));

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = normalize(vec4(1, 1, 0, 0));
				vec4 testVec = normalize(vec4(1, 1, 0, 0));
				vec4 desiredVec = vec4(0, 1, 0, 0);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}

			//////////// Do NOT rotate vectors orthogonal ////////////
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 0, 1, 0);
				vec4 desiredVec = testVec;

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 0, 0, 1);
				vec4 desiredVec = testVec;

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 0, 1, 1);
				vec4 desiredVec = testVec;

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = normalize(vec4(0, 1, 1, 0));
				vec4 testVec = vec4(0, 0, 0, 1);
				vec4 desiredVec = testVec;

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = normalize(vec4(0, 1, 1, 0));
				vec4 testVec = vec4(0, 0, 0, -1);
				vec4 desiredVec = testVec;

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}

			//////////// Non-normal vectors are still non-normal ////////////
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 2, 0, 0);
				vec4 desiredVec = vec4(-2, 0, 0, 0);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(-0.3, 0, 0, 0);
				vec4 desiredVec = vec4(0, -0.3, 0, 0);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec)> test_epsilon) {
					throw std::exception();
				}
			}

			//////////// Rotate vectors not orthogonal but not in plane ////////////
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 1, 0, 1);
				vec4 desiredVec = vec4(-1, 0, 0, 1);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}

			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				vec4 testVec = vec4(0, 1, 2, 1);
				vec4 desiredVec = vec4(-1, 0, 2, 1);

				rotate4DSinglePlane(fromVec, toVec, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}

			//////////// With specific angle ////////////

			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				float angle = radians(45.0);
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(1, 1, 0, 0));

				rotate4DSinglePlaneSpecificAngle(fromVec, toVec, angle, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				float angle = radians(-45.0);
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(1, -1, 0, 0));

				rotate4DSinglePlaneSpecificAngle(fromVec, toVec, angle, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				float angle = radians(-90.0);
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(0, -1, 0, 0));

				rotate4DSinglePlaneSpecificAngle(fromVec, toVec, angle, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}
			{
				vec4 fromVec = vec4(1, 0, 0, 0);
				vec4 toVec = vec4(0, 1, 0, 0);
				float angle = radians(30.0);
				vec4 testVec = vec4(1, 0, 0, 0);
				vec4 desiredVec = normalize(vec4(cos(radians(30.0)), 0.5, 0, 0));

				rotate4DSinglePlaneSpecificAngle(fromVec, toVec, angle, { &testVec });
				if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
					throw std::exception();
				}
			}

        }
    }
    
	void onRenderGraphicsScene(const VRGraphicsState& state) {
		// clear screen
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		int windowId = state.getWindowId();
		if (((VRString)state.index().getValue("Eye")) == "Left") {
			// So that left and right eye will get treated separately
			windowId += 0xF0;
		}

		mat4 curViewMatrix = make_mat4(state.getViewMatrix());

		if (state.isInitialRenderCall()) {
			CameraInfo inf = { mat4(1.0), vec4(0,1,0,0), vec4(1,0,0,0), vec4(0,0,0,1), vec4(0,0,1,0) };
			cameraInfos[windowId] = inf;
		}

		CameraInfo* thisCameraInfo = &cameraInfos.at(windowId);

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


		float user_scale = 1;

		// Move position in virtual world
		vec4 moveDirection = normalize(
			(thisCameraInfo->rightDir * changeMat_translation.x) +
			(thisCameraInfo->upDir * changeMat_translation.y) +
			(thisCameraInfo->forwardDir * changeMat_translation.z));
		float moveAmount = user_scale * length(changeMat_translation);
	
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

		//vec4 rotationDirection = normalize(
		//	(thisCameraInfo->rightDir * rotatedForwardVector.x) +
		//	(thisCameraInfo->upDir * rotatedForwardVector.y) +
		//	(thisCameraInfo->forwardDir * rotatedForwardVector.z));

		//rotate4DSinglePlane(thisCameraInfo->forwardDir, rotationDirection,
		//	{ &thisCameraInfo->rightDir, &thisCameraInfo->upDir, &thisCameraInfo->forwardDir });

		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->forwardDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->upDir)) > 0.0001) {
			throw std::exception();
		}
		if (abs(dot(thisCameraInfo->pos, thisCameraInfo->rightDir)) > 0.0001) {
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

		glDrawElements(GL_TRIANGLE_STRIP, _numIndices, GL_UNSIGNED_INT, 0);

	}
    
    
    void onRenderHaptics(const VRHapticsState& state) {}

	void rotate4DSinglePlane(vec4 fromVector, vec4 toVector, std::vector<vec4*> vectorsToRotate) {
		if (abs(length(fromVector) - 1) > 0.0001) {
			throw std::exception();
		}
		if (abs(length(toVector) - 1) > 0.0001) {
			throw std::exception();
		}

		fromVector = normalize(fromVector);
		toVector = normalize(toVector);

		// Clamp for cases where a vector is *slightly* longer than 1
		float rotationAngle = acos(clamp(dot(fromVector, toVector), -1.0f, 1.0f));
		rotate4DSinglePlaneSpecificAngle(fromVector, toVector, rotationAngle, vectorsToRotate);
	}

	void rotate4DSinglePlaneSpecificAngle(vec4 fromVector, vec4 toVector, float angle, std::vector<vec4*> vectorsToRotate) {
		if (abs(length(fromVector) - 1) > 0.0001) {
			throw std::exception();
		}
		if (abs(length(toVector) - 1) > 0.0001) {
			throw std::exception();
		}
		
		fromVector = normalize(fromVector);
		toVector = normalize(toVector);

		float rotationAngle = angle;

		if (abs(rotationAngle) < radians(0.00001)) {
			//from and to are likely the same, so just return
			return;
		}

		vec4 non_norm_proj = toVector - proj(toVector, fromVector);
		if (length(non_norm_proj) < 0.00001) {
			//from and to are likely the same, so just return
			return;
		}
		vec4 perp_toVector = normalize(non_norm_proj);

		if (any(isnan(perp_toVector))) {
			throw std::exception();
		}
		if (abs(dot(perp_toVector, fromVector)) > 0.0001) {
			float f = dot(perp_toVector, fromVector);
			throw std::exception();
		}
		if (abs(length(perp_toVector) - 1) > 0.0001) {
			float f = length(perp_toVector);
			throw std::exception();
		}

		for (vec4* p_v : vectorsToRotate) {
			vec4 orig_vec = *p_v;

			float to_scalar_component = dot(orig_vec, perp_toVector);
			float from_scalar_component = dot(orig_vec, fromVector);

			vec4 ortho_component = orig_vec - (from_scalar_component*fromVector + to_scalar_component * perp_toVector);

			if (abs(dot(ortho_component, fromVector)) > 0.0001) {
				throw std::exception();
			}
			if (abs(dot(ortho_component, perp_toVector)) > 0.0001) {
				throw std::exception();
			}

			vec2 combined_components = vec2(from_scalar_component, to_scalar_component);

			// Note: rotates CCW (pos x-axis -> pos y-axis)
			vec2 rotated = rotate(combined_components, rotationAngle);

			vec4 final_result = ortho_component + (rotated.x*fromVector + rotated.y * perp_toVector);

			if (abs(length(final_result) - length(orig_vec)) > 0.0001) {
				throw std::exception();
			}

			*p_v = final_result;
		}
	}
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
