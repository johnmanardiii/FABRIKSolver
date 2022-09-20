#pragma once
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_float.hpp>


#include "line.h"

class Arm
{
public:
	vec3 Mount, P1, P2, P3, goal;
	float L1, L2, L3;

	vec3 nP1, nP2, nP3;
	Line linerender;
	glm::vec3 baseColor;


	Arm(glm::vec3 mount, vec3 p1, vec3 p2, vec3 p3)
	{
		Mount = mount;
		P1 = p1;
		P2 = p2;
		P3 = p3;
		L1 = distance(mount, p1);
		L2 = distance(p1, p2);
		L3 = distance(p2, p3);
		goal = P3;
		linerender.init();
		baseColor = vec3(1, .5, 0);
	}

	void init(Arm other);
	Arm() = default;

	void draw(mat4 P, mat4 V)
	{
		vector<vec3> armPoints;
		armPoints.push_back(Mount);
		armPoints.push_back(P1);
		armPoints.push_back(P2);
		armPoints.push_back(P3);
		linerender.re_init_line(armPoints);
		linerender.init();
		linerender.draw(P, V, baseColor);
	}

	Arm FabrikSolve(vec3 endPoint);

};

Arm linearInterpolateBetweenArms(const Arm& a1, const Arm& a2, float t);
