#include "Texture.h"
#include "SOIL2/SOIL2.h"

#include "Source/Utils.h"

#include <glad/glad.h>
#include <glfw3.h>

Texture::Texture(const char* filepath) : textureID(0), texturesCount(1) {
	this->filepaths = new std::string[1];
	*(this->filepaths) = filepath;
	glGenTextures(1, &this->textureID);

	print("Texture filepath set to: " << filepath);
}

Texture::Texture(std::string* filepaths, short unsigned int texturesCount) 
	: textureID(0), filepaths(filepaths), texturesCount(texturesCount) 
{
	glGenTextures(1, &this->textureID);
}

Texture::~Texture() { /*glDeleteTextures(1, &textureID);*/ }

void Texture::Load() {
	/*
		- Create textureID and binds it
		- Configs the Mipmap, Wrap/Filtering options
		- Uses an image library to load the pixels
		- Generates a texture
		- Frees the image buffer
	*/

	// Texture Wrap/Filtering options and Mipmap
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	int channels;
	unsigned char* image = SOIL_load_image((this->filepaths)->c_str(), &(this->width), &(this->height), &channels, SOIL_LOAD_RGBA);

	if (!image) { print("Couldn't Load Texture: " << this->filepaths->c_str() ); return; }
	int mipmaplvl = 0; // "Texture LOD" level
	glTexImage2D(GL_TEXTURE_2D, mipmaplvl, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	//glGenerateMipmap(GL_TEXTURE_2D); - Disabled to remove weird polar coordnates edges

	SOIL_free_image_data(image);
	Unbind();
}

void Texture::LoadArrray(int storageWidth, int storageHeight) {
	
	/*glTexStorage3D(GL_TEXTURE_2D_ARRAY,
		1,                    // mipmaps
		GL_RGBA,               // Internal format
		2048, 1024,           // width,height
		16                   // Number of layers
	);*/

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//const int storageWidth = 1024, storageHheight = 512;
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, storageWidth, storageHeight, texturesCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	
	// Loop
	for (int layer = 0; layer < texturesCount; layer++) {
		int channels;
		unsigned char* image = SOIL_load_image((this->filepaths+layer)->c_str(), &(this->width), &(this->height), &channels, SOIL_LOAD_RGBA);
		print((this->filepaths + layer)->c_str());

		//BindArray();

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
			0,                      //Mipmap number
			0, 0, layer, //xoffset, yoffset, zoffset
			width, height, 1,          //width, height, depth
			GL_RGBA,                 //format
			GL_UNSIGNED_BYTE,       //type
			image					//pointer to data
		);
		//glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 2048, 1024, layer, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		//glTextureSubImage3D(textureID, 0, 0, 0, 0, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);


	}
	//texture->textureArrayLocation = layer;
	//layer++;
}

void Texture::Bind(unsigned int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, this->textureID);
}

void Texture::BindArray(unsigned int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureID);
}

void Texture::Unbind() { glBindTexture(GL_TEXTURE_2D, 0); }


// --- FBTexture

FBTexture::FBTexture(float screen_width, float screen_height) : textureID(0) {
	this->width = screen_width;
	this->height = screen_height;

	glGenTextures(1, &this->textureID);
	//glGenTextures(1, &this->hdrBufferID);

	print("FrameBuffer Texture Set");
}

FBTexture::~FBTexture() { Unbind(); glDeleteTextures(1, &this->textureID); }

void FBTexture::Load(unsigned int texSlot) {
	// Texture Wrap/Filtering options and Mipmap
	// FrameBuffer doesn't need wrapping
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	Bind(texSlot, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->textureID, 0);

	// HDR/Bloom Buffer
	/*
	Bind(1, true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->hdrBufferID, 0);
	*/

	// Stencil Buffer and 3D
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, this->textureID, 0);
}

void FBTexture::UpdateSize(float width, float height) {
	this->width = width;
	this->height = height;

	Bind(0, false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	Unbind();
}

void FBTexture::Bind(unsigned int slot, bool hdrBuffer) const {
	glActiveTexture(GL_TEXTURE0 + slot);

	//if (hdrBuffer) { glBindTexture(GL_TEXTURE_2D, this->hdrBufferID); return; }
	
	glBindTexture(GL_TEXTURE_2D, this->textureID);
}

void FBTexture::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }



SkyboxTexture::SkyboxTexture(const std::string* cubeMapTextures) : filepaths(cubeMapTextures)
{
	glGenTextures(1, &textureID);
	Bind();
}

SkyboxTexture::~SkyboxTexture() {}

void SkyboxTexture::Load() {

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	int channels;
	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char* image = SOIL_load_image((this->filepaths+i)->c_str(), &(this->width), &(this->height), &channels, SOIL_LOAD_RGBA);
		if (!image) { print("Couldn't Load Skybox Texture: " << (this->filepaths+i)); continue; }
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image
		);
	}
}

void SkyboxTexture::Bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
}

void SkyboxTexture::Unbind() {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


FBBloomMip::FBBloomMip(float width, float height) : width(width), height(height), textureID(0) {
	glGenTextures(1, &textureID);
}

FBBloomMip::~FBBloomMip() {}


void FBBloomMip::Load() {
	Bind(1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	
}

void FBBloomMip::UpdateSize(float width, float height) {
	this->width = width;
	this->height = height;

	Bind();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	Unbind();
}


void FBBloomMip::Bind(unsigned int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, this->textureID);
}
void FBBloomMip::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }






