#include "Arm.h"

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

float cosinterpolation(float t, float pctLinear)
{
	float ct = (1.f - (cos(t * 3.1415926535f) + 1.f) / 2.0f);
	return ct * (1.0 - pctLinear) + t * pctLinear;
}

mat4 linint_between_two_orientations(glm::vec3 ez_aka_lookto_1, glm::vec3 ey_aka_up_1, glm::vec3 ez_aka_lookto_2,
	glm::vec3 ey_aka_up_2, float t)
{
	t = cosinterpolation(t, .5f);
	glm::mat4 m1, m2;
	glm::quat q1, q2;
	glm::vec3 ex, ey, ez;
	ey = ey_aka_up_1;
	ez = ez_aka_lookto_1;
	ex = cross(ey, ez);
	m1[0][0] = ex.x;		m1[0][1] = ex.y;		m1[0][2] = ex.z;		m1[0][3] = 0;
	m1[1][0] = ey.x;		m1[1][1] = ey.y;		m1[1][2] = ey.z;		m1[1][3] = 0;
	m1[2][0] = ez.x;		m1[2][1] = ez.y;		m1[2][2] = ez.z;		m1[2][3] = 0;
	m1[3][0] = 0;			m1[3][1] = 0;			m1[3][2] = 0;			m1[3][3] = 1.0f;
	ey = ey_aka_up_2;
	ez = ez_aka_lookto_2;
	ex = cross(ey, ez);
	m2[0][0] = ex.x;		m2[0][1] = ex.y;		m2[0][2] = ex.z;		m2[0][3] = 0;
	m2[1][0] = ey.x;		m2[1][1] = ey.y;		m2[1][2] = ey.z;		m2[1][3] = 0;
	m2[2][0] = ez.x;		m2[2][1] = ez.y;		m2[2][2] = ez.z;		m2[2][3] = 0;
	m2[3][0] = 0;			m2[3][1] = 0;			m2[3][2] = 0;			m2[3][3] = 1.0f;
	q1 = glm::quat(m1);
	q2 = glm::quat(m2);
	glm::quat qt = glm::slerp(q1, q2, t); //<---
	qt = glm::normalize(qt);
	glm::mat4 mt = glm::mat4(qt);
	//mt = transpose(mt);		 //<---
	return mt;
}

quat calculate_arm_orientation(vec3 p1, vec3 p2)
{
	vec3 ez = normalize(p2 - p1);
	vec3 ey = vec3(0, 1, 0);
	ey = ey - ((dot(ey, ez) / dot(ez, ez)) * ez);
	ey = normalize(ey);
	vec3 ex = cross(ey, ez);

	mat4 m1 = mat4(1);
	
	m1[0][0] = ex.x;		m1[0][1] = ex.y;		m1[0][2] = ex.z;		m1[0][3] = 0;
	m1[1][0] = ey.x;		m1[1][1] = ey.y;		m1[1][2] = ey.z;		m1[1][3] = 0;
	m1[2][0] = ez.x;		m1[2][1] = ez.y;		m1[2][2] = ez.z;		m1[2][3] = 0;
	m1[3][0] = 0;			m1[3][1] = 0;			m1[3][2] = 0;			m1[3][3] = 1.0f;
	vec4 rotated_point = m1 * vec4(0, 0, 1, 0);
	vec3 sub = p2 - p1;
	return quat(m1);
}

void calculate_arm_quats(const Arm &a, quat &q1, quat &q2, quat &q3)
{
	q1 = calculate_arm_orientation(a.Mount, a.P1);
	q2 = calculate_arm_orientation(a.P1, a.P2);
	q3 = calculate_arm_orientation(a.P2, a.P3);
}

vec3 get_arm_vector(float length, quat arm_quat)
{
	vec4 initial_orientation = vec4(0, 0, 1, 0);
	vec4 rotated = mat4(arm_quat) * initial_orientation;
	return vec3(rotated) * length;
}

