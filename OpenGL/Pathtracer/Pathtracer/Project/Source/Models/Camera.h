#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"


#define RAD2DEG 57.29f
class Camera {
	
	glm::vec3 position;
	glm::vec3 rotation;
public:
	Camera(glm::vec3 position = glm::vec3(0), glm::vec3 rotation = glm::vec3(0)) : position(position), rotation(rotation) {};
	~Camera() {};

	void LookAt(glm::vec3 target) {
		//glm::mat4 lookAtMat = glm::lookAt(position, target, glm::vec3(0, 1, 0));
		//rotation *= lookAtMat;
	}

	glm::mat4 GetView() {
		glm::mat4 model = glm::mat4(1.0f);
		model = GetRotationTransform();

		model = glm::translate(model, position);
		return model;
	}

	void Translate(glm::vec3 translation) { this->position += translation; }
	void Move(glm::vec3 direction) {
		//glm::mat3 rt = GetRotationTransform();
		glm::mat4 rt = glm::mat4(1.0f);
		rt = glm::rotate(rt, -rotation.x, glm::vec3(1, 0, 0));
		rt = glm::rotate(rt, -rotation.y, glm::vec3(0, 1, 0));
		glm::vec3 move =  (rt*glm::vec4(direction.x, 0, direction.z, 1.0)) * glm::vec4(1, cos(-rotation.y), 1, 1);
		move.y += direction.y;
		this->position += move;
	}
	void Rotate(glm::vec3 rotate) { 
		this->rotation += rotate;
		this->rotation.x = glm::clamp(this->rotation.x, -(3.14f / 2.0f), (3.14f / 2.0f));
	}
	
	glm::vec3 GetPosition() const { return this->position; }
	glm::vec3 GetRotation() const { return this->rotation; }

private: 
	glm::mat4 GetRotationTransform() {
		glm::mat4 rt = glm::mat4(1.0f);
		rt = glm::rotate(rt, rotation.x, glm::vec3(1, 0, 0));
		rt = glm::rotate(rt, rotation.y, glm::vec3(0, 1, 0));
		//rt = glm::rotate(rt, rotation.z, glm::vec3(0, 0, 1));
		return rt;
	}
};

#endif // !CAMERA_H
