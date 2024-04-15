#ifndef UTILS_H
#define UTILS_H
#include <iostream>

#define print(x) std::cout << x << std::endl
#define printVector3(vector) std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl

#define count(arr, type) sizeof(arr)/sizeof(type)
#define Resources(path) "Project/Resources/"  path

#endif // !UTILS_H

