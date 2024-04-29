#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <vector>
#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Vertex {
	glm::vec3 position;
	glm::vec2 textureCoord;
	glm::vec3 normal;

	static int GetStride() { return (3*2 + 2); }
};

class OBJLoader {

	// Won't use the Vertex struct because these are only the vertex data, not the faces
	std::vector<glm::vec3> positions = std::vector<glm::vec3>();
	std::vector<glm::vec2> textureCoords = std::vector<glm::vec2>();
	std::vector<glm::vec3> normals = std::vector<glm::vec3>();

	struct VertexData {
		float* vertices = nullptr;
		int verticesSize = 0; // Considers Stride, but not type
		int verticesCount = 0; // Just the vertices count

		//float* vPos = nullptr; // Only the vertex positions
		//float* vNPos = nullptr; // Only the vertex position and normals
	};

	VertexData vData;
	VertexData ssbVData;

public:
	OBJLoader(const char* filepath);

	~OBJLoader() { delete[] vData.vertices; delete[] ssbVData.vertices; };

private:
	glm::vec3 LoadVertexData(const std::string& data);
	std::vector<Vertex> LoadFace(const std::string& face);
	Vertex CreateVertex(const std::string& indicies);
	void CreateVertexArray(const std::vector<Vertex>& loadedVertices);
	void CreateSSBuffer(const std::vector<Vertex>& loadedVertices);

public:
	VertexData GetVerticesAsSSBuffer() const { return ssbVData; };

	VertexData GetVertices() const { return vData; }

	std::vector<glm::vec3> GetPositions() const { return positions; }
};

#endif // !OBJLOADER_H
