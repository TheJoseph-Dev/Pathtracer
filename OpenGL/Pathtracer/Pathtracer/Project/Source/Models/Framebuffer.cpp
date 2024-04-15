#include "Framebuffer.h"
#include "Source/Utils.h"
#include <glad/glad.h>

//FrameBuffer::FrameBuffer() : ID(0), RBO(0), VAO(0), mvp(glm::mat4(1.0f)), width(960.0f), height(540.0f), border(false) {}

FrameBuffer::FrameBuffer(float width, float height, bool border, unsigned int texSlot)
	: fbShader(Shader(Resources("Shaders/Default_FrameBuffer.glsl"))), mvp(glm::mat4(1.0f)), fbTexture(FBTexture(width, height)), ID(0), width(width), height(height), border(border)
{
	glGenFramebuffers(1, &this->ID);
	glBindFramebuffer(GL_FRAMEBUFFER, this->ID);

	//fbTexture.Bind();
	fbTexture.Load(texSlot);

	// Texture and HDR
	//unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	//glDrawBuffers(2, attachments);

	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	const unsigned short stride = 5;

	float vertices[] = {
		0.0, 0.0, 0.0,	 0.0, 0.0,
		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,
						 
		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,
		1.0, 1.0, 0.0,	 1.0, 1.0
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &this->VAO);
	glBindVertexArray(this->VAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));

	Print();
}

FrameBuffer::~FrameBuffer() {
	Unbind();
	//fbTexture.~FBTexture();
	glDeleteFramebuffers(1, &ID);
}

void FrameBuffer::Draw(bool shouldTonemap, unsigned int texSlot, int preLoadedTextureID) {
	const unsigned short quadVertexCount = 6;

	// second pass
	//this->Unbind(); // back to default
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	glClearColor(0.01, 0.02, 0.05, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draws a framebuffer border
	//if (border == true) {}

	bool hdrRender = false;
	bool usingFBTexture = preLoadedTextureID == -1;

	fbShader.Bind();
	if (usingFBTexture)
		fbTexture.Bind(texSlot, hdrRender);
	else {
		glActiveTexture(GL_TEXTURE0 + texSlot);
		glBindTexture(GL_TEXTURE_2D, (unsigned int)preLoadedTextureID);
	}

	glm::mat4 model_scale = glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 0.0f));

	fbShader.SetUniformMat4("MVP", mvp*model_scale);
	fbShader.SetUniform2f("iResolution", this->width, this->height);
	fbShader.SetUniformInt("screenTexture", texSlot);
	fbShader.SetUniformInt("shouldTonemap", (int)shouldTonemap);
	glBindVertexArray(this->VAO);
	glDrawArrays(GL_TRIANGLES, 0, quadVertexCount);
}

void FrameBuffer::Check() {
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { print("ERROR::FRAMEBUFFER:: Framebuffer is not complete! - ID: " << this->ID); }
}

