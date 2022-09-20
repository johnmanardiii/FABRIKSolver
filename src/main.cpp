/*
CPE/CSC 474 Lab base code Eckhardt/Dahl
based on CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#include "SmartTexture.h"
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "skmesh.h"
#include "Camera.h"
#include "sound.h"
#include "line.h"
#include "Arm.h"
#include <cstdlib>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// assimp
#include <stdio.h>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"
#include <assimp/mesh.h>


using namespace std;
using namespace glm;
using namespace Assimp;

music_ music;

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}


class Application : public EventCallbacks
{
public:
	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> psky, skinProg, prog, bloomP, fBloomP;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexNormDBox, VertexTexBox, IndexBufferIDBox, Texture;
	GLuint bFBO, quadVAO, quadVBO, quadVBO2, rboDepth, rboStencil;
	unsigned int pingpongFBO[2];
	unsigned int pingpongBuffer[2];
	unsigned int colorBuffers[2];
	Arm arm;

	// skinnedMesh
	SkinnedMesh skmesh;
	// textures
	shared_ptr<SmartTexture> skyTex;
	
	// shapes
	shared_ptr<Shape> skyShape;

	int musicID = 0;
	
	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{

	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
	/*Note that any gl calls must always happen after a GL state is initialized */
	
	
	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom(const std::string& resourceDirectory)
	{
	
		// Initialize mesh.
		skyShape = make_shared<Shape>();
		skyShape->loadMesh(resourceDirectory + "/sphere.obj");
		skyShape->resize();
		skyShape->init();

		// sky texture
		auto strSky = resourceDirectory + "/sky.jpg";
		skyTex = SmartTexture::loadTexture(strSky, true);
		if (!skyTex)
			cerr << "error: texture " << strSky << " not found" << endl;

		// initialize objects used for bloom
		// initialize Framebuffers for bloom
		// initialize the frame buffer object
		glGenFramebuffers(1, &bFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, bFBO);


		int screenW, screenH;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenW, &screenH);

		glGenTextures(2, colorBuffers);
		for (unsigned int i = 0; i < 2; i++)
		{
			glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA16F, screenW, screenH, 0, GL_RGBA, GL_FLOAT, NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attach texture to framebuffer
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
			);
		}
		// attach depth buffer to render buffer
		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, screenW, screenH);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

		// specify the color buffers to draw into
		unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;

		// create ping pong fbo for 2 pass gaussian blur
		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongBuffer);
		for (unsigned int i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA16F, screenW, screenH, 0, GL_RGBA, GL_FLOAT, NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
			);
		}

		// specify which locations to use in the final shader
		GLuint Tex0Location = glGetUniformLocation(fBloomP->pid, "scene");
		GLuint Tex1Location = glGetUniformLocation(fBloomP->pid, "bloomBlur");
		glUseProgram(fBloomP->pid);
		glUniform1i(Tex0Location, 0);
		glUniform1i(Tex1Location, 1);
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		//arm.init(Arm( vec3(0), vec3(0, -.5, .5), vec3(0, -.4, 1.0), vec3(0, -.1, 1.7)));
		arm.init(Arm(vec3(0), vec3(0, -4, -3), vec3(0, -3, -6), vec3(0, -1, -8)));
		GLSL::checkVersion();

		// initialize music
		string musicDir = resourceDirectory + "/mario_epic_music.mp3";
		char* musicName = new char[musicDir.length() + 1];
		strcpy(musicName, musicDir.c_str());

		musicID = music.init_music(musicName);
		delete[] musicName;

		int width, height;
		glfwSetCursorPosCallback(windowManager->getHandle(), mouse_curs_callback);
		glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		lastX = width / 2.0f;
		lastY = height / 2.0f;

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			windowManager->shutdown();
		}
		
		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("tex");
		psky->addUniform("camPos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");
		psky->addUniform("ballColor");

		skinProg = std::make_shared<Program>();
		skinProg->setVerbose(true);
		skinProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
		if (!skinProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			windowManager->shutdown();
		}
		
		skinProg->addUniform("P");
		skinProg->addUniform("V");
		skinProg->addUniform("M");
		skinProg->addUniform("tex");
		skinProg->addUniform("camPos");
		skinProg->addAttribute("vertPos");
		skinProg->addAttribute("vertNor");
		skinProg->addAttribute("vertTex");
		skinProg->addAttribute("BoneIDs");
		skinProg->addAttribute("Weights");

		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");

		bloomP = std::make_shared<Program>();
		bloomP->setVerbose(true);
		bloomP->setShaderNames(resourceDirectory + "/bloom_vertex.glsl", resourceDirectory + "/bloom_fragment.glsl");
		if (!bloomP->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		bloomP->addAttribute("vertPos");
		bloomP->addAttribute("vertTex");
		bloomP->addUniform("horizontal");
		//bloomP->addUniform("weight");

		fBloomP = std::make_shared<Program>();
		fBloomP->setVerbose(true);
		fBloomP->setShaderNames(resourceDirectory + "/final_vertex.glsl", resourceDirectory + "/final_fragment.glsl");
		if (!fBloomP->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		fBloomP->addAttribute("vertPos");
		fBloomP->addAttribute("vertTex");
		fBloomP->addUniform("exposure");
		fBloomP->addUniform("bloom");
		fBloomP->addUniform("glTime");
	}

	// renders a quad for post-processing effects
	void renderQuad()
	{
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void renderBloom(float timeSinceStart)
	{
		// blur fragments
		bool horizontal = true, first_iteration = true;
		unsigned int amount = 30;
		// use the blur program to draw the scene
		bloomP->bind();
		for (unsigned int i = 0; i < amount; i++)
		{
			// bind the appropriate buffer to draw to
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			glUniform1i(bloomP->getUniform("horizontal"), horizontal);
			// first time rendering -> use extracted colors, otherwise use from previous step.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(
				GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffer[!horizontal]
			);
			renderQuad();
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		bloomP->unbind();
		// set render output to screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// mix the two frambuffers and send it out to the screen
		float exposure = 1.0f;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fBloomP->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
		//glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
		//glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
		glActiveTexture(GL_TEXTURE1);
		//BindTexture(GL_TEXTURE_2D, colorBuffers[1]);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glUniform1i(fBloomP->getUniform("bloom"), true);
		glUniform1f(fBloomP->getUniform("exposure"), exposure);
		glUniform1f(fBloomP->getUniform("glTime"), timeSinceStart);
		renderQuad();
		fBloomP->unbind();
	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();
		static double totaltime = 0;
		static double animation_time = 0;
		totaltime += frametime;
		animation_time += frametime;

		const int num_targets = 4;
		static vec3 targets[num_targets] = { vec3(8, 2, 2),
			vec3(-3, -5, -3),
			vec3(6, -3, 3.0),
			vec3(-8, 0, 2)};
		
		// start music on first frame:
		static int hasRunMusic = 0;
		if (!hasRunMusic)
		{
			hasRunMusic++;
			// music.play(musicID);
		}

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glBindFramebuffer(GL_FRAMEBUFFER, bFBO);	// render to framebuffer rather than screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		
		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);		
		if (width < height)
			{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect,  1.0f / aspect, -2.0f, 100.0f);
			}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		auto sangle = -3.1415926f / 2.0f;

		// Draw the sky using GLSL.
		psky->bind();
		GLuint texLoc = glGetUniformLocation(psky->pid, "tex");
		skyTex->bind(texLoc);
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3f(psky->getUniform("ballColor"), 1, 1, 1);
		mat4 ballScale = scale(mat4(1), vec3(.4));
		for (int i = 0; i < num_targets; i++)
		{
			M = translate(mat4(1), targets[i]) * ballScale;
			glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			skyShape->draw(psky, false);
		}
		skyTex->unbind();
		psky->unbind();

		// every 2 seconds, a new target will be reached:
		const double base_wait_time = 2.0f;
		static double wait_timer = base_wait_time;
		const float time_to_grab = 5.0f;
		static int current_index = -1;
		if (wait_timer < -time_to_grab)
		{
			wait_timer = base_wait_time;
		}
		wait_timer -= frametime;
		if(wait_timer > 0)
		{
			animation_time -= frametime;
		}
		int new_index = ((int)(animation_time / time_to_grab)) % num_targets;
		static Arm next_arm;
		if(current_index != new_index)
		{
			// we need new arms and a new thing to solve
			if (current_index != -1)
			{
				arm.init(next_arm);
			}
			next_arm.init(arm.FabrikSolve(targets[new_index]));
			current_index = new_index;
		}
		float t = fmod(animation_time, time_to_grab) / time_to_grab;
		t = clamp(t, 0.f, 1.f);

		Arm interp_Arm = linearInterpolateBetweenArms(arm, next_arm, t);
		if(t == 1)
		{
			interp_Arm.init(next_arm);
		}
		interp_Arm.draw(P, V);
		//next_arm.draw(P, V);

		// draw the joints for fun:
		psky->bind();
		skyTex->bind(texLoc);
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3f(psky->getUniform("ballColor"), 1, 0, 0);
		ballScale = scale(mat4(1), vec3(.05));
		
		M = translate(mat4(1), interp_Arm.P1) * ballScale;
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skyShape->draw(psky, false);

		M = translate(mat4(1), interp_Arm.P2) * ballScale;
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skyShape->draw(psky, false);

		M = translate(mat4(1), interp_Arm.P3) * ballScale;
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skyShape->draw(psky, false);
		
		skyTex->unbind();
		psky->unbind();

		// draw the camera path TODO: should be moved to camera class
		//campaths[0].draw(P, V, prog, skyShape, !mycam.pathcam);
		renderBloom(totaltime);
	}
};

//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	std::string missingTexture = "missing.jpg";
	
	SkinnedMesh::setResourceDir(resourceDir);
	SkinnedMesh::setDefaultTexture(missingTexture);
	
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

