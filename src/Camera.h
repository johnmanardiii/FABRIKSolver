#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#include "line.h"
#include "Shape.h"

class camera;
struct PathPoint;

extern int realspeed;


class camera
{
public:
	glm::vec3 pos, rot;
	float pitch, yaw;
	bool firstMouse, pathcam;
	int w, a, s, d, q, e, z, c, t, y, alt;

	const vec3 cameraUp = vec3(0, 1, 0);

	camera()
	{
		w = a = s = d = q = e = z = c = t = y = alt = 0;
		pathcam = 0;
		rot = glm::vec3(0, 0, 0);
		pos = vec3(16.16, -2.74,15.96);
		//rot = vec3(.266f, .768f, 0.0f);
		yaw = 296.f;
		pitch = -29.5f;
		firstMouse = true;
	}
	
	glm::mat4 process(double ftime)
	{
		float speed = 0;

		float fwdspeed = 8;
		if (realspeed)
			fwdspeed = 60;

		if (w == 1)
		{
			speed = fwdspeed * ftime;
		}
		else if (s == 1)
		{
			speed = -fwdspeed * ftime;
		}

		float yangle = 0;
		/*if (a == 1)
			yangle = -3*ftime;
		else if(d==1)
			yangle = 3*ftime;*/
		static float lastYaw;
		static float lastPitch;
		yangle = (yaw - lastYaw) * .008f;

		rot.y += yangle;
		float zangle = 0;
		if (q == 1)
			zangle = -3 * ftime;
		else if (e == 1)
			zangle = 3 * ftime;
		rot.z += zangle;
		float xangle = 0;
		/*if (z == 1)
			xangle = -0.3 * ftime;
		else if (c == 1)
			xangle = 0.3 * ftime;*/
		xangle += (lastPitch - pitch) * .008f;
		rot.x += xangle;

		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed, 1);
		R = Rz * Rx * R;
		dir = dir * R;
		vec3 camForwards = vec4(0, 0, 1, 1) * R;
		pos += glm::vec3(dir.x, dir.y, dir.z);

		// lateral movement:
		vec3 side = normalize(cross(vec3(normalize(camForwards)), cameraUp));
		float sideSpeed = 2 * ftime;
		if (a == 1)
		{
			pos -= side * sideSpeed;
		}
		if (d == 1)
		{
			pos += side * sideSpeed;
		}
		if (z == 1)
		{
			pos += cameraUp * sideSpeed;
		}
		if (c == 1)
		{
			pos -= cameraUp * sideSpeed;
		}
		glm::mat4 T = glm::translate(glm::mat4(1), pos);

		lastYaw = yaw;
		lastPitch = pitch;



		return R * T;
	}
	void get_dirpos(vec3& up, vec3& dir, vec3& position)
	{
		position = -pos;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::vec4 dir4 = glm::vec4(0, 0, 1, 0);
		R = Rz * Rx * R;
		dir4 = dir4 * R;
		dir = vec3(dir4);
		glm::vec4 up4 = glm::vec4(0, 1, 0, 0);
		up4 = R * vec4(0, 1, 0, 0);
		up4 = vec4(0, 1, 0, 0) * R;
		up = vec3(up4);
	}
	/*void get_dirpos(vec3& position, float& roll)
	{
		position = -pos;
		roll = rot.z;
	}*/

};
extern camera mycam;
extern float lastX, lastY;
void mouse_curs_callback(GLFWwindow* window, double xpos, double ypos);

struct PathPoint
{
	vec3 pos, dir, up;
	//float roll;
	PathPoint(vec3 _pos, vec3 _dir, vec3 _up);
};