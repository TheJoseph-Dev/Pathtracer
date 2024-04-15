#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Shader.h"
#include "Texture.h"
#include <vector>


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class FrameBuffer {

	unsigned int ID;
	unsigned int RBO;
	unsigned int VAO;

	Shader fbShader;
	FBTexture fbTexture;

	glm::mat4 mvp;

	float width, height;
	bool border;
public:

	//FrameBuffer() {};
	FrameBuffer(float width, float height, bool border = false, unsigned int texSlot = 0U);
	~FrameBuffer();

	void Check();

	void UpdateBounds(float width, float height);

	void Draw(bool shouldTonemap = false, unsigned int texSlot = 0U, int preLoadedTextureID = -1);

	void Bind(unsigned int texSlot = 0U, bool useHDR = false);
	void Unbind();

	void Print();

	// Setters
	void SetTransform(const glm::mat4& mvp);
	void SetShader(const Shader& shader);
	void SetTexture(const FBTexture& texture);

	// Getters
	inline const Shader& GetShader() const { return this->fbShader; }
	inline const FBTexture& GetFBTexture() const { return this->fbTexture; }
	inline float GetWidth() const { return this->width; };
	inline float GetHeight() const { return this->height; };
};


class BloomFB {

	unsigned int ID;
	unsigned int RBO;
	unsigned int VAO;

	unsigned short mipChainLength = 4;

	Shader downsampler;
	Shader upsampler;
	std::vector<FBBloomMip> mips = std::vector<FBBloomMip>();

	glm::mat4 mvp;

	float width, height;
public:

	//FrameBuffer() {};
	BloomFB(float width, float height, unsigned short mipChainLength = 4);
	~BloomFB();

	void Check();

	void UpdateBounds(float width, float height);

	void CreateBloomTexture();
	void Draw(const FBTexture& srcTexture, float filterRadius, unsigned int mipSlot = 0U);

	void Bind();
	void Unbind();

	//void Print();

	// Setters
	void SetTransform(const glm::mat4& mvp);
	//void SetShader(const Shader& shader);

	// Getters
	//inline Shader& GetShader() { return this->fbShader; }
	inline unsigned int GetBloomTextureID() const { return this->mips[0].GetMipID(); }
	inline unsigned int GetBloomMipAt(unsigned int index) const { return this->mips[index].GetMipID(); }
	inline float GetWidth() const { return this->width; };
	inline float GetHeight() const { return this->height; };
};


#endif // !FRAMEBUFFER_H
