#pragma once
#include <GL\glew.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <opencv2\core\core.hpp>
#include <iomanip>
#include <assert.h>

#include <string>
#include <vector>

#include "Shader.h"
#include "VisualisationDisplayCameraClass.h"

namespace vis{

class Model
{
public:

	Model(float x, float y, float z);
	Model(GLuint p, float x, float y, float z);

	void generate(void);
	void upload(void);
	void draw(void);
	void updateMatrices(void);
	
	//variables
	GLuint VAO,VBO,NBO,TBO,IBO,CBO;
	GLuint program, texture0;
	int numberOfIndices;
	
	std::vector<cv::Vec3f> vertexArray, normalArray;
	std::vector<cv::Vec2f> texCoordArray;
	std::vector<cv::Vec3i> indexArray;

	cv::Vec3f position;
	cv::Vec3f scale;

	cv::Mat scaleTrans;

};

}