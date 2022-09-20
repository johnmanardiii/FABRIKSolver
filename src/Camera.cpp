#include "Camera.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include "Program.h"
#include "line.h"

int realspeed = 0;


void convertToPoints(const vector<PathPoint>& path, vector<vec3>& toBeFilled)
{
	for (int i = 0; i < path.size(); i++)
	{
		toBeFilled.push_back(path[i].pos);
	}
}

std::vector<std::string> split(const std::string str, char delim)
{
	std::vector<std::string> result;
	std::istringstream ss{ str };
	std::string token;
	while (std::getline(ss, token, delim)) {
		if (!token.empty()) {
			result.push_back(token);
		}
	}
	return result;
}

PathPoint::PathPoint(vec3 _pos, vec3 _dir, vec3 _up)
{
	pos = _pos;
	//roll = _roll;
	dir = _dir;
	up = _up;
}

static float w = 0.0f;

float lastX, lastY;
camera mycam;
void mouse_curs_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mycam.firstMouse) // initially set to true
	{
		lastX = xpos;
		lastY = ypos;
		mycam.firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = ypos - lastY;
	lastX = xpos;
	lastY = ypos;
	const float sensitivity = .1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	mycam.yaw += xoffset;
	mycam.pitch -= yoffset;

	if (mycam.pitch > 179.0f)
		mycam.pitch = 179.0f;
	if (mycam.pitch < -179.0f)
		mycam.pitch = -179.0f;
}