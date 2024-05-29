#include <iostream>

#include <GLAD/glad.h>
#include <glfw3.h>

#include "Models/Shader.h"
#include "Models/Texture.h"
#include "Models/Camera.h"
#include "Models/OBJLoader.h"
#include "Models/Framebuffer.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

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


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// End Init ---

	Camera camera = Camera(glm::vec3(0.0f, 0.9f, -1.6f));
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

	OBJLoader triangleObj = OBJLoader(Resources("3D Models/lpKnight.obj"));
	float icoPos[4] = { 0.0, 0.5, 0.0, 0.0 };
	auto triangleObjData = triangleObj.GetVerticesAsSSBuffer();

	constexpr unsigned int objs = 1;
	constexpr int mInfoStride = 8;

	struct MeshInfo {
		float info[4];
		float gPos[4];
	};

	MeshInfo mInfos[objs * mInfoStride] = { 
		{
			(triangleObjData.verticesCount), 0.0f, 0.0f, 0.0f,
			icoPos[0], icoPos[1], icoPos[2], icoPos[3]
		}
	};
	
	unsigned int lObjsSize = triangleObjData.verticesSize;
	float* lObjsVertices = triangleObjData.vertices;

	Texture pyObjTex = Texture(Resources("Textures/Gold.jpg"));
	pyObjTex.Load();

	SSBO meshLayoutSSBO;
	meshLayoutSSBO.Bind(10);
	meshLayoutSSBO.SendData((long)(objs * mInfoStride * sizeof(float)), (void*)(mInfos) );
	meshLayoutSSBO.Unbind();

	SSBO meshSSBO;
	meshSSBO.Bind(11);
	meshSSBO.SendData((long)(lObjsSize * sizeof(float)), (void*)lObjsVertices);
	meshSSBO.Unbind();

	struct ObjectInfo {
		glm::vec4 position;
		float type;
		float matIndex;
		float size;
		float w = 0.0f;
		ObjectInfo(glm::vec4 position, unsigned int type, unsigned int matIndex, float size):
			position(position), type(type), matIndex(matIndex), size(size) {};
	};


	std::vector<ObjectInfo> objsInfo = std::vector<ObjectInfo>();
	ObjectInfo floorBox = ObjectInfo(glm::vec4(0.0, -0.7, 0.0, 0.0), 1, 0, 1.2f);
	ObjectInfo l1Sph = ObjectInfo(glm::vec4(0.0f, 1.7f, -0.5f, 0.0f), 0, 6, 0.1f);
	ObjectInfo lsBox = ObjectInfo(glm::vec4(-2.4f, 1.2f, 0.0f, 0.0f), 1, 1, 1.2f);
	ObjectInfo rsBox = ObjectInfo(glm::vec4(2.4f, 1.2f, 0.0f, 0.0f), 1, 8, 1.2f);
	ObjectInfo backBox = ObjectInfo(glm::vec4(0.0f, 1.2f, 2.4f, 0.0f), 1, 1, 1.2f);
	ObjectInfo topBox = ObjectInfo(glm::vec4(0.0f, 3.6f, 0.0f, 0.0f), 1, 1, 1.2f);
	ObjectInfo refSph = ObjectInfo(glm::vec4(0.8f, 0.9f, 0.5f, 0.0f), 0, 2, 0.3f);
	ObjectInfo blSph = ObjectInfo(glm::vec4(-0.15f, 0.56f, -0.5f, 0.0f), 0, 9, 0.05f);
	ObjectInfo spSph = ObjectInfo(glm::vec4(-0.3f, 0.7f, -0.3f, 0.0f), 0, 3, 0.15f);
	ObjectInfo pRefSph = ObjectInfo(glm::vec4(0.6f, 0.65f, -0.5f, 0.0f), 0, 4, 0.15f);
	ObjectInfo lCube = ObjectInfo(glm::vec4(0.4f, 0.525f, -0.7f, 0.0f), 1, 7, 0.025f);
	objsInfo.push_back(floorBox);
	objsInfo.push_back(lsBox);
	objsInfo.push_back(rsBox);
	objsInfo.push_back(backBox);
	objsInfo.push_back(topBox);
	objsInfo.push_back(refSph);
	objsInfo.push_back(l1Sph);
	objsInfo.push_back(blSph);
	objsInfo.push_back(spSph);
	objsInfo.push_back(pRefSph);
	objsInfo.push_back(lCube);
	SSBO objsInfoSSBO;
	//objsInfoSSBO.Bind(12);
	//objsInfoSSBO.SendData(sizeof(objsInfo), (void*)objsInfo);
	//objsInfoSSBO.Unbind();

	// SSBO = Global GPU Memory => Bigger, but Slower
	// UBO = Local GPU Memory => Smaller, but Fasters

	FrameBuffer fb = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 0);
	fb.Check();
	fb.Unbind();

	FrameBuffer postFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 2);
	postFB.Check();
	postFB.Unbind();


	FrameBuffer bloomFilterFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 1);
	bloomFilterFB.SetShader(Shader(Resources("Shaders/Bloom/Filter.glsl")));
	bloomFilterFB.Check();
	bloomFilterFB.Unbind();

	constexpr unsigned short bloomMipChainLegth = 8;
	BloomFB bloomFB = BloomFB(WINDOW_WIDTH, WINDOW_HEIGHT, bloomMipChainLegth);
	bloomFB.Check();
	bloomFB.Unbind();

	FrameBuffer finalBloomFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 1);
	finalBloomFB.Check();
	finalBloomFB.Unbind();

	FrameBuffer bloomMixFB = FrameBuffer(WINDOW_WIDTH, WINDOW_HEIGHT, false, 2);
	bloomMixFB.SetShader(Shader(Resources("Shaders/Bloom/Mixer.glsl")));
	bloomMixFB.Check();
	bloomMixFB.Unbind();

	pyObjTex.Bind();
	Shader canvasShader = Shader(Resources("Shaders/pathtracer.glsl"));
	canvasShader.Bind();
	canvasShader.SetUniform2f("iResolution", WINDOW_WIDTH, WINDOW_HEIGHT);
	canvasShader.SetUniformInt("meshTexture", 3);
	pyObjTex.Unbind();

	glm::mat4 mvp = glm::mat4(1.0f);
	glm::mat4 proj = glm::ortho(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f);
	glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(WINDOW_WIDTH, WINDOW_HEIGHT, 1.0f));

	//#pragma region Settings
	// Settings
	bool bloom = true;
	bool accumulate = true;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	ImGui::StyleColorsDark();

	float time = 0;
	int currentFrame = -1;

	glm::vec3 lastCamPos = camera.GetPosition();
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		fb.SetTransform(proj);
		postFB.SetTransform(proj);
		bloomFilterFB.SetTransform(proj);
		bloomFB.SetTransform(proj);
		finalBloomFB.SetTransform(proj);
		bloomMixFB.SetTransform(proj);
		fb.Bind(0);


		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Pathtracer");

		ImGui::Text("Settings");
		bool accPress = ImGui::Checkbox("Accumulate", &accumulate);
		ImGui::Text("PostProcessing");
		ImGui::Checkbox("Bloom", &bloom);

		ImGui::End();

		ImGui::Begin("Objects");

		for (int i = 0; i < objsInfo.size(); i++) {
			ObjectInfo* objInfo = &objsInfo[i];
			ImGui::PushID(i);

			ImGui::DragFloat4("Position", glm::value_ptr(objInfo->position), 0.1f);
			ImGui::DragFloat("Type", &(objInfo->type), 1, 0, 1);
			ImGui::DragFloat("Material Index", &(objInfo->matIndex), 1, 0, 5);
			ImGui::DragFloat("Size", &(objInfo->size), 0.1f, 0.1f, 5);
			ImGui::Separator();
			ImGui::PopID();
		}

		ImGui::End();

		objsInfoSSBO.Bind(12);
		objsInfoSSBO.SendData(sizeof(ObjectInfo) * objsInfo.size(), (void*)objsInfo.data());
		objsInfoSSBO.Unbind();

		if (accPress && accumulate) currentFrame = 0;

		time = glfwGetTime();
		currentFrame++;
		
		//mvp = glm::mat4(1.0f);
		mvp = proj * model;

		canvasShader.Bind();
		//camera.set(glm::vec3(sin(time), 0.0f, cos(time)*0.5));
		glm::vec3 cameraPos = camera.GetPosition();
		glm::vec3 cameraRot = camera.GetRotation();
		canvasShader.SetUniform3f("cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);
		canvasShader.SetUniform3f("cameraRot", cameraRot.x, cameraRot.y, cameraRot.z);
		canvasShader.SetUniformMat4("MVP", mvp);
		canvasShader.SetUniformFloat("iTime", time);
		canvasShader.SetUniformInt("iFrame", currentFrame);
		canvasShader.SetUniformInt("accumulate", accumulate);
		canvasShader.SetUniformInt("lastFrameTex", 0);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, count(canvasVertices, float));

		fb.Unbind();
		postFB.Bind(2);
		fb.Draw(false, 0);
		postFB.Unbind();
		postFB.GetShader().Bind();
		postFB.GetShader().SetUniformInt("iFrame", currentFrame);
		postFB.GetShader().SetUniformInt("accumulate", accumulate);
		if(bloom) {
			bloomFilterFB.Bind(1);
			postFB.Draw(true, 2);
			bloomFilterFB.Unbind();
			bloomFilterFB.Draw(false, 1);

			constexpr unsigned int bloomBind = 1;
			bloomFB.Draw(bloomFilterFB.GetFBTexture(), 0.01f, bloomBind);
			finalBloomFB.Draw(false, bloomBind, bloomFB.GetBloomMipAt(0));

			bloomMixFB.GetShader().Bind();
			bloomMixFB.GetShader().SetUniformInt("iFrame", currentFrame);
			bloomMixFB.GetShader().SetUniformInt("lastFrameTex", 1);
			bloomMixFB.Draw(true, 2, 4);
		}
		else postFB.Draw(true, 2);
		//#endif

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
}