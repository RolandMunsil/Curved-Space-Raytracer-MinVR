#include <exception>
#include <vector>

#include <glm/glm.hpp>
using namespace glm;

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

void testRotationMethods() {
	//Testing
	float test_epsilon = 0.00001;
	//////////// Rotating A from A to B ////////////
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = vec4(1, 0, 0, 0);

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = vec4(0, 1, 0, 0);

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			vec4 diff = testVec - desiredVec;
			float len = length(testVec - desiredVec);
			throw std::exception();
		}
	}
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = vec4(0, 0, 1, 0);

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = vec4(0, 0, 0, 1);

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = normalize(vec4(4, 3, 2, 1));

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 testVec = vec4(1, 0, 0, 0);
		vec4 desiredVec = normalize(vec4(-3, 0, 10, 0.36));

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 testVec = normalize(vec4(4, 3, 2, 1));
		vec4 desiredVec = normalize(vec4(-3, 0, 10, 0.36));

		rotate4DSinglePlane(testVec, desiredVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
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
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = vec4(0, 1, 0, 0);
		vec4 testVec = vec4(-1, 0, 0, 0);
		vec4 desiredVec = vec4(0, -1, 0, 0);

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = normalize(vec4(1, 1, 0, 0));
		vec4 testVec = vec4(0, 1, 0, 0);
		vec4 desiredVec = normalize(vec4(-1, 1, 0, 0));

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = normalize(vec4(1, 1, 0, 0));
		vec4 testVec = normalize(vec4(1, 1, 0, 0));
		vec4 desiredVec = vec4(0, 1, 0, 0);

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
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
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = vec4(0, 1, 0, 0);
		vec4 testVec = vec4(0, 0, 0, 1);
		vec4 desiredVec = testVec;

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = vec4(0, 1, 0, 0);
		vec4 testVec = vec4(0, 0, 1, 1);
		vec4 desiredVec = testVec;

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = normalize(vec4(0, 1, 1, 0));
		vec4 testVec = vec4(0, 0, 0, 1);
		vec4 desiredVec = testVec;

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = normalize(vec4(0, 1, 1, 0));
		vec4 testVec = vec4(0, 0, 0, -1);
		vec4 desiredVec = testVec;

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
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
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
			throw std::exception();
		}
	}
	{
		vec4 fromVec = vec4(1, 0, 0, 0);
		vec4 toVec = vec4(0, 1, 0, 0);
		vec4 testVec = vec4(-0.3, 0, 0, 0);
		vec4 desiredVec = vec4(0, -0.3, 0, 0);

		rotate4DSinglePlane(fromVec, toVec, { &testVec });
		if (any(isnan(testVec)) || any(isinf(testVec)) || length(testVec - desiredVec) > test_epsilon) {
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