void FrameBuffer::UpdateBounds(float width, float height) {
	this->width = width;
	this->height = height;
	glBindFramebuffer(GL_FRAMEBUFFER, this->ID);
	//glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction 2.2

	fbTexture.UpdateSize(width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void FrameBuffer::Print() {
	print("FB_ID:  " << this->ID);
	print("WIDTH: " << this->width << " HEIGHT: " << this->height);
}

void FrameBuffer::SetTransform(const glm::mat4& mvp) {
	this->mvp = mvp;
}

void FrameBuffer::SetShader(const Shader& shader) { 
	this->fbShader = shader;
}

void FrameBuffer::SetTexture(const FBTexture& texture) {
	this->fbTexture = texture;
}

void FrameBuffer::Bind(unsigned int texSlot, bool useHDR) {
	glBindFramebuffer(GL_FRAMEBUFFER, ID); this->fbShader.Bind(); this->fbTexture.Bind(texSlot, useHDR);
}

void FrameBuffer::Unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


// --- Bloom ---

BloomFB::BloomFB(float width, float height, unsigned short mipChainLength)
	: downsampler(Shader(Resources("Shaders/Bloom/Downsampler.glsl"))), mvp(glm::mat4(1.0f)),
	  upsampler(Shader(Resources("Shaders/Bloom/Upsampler.glsl"))), ID(0), width(width), height(height), mipChainLength(mipChainLength)
{
	glGenFramebuffers(1, &this->ID);
	glBindFramebuffer(GL_FRAMEBUFFER, this->ID);

	glm::vec2 downsampledResolution = glm::vec2(width, height);

	for (int i = 0; i < this->mipChainLength; i++) {
		if (i != 0) downsampledResolution /= 2;

		FBBloomMip mip = FBBloomMip(downsampledResolution.x, downsampledResolution.y);
		mip.Load();
		print("Mip ID: " << mip.GetMipID() << " - " << mip.GetWidth() << ", " << mip.GetHeight());

		this->mips.emplace_back(mip);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->mips[0].GetMipID(), 0);

	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	const unsigned short stride = 5;

	float vertices[] = {
		0.0, 0.0, 0.0,	 0.0, 0.0,
		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,

		1.0, 0.0, 0.0,	 1.0, 0.0,
		0.0, 1.0, 0.0,	 0.0, 1.0,
		1.0, 1.0, 0.0,	 1.0, 1.0
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &this->VAO);
	glBindVertexArray(this->VAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
}

BloomFB::~BloomFB() {
	Unbind();
	//fbTexture.~FBTexture();
	glDeleteFramebuffers(1, &ID);
}

void BloomFB::CreateBloomTexture() {
	// Downsample
	this->downsampler.Bind();
	this->downsampler.SetUniformInt("srcTexture", 0);
	//this->downsampler.Unbind();

	// Upsample
	this->upsampler.Bind();
	this->upsampler.SetUniformInt("srcTexture", 0);
	//this->upsampler.Unbind();
}

void BloomFB::Draw(const FBTexture& srcTexture, float filterRadius, unsigned int mipSlot) {
	const unsigned short quadVertexCount = 6;

	Bind();

	const glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 0.0f));

	//CreateBloomTexture();
	srcTexture.Bind(1);

	this->downsampler.Bind();
	this->downsampler.SetUniform2f("srcResolution", this->width, this->height);
	this->downsampler.SetUniformMat4("MVP", mvp * modelScale);

	// Bind srcTexture (HDR color buffer) as initial texture input
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, srcTexture);

	// Progressively downsample through the mip chain
	for (int i = 0; i < mips.size(); i++)
	{
		const FBBloomMip& mip = mips[i];
		glViewport(0, 0, mip.GetWidth(), mip.GetHeight());
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.GetMipID(), 0);

		// Render screen-filled quad of resolution of current mip
		glBindVertexArray(this->VAO);
		glDrawArrays(GL_TRIANGLES, 0, quadVertexCount);
		glBindVertexArray(0);

		// Set current mip resolution as srcResolution for next iteration
		this->downsampler.SetUniform2f("srcResolution", mip.GetWidth(), mip.GetHeight());
		// Set current mip as texture input for next iteration
		glBindTexture(GL_TEXTURE_2D, mip.GetMipID());
	}


	this->upsampler.Bind();
	this->upsampler.SetUniformFloat("filterRadius", filterRadius);
	this->upsampler.SetUniformMat4("MVP", mvp * modelScale);

	// Enable additive blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	for (int i = mips.size() - 1; i > 0; i--)
	{
		const FBBloomMip& mip = mips[i];
		const FBBloomMip& nextMip = mips[i - 1];

		// Bind viewport and texture from where to read
		mip.Bind(mipSlot);

		// Set framebuffer render target (we write to this texture)
		glViewport(0, 0, nextMip.GetWidth(), nextMip.GetHeight());
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextMip.GetMipID(), 0);

		// Render screen-filled quad of resolution of current mip
		glBindVertexArray(this->VAO);
		glDrawArrays(GL_TRIANGLES, 0, quadVertexCount);
		glBindVertexArray(0);
	}

	// Disable additive blending
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
	glDisable(GL_BLEND);


	Unbind();
	glViewport(0, 0, this->width, this->height);
}

void BloomFB::Check() {
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { print("ERROR::FRAMEBUFFER:: Framebuffer is not complete! - ID: " << this->ID); }
}

void BloomFB::UpdateBounds(float width, float height) {
	this->width = width;
	this->height = height;
	glBindFramebuffer(GL_FRAMEBUFFER, this->ID);

	glm::vec2 downsampledResolution = glm::vec2(width, height);
	for (int i = mips.size() - 1; i >= 0; i--) {
		if (i != 0) downsampledResolution /= 2;
		mips[i].UpdateSize(downsampledResolution.x, downsampledResolution.y);
	}
	

	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}


void BloomFB::SetTransform(const glm::mat4& mvp) {
	this->mvp = mvp;
}


void BloomFB::Bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, ID); downsampler.Bind();
}

void BloomFB::Unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}