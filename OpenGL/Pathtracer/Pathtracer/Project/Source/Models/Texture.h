#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>

class Texture {

protected:
	unsigned int textureID;
	std::string* filepaths = nullptr;
	short unsigned int texturesCount;

	int width = 0, height = 0;
public:

	Texture() : textureID(0), texturesCount(0) {};
	Texture(const char* filepath);
	Texture(std::string* filepaths, short unsigned int texturesCount); // TEXTURE_ARRAY
	~Texture();

	void Load();
	void LoadArrray(int storageWidth = 1024, int storageHeight = 1024);

	void Bind(unsigned int slot = 0);
	void BindArray(unsigned int slot = 0);
	void Unbind();

	// Getters
	inline int GetWidth() const { return this->width; }
	inline int GetHeight() const { return this->height; }
};

// Framebuffer Texture
class FBTexture {

	unsigned int textureID;
	//std::string* filepaths = nullptr;

	int width = 0, height = 0;
public:
	//FBTexture();
	FBTexture(float screen_width, float screen_height);
	~FBTexture();

	void Load(unsigned int texSlot = 0U);
	void UpdateSize(float width, float height);

	void Bind(unsigned int slot = 0, bool hdrBuffer = false) const;
	void Unbind() const;

	// Setters
	void SetWidth(int width) { this->width = width; }
	void SetHeight(int height) { this->height = height; }
};


class SkyboxTexture
{
private:
	unsigned int textureID;
	const std::string* filepaths;
	int width = 0, height = 0;

public:
	SkyboxTexture(const std::string* cubeMapTextures);
	~SkyboxTexture();

	void Load();
	void Bind();
	void Unbind();

};

class FBBloomMip {

	unsigned int textureID;
	int width = 0, height = 0;

public:

	FBBloomMip(float width, float height);
	~FBBloomMip();

	void Load();

	void Bind(unsigned int slot = 0U) const;
	void Unbind() const;

	void UpdateSize(float width, float height);

	// Getters
	inline unsigned int GetMipID() const { return this->textureID; }
	inline unsigned int GetWidth() const { return this->width; }
	inline unsigned int GetHeight() const { return this->height; }
};

#endif // !TEXTURE_H