Arm linearInterpolateBetweenArms(const Arm &a1, const Arm &a2, float t)
{

	// t = cosinterpolation(t, .5f);
	
	quat a1q1, a1q2, a1q3;
	quat a2q1, a2q2, a2q3;

	calculate_arm_quats(a1, a1q1, a1q2, a1q3);
	calculate_arm_quats(a2, a2q1, a2q2, a2q3);

	quat l1quat, l2quat, l3quat;

	l1quat = slerp(a1q1, a2q1, t);
	l2quat = slerp(a1q2, a2q2, t);
	l3quat = slerp(a1q3, a2q3, t);

	// now reform the arm :/
	vec3 Mount, P1, P2, P3;
	Mount = a1.Mount;
	P1 = Mount + get_arm_vector(a1.L1, l1quat);
	P2 = P1 + get_arm_vector(a1.L2, l2quat);
	P3 = P2 + get_arm_vector(a1.L3, l3quat);
	
	return Arm{Mount, P1, P2, P3};
}


void Arm::init(const Arm other)
{
	// basically a copy constructor because idk how to use cpp very well lol
	Mount = other.Mount;
	P1 = other.P1;
	P2 = other.P2;
	P3 = other.P3;
	L1 = other.L1;
	L2 = other.L2;
	L3 = other.L3;
	goal = P3;
	linerender.init();
	baseColor = other.baseColor;
}

void calculateFromEnd(Arm &arm, vec3 endpoint)
{
	// calculated FABRIK from the end
	// set P3 to endpoint
	arm.P3 = endpoint;
	// set p2 to the point L3 away from P3 in the direction of old p2
	vec3 dir = normalize(arm.P3 - arm.P2) * arm.L3;
	arm.P2 = arm.P3 + dir;
	dir = normalize(arm.P2 - arm.P1) * arm.L2;
	arm.P1 = arm.P2 + dir;
	dir = normalize(arm.P1 - arm.Mount) * arm.L1;
	arm.Mount = arm.P1 + dir;
}

void calculateFromStart(Arm& arm, vec3 o_mount)
{
	// calculated FABRIK from the start
	arm.Mount = o_mount;
	vec3 dir = normalize(arm.P1 - arm.Mount) * arm.L1;
	arm.P1 = arm.Mount + dir;
	dir = normalize(arm.P2 - arm.P1) * arm.L2;
	arm.P2 = arm.P1 + dir;
	dir = normalize(arm.P3 - arm.P2) * arm.L3;
	arm.P3 = arm.P2 + dir;
}

Arm Arm::FabrikSolve(vec3 endPoint)
{
	Arm a;
	a.init(*this);	// a is now a copy of this arm (we don't want to modify ourselves, we want a new arm representing the final pos)
	const float min_dist = .005f;	// distance to see if close enough
	// test if we can even reach it
	float testDist = distance(Mount, endPoint);
	if (testDist > (L1 + L2 + L3))
	{
		cout << "im sowwy, i can't gwab the point u asked for D:" << endl;
		return Arm(Mount, P1, P2, P3);
	}
	vec3 original_mount = Mount;

	int iterations = 0;
	while(true)
	{
		iterations++;
		if(distance(a.P3, endPoint) < min_dist)
		{
			break;	// going backwards and mount ends up pretty close to the original mount 
		}

		calculateFromEnd(a, endPoint);	// backwards
		calculateFromStart(a, original_mount);	// forwards

		// after iteration, check if close enough to call it quits:
		// if terminated, startFromEnd will contain the value last used for an iteration (for mounting point shifting purposes)
		if(iterations > 100)
		{
			break;
		}
	}

	/*if(startFromEnd)
	{
		// we ended off starting from the end, so make the mount point the real mount point
		vec3 diff = a.Mount - original_mount;
		a.Mount -= diff;
		a.P1 -= diff;
		a.P2 -= diff;
		a.P3 -= diff;
	}*/
	
	return a;
}