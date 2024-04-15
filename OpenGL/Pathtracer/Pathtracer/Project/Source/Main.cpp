#include <iostream>

#include <GLAD/glad.h>
#include <glfw3.h>

#include "Models/Shader.h"
#include "Models/Texture.h"
#include "Models/Camera.h"
#include "Models/OBJLoader.h"
#include "Models/Framebuffer.h"

#include "Utils.h"

#define WINDOW_WIDTH 1920.0f//960.0f
#define WINDOW_HEIGHT 1080.0f//540.0f

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	//currentWidth = width; currentHeight = height;
	glViewport(0, 0, width, height);
}

int main() {
	std::cout << "Hello World!" << std::endl;

	// SETUP ---
	// Init GLFW
	if (!glfwInit()) { print("ERROR: Could not Init GLFW");  return -1; }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Pathtracer", NULL, NULL);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Init GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { print("ERROR: Could not Init GLAD"); return -1; }


	// Tell OpenGL the size of the rendering window so OpenGL knows how we want to display the data and coordinates with respect to the window.
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Window Resize Callback
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// End Init ---

	Camera camera = Camera(glm::vec3(0.0f, 0.9f, -0.1f));
	//camera.Rotate(glm::vec3(1.57/2.0f, 0, 0));

	float canvasVertices[] = {
		0.0, 0.0, 0.0,	 0.0, 0.0,
		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,

		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,
		1.0, 1.0, 0.0,	 1.0, 1.0
	};
	constexpr unsigned int stride = 5;

	unsigned int VBO;
	glCreateBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(canvasVertices), canvasVertices, GL_STATIC_DRAW);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(sizeof(float)*3));

	FrameBuffer fb = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 0);
	fb.Check();
	fb.Unbind();

	FrameBuffer postFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 1);
	postFB.Check();
	postFB.Unbind();

	/*
	constexpr unsigned short bloomMipChainLegth = 8;
	BloomFB bloomFB = BloomFB(WINDOW_WIDTH, WINDOW_HEIGHT, bloomMipChainLegth);
	bloomFB.Check();
	bloomFB.Unbind();
	*/

	Shader canvasShader = Shader(Resources("Shaders/pathtracer.glsl"));
	canvasShader.Bind();
	canvasShader.SetUniform2f("iResolution", WINDOW_WIDTH, WINDOW_HEIGHT);

	glm::mat4 mvp = glm::mat4(1.0f);
	glm::mat4 proj = glm::ortho(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f);
	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(WINDOW_WIDTH, WINDOW_HEIGHT, 1.0f));

	bool bloom = true;

	float time = 0;
	int currentFrame = -1;

	glm::vec3 lastCamPos = camera.GetPosition();
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		fb.SetTransform(proj);
		postFB.SetTransform(proj);
		//bloomFB.SetTransform(proj);
		fb.Bind(0);

		time = glfwGetTime();
		currentFrame++;
		
		//mvp = glm::mat4(1.0f);
		mvp = proj * model;

		canvasShader.Bind();
		glm::vec3 cameraPos = camera.GetPosition();
		glm::vec3 cameraRot = camera.GetRotation();
		canvasShader.SetUniform3f("cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);
		canvasShader.SetUniform3f("cameraRot", cameraRot.x, cameraRot.y, cameraRot.z);
		canvasShader.SetUniformMat4("MVP", mvp);
		canvasShader.SetUniformFloat("iTime", time);
		canvasShader.SetUniformInt("iFrame", currentFrame);
		canvasShader.SetUniformInt("lastFrameTex", 0);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, count(canvasVertices, float));


		fb.Unbind();
		postFB.Bind(1);
		fb.Draw(false, 0);
		postFB.GetShader().Bind();
		postFB.GetShader().SetUniformInt("iFrame", currentFrame);
		postFB.Unbind();
		postFB.Draw(true, 1);

		if (bloom) {
			/*
			bloomFB.Draw(fb.GetFBTexture(), 0.005f, 1);
			postFB.Bind();
			postFB.GetShader().Bind();
			postFB.GetShader().SetUniformInt("iFrame", currentFrame);
			postFB.Draw(true, 1, 1);
			*/
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}