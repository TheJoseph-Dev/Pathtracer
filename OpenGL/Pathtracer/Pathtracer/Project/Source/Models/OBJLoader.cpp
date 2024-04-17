#include "OBJLoader.h"

#include <fstream> // file stream
#include <sstream> // string stream
#include <ostream>

#include "Source/Utils.h"

std::vector<std::string> split(std::string s, std::string separator) {
	std::string sCopy = s;
	std::vector<std::string> splittedString = std::vector<std::string>();
	while (sCopy.find(separator) != std::string::npos) {
		std::string token = sCopy.substr(0, sCopy.find(separator));
		splittedString.push_back(token);
		sCopy.erase(0, sCopy.find(separator) + separator.length());
	}

	splittedString.push_back(sCopy);

	return splittedString;
}

OBJLoader::OBJLoader(const char* filepath) {

	std::ifstream stream = std::ifstream(filepath);
	print(filepath << "\n\n");

	if (!stream) { print("ERROR: OBJLoader got a NULL directory"); return; }

	std::vector<Vertex> objVertices;
	std::string line;

	while (getline(stream, line)) {
		
		if (line.rfind("v ",  0) == 0)  { this->positions.push_back(LoadVertexData(line)); continue; }
		if (line.rfind("vt", 0) == 0) { this->textureCoords.push_back(LoadVertexData(line)); continue; }
		if (line.rfind("vn", 0) == 0) { this->normals.push_back(LoadVertexData(line)); continue; }

		if (line.rfind("f", 0) == 0) {
			auto face = LoadFace(line);
			objVertices.insert(objVertices.end(), face.begin(), face.end());
			continue;
		}
	}

	CreateVertexArray(objVertices);
	CreateSSBuffer(objVertices);
}

// v, vt, vn
glm::vec3 OBJLoader::LoadVertexData(const std::string& data) {
	const std::string separator = " ";
	glm::vec3 vData = glm::vec3(0);
	auto values = split(data, separator);
	values.erase(values.begin());
	
	vData.x = stof(values.at(0));
	vData.y = stof(values.at(1));
	if (values.size() > 2) vData.z = stof(values.at(2));

	return vData;
}

// f
std::vector<Vertex> OBJLoader::LoadFace(const std::string& face) {
	std::vector<Vertex> vertices;

	// f 1/2/3 4/5/6 7/8/9 => (1/2/3), (4/5/6), (7/8/9),
	std::string separator = " ";
	auto triangle = split(face, separator);

	triangle.erase(triangle.begin());

	for (std::string vIndicies : triangle) {
		vertices.push_back(CreateVertex(vIndicies));
	}

	return vertices;
}

// indicies
Vertex OBJLoader::CreateVertex(const std::string& indicies) {
	Vertex vertex = Vertex();

	// (1/2/3)
	std::string separator = "/";
	auto vertexIndicies = split(indicies, separator);

	int posIndex = stoi(vertexIndicies.at(0)) - 1;
	int texCoordIndex = stoi(vertexIndicies.at(1)) - 1;
	int normIndex = stoi(vertexIndicies.at(2)) - 1;

	vertex.position = this->positions.at(posIndex);
	vertex.textureCoord = this->textureCoords.at(texCoordIndex);
	vertex.normal = this->normals.at(normIndex);

	return vertex;
}


void OBJLoader::CreateVertexArray(const std::vector<Vertex>& loadedVertices) {
	uint32_t arrSize = Vertex::GetStride() * loadedVertices.size();
	this->vData.vertices = new float[arrSize];
	this->vData.verticesSize = arrSize;
	this->vData.verticesCount = loadedVertices.size();
	const std::vector<Vertex>& lv = loadedVertices;
	//std::reverse(lv.begin(), lv.end());

	float* vertices = (this->vData.vertices);
	std::ofstream outfile("Project/Resources/vertices.txt");

	for (int i = 0, k = 0; k < lv.size(); i += Vertex::GetStride(), k++) {
		*(vertices + i + 0) = lv.at(k).position.x;
		*(vertices + i + 1) = lv.at(k).position.y;
		*(vertices + i + 2) = lv.at(k).position.z;

		*(vertices + i + 3) = lv.at(k).textureCoord.x;
		*(vertices + i + 4) = lv.at(k).textureCoord.y;

		*(vertices + i + 5) = lv.at(k).normal.x;
		*(vertices + i + 6) = lv.at(k).normal.y;
		*(vertices + i + 7) = lv.at(k).normal.z;

		outfile << lv.at(k).position.x << ", ";
		outfile << lv.at(k).position.y << ", ";
		outfile << lv.at(k).position.z << ", \t";

		outfile << lv.at(k).textureCoord.x << ", ";
		outfile << lv.at(k).textureCoord.y << ", \t\t";

		outfile << lv.at(k).normal.x << ", ";
		outfile << lv.at(k).normal.y << ", ";
		outfile << lv.at(k).normal.z << ", ";
		outfile << std::endl;
		if ((k + 1) % 6 == 0) outfile << std::endl;
	}

	outfile.close();


}

void OBJLoader::CreateSSBuffer(const std::vector<Vertex>& loadedVertices) {
	// SSBData
	uint32_t arrSize = (Vertex::GetStride()+2) * loadedVertices.size();
	this->ssbVData.vertices = new float[arrSize];
	this->ssbVData.vPos = new float[4 * loadedVertices.size()];
	this->ssbVData.verticesSize = arrSize;
	this->ssbVData.verticesCount = loadedVertices.size();
	const std::vector<Vertex>& lv = loadedVertices;

	float* vertices = (this->ssbVData.vertices);
	float* vPos = (this->ssbVData.vPos);
	for (int i = 0, j = 0, k = 0; k < lv.size(); i += Vertex::GetStride() + 2, j += 4, k++) {
		*(vertices + i + 0) = lv.at(k).position.x;
		*(vertices + i + 1) = lv.at(k).position.y;
		*(vertices + i + 2) = lv.at(k).position.z;
		*(vertices + i + 3) = 0.0f;

		*(vPos + j + 0) = lv.at(k).position.x;
		*(vPos + j + 1) = lv.at(k).position.y;
		*(vPos + j + 2) = lv.at(k).position.z;
		*(vPos + j + 3) = 0.0f;

		*(vertices + i + 4) = lv.at(k).textureCoord.x;
		*(vertices + i + 5) = lv.at(k).textureCoord.y;

		*(vertices + i + 6) = lv.at(k).normal.x;
		*(vertices + i + 7) = lv.at(k).normal.y;
		*(vertices + i + 8) = lv.at(k).normal.z;
		*(vertices + i + 9) = 0.0f;
	}
}