// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 420 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	Author: Changyu Jiang
	Date: 2017/10/23
*/

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include "pointV.h"
using namespace std;
#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#define GL_CLAMP_TO_EDGE 0x812F

/* interaction*/
int g_iMenuId;
int g_vMousePos[2] = { 0, 0 };
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

/* window size*/
int win_width = 640;
int win_height = 480;

/* textures*/
GLuint textures[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
const int GROUND = 0, FRONT = 1, LEFT = 2, RIGHT = 3, BACK = 4, UP = 5, METAL = 6, WOOD = 7;
char* JPG_GROUND = "images/plains-of-abraham_dn.jpg";
char* JPG_FRONT = "images/plains-of-abraham_ft.jpg";
char* JPG_LEFT = "images/plains-of-abraham_lf.jpg";
char* JPG_RIGHT = "images/plains-of-abraham_rt.jpg";
char* JPG_BACK = "images/plains-of-abraham_bk.jpg";
char* JPG_UP = "images/plains-of-abraham_up.jpg";
char* JPG_METAL = "images/metal.jpg";
char* JPG_WOOD = "images/wood1.jpg";

/* basis matrix */
GLfloat s = 0.5;
GLfloat** m_basis;

/* index of spline, index of point */
int i_spline = 0;
int i_point = 0;

/* spline points */
vector<vector<pointV>> spline_vertices;

/* vector of lookAt parameters */
vector<vector<pointV>> lookAt_eyes;
vector<vector<pointV>> lookAt_centers;
vector<vector<pointV>> lookAt_ups;

/* camera move stride*/
int m_stride = 3;

/* vector of track vertices */
vector<vector<pointV>> track_left;
vector<vector<pointV>> track_right;

const GLfloat WORLD_SIZE = 120.0;
const GLfloat TRACK_ALPHA = 0.06; //alpha for drawing cross-section tube
const GLfloat TRACK_WIDTH = 0.6;
const GLfloat OFFSET = 0.4;
const GLfloat MAX_LINE_LENGTH = 0.1; // subdivsion maxLineLength

/* Object where you can load an image */
cv::Mat3b imageBGR;

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* the spline array */
struct spline *g_Splines;
int g_iNumOfSplines; // total number of splines

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);
	printf("%d\n", g_iNumOfSplines);
	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferRGB = cv::Mat::zeros(480, 640, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	//use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (bufferRGB.step & 3) ? 1 : 4);
	//set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, bufferRGB.step / bufferRGB.elemSize());
	glReadPixels(0, 0, bufferRGB.cols, bufferRGB.rows, GL_RGB, GL_UNSIGNED_BYTE, bufferRGB.data);
	//flip to account for GL 0,0 at lower left
	cv::flip(bufferRGB, bufferRGB, 0);
	//convert RGB to BGR
	cv::Mat3b bufferBGR(bufferRGB.rows, bufferRGB.cols, CV_8UC3);
	cv::Mat3b out[] = { bufferBGR };
	// rgb[0] -> bgr[2], rgba[1] -> bgr[1], rgb[2] -> bgr[0]
	int from_to[] = { 0,2, 1,1, 2,0 };
	mixChannels(&bufferRGB, 1, out, 1, from_to, 3);

	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

/* Function to get a pixel value. Use like PIC_PIXEL macro. 
Note: OpenCV images are in channel order BGR. 
This means that:
chan = 0 returns BLUE, 
chan = 1 returns GREEN, 
chan = 2 returns RED. */
unsigned char getPixelValue(cv::Mat3b& image, int x, int y, int chan)
{
	return image.at<cv::Vec3b>(y, x)[chan];
}

/* Function that does nothing but demonstrates looping through image coordinates.*/
void loopImage(cv::Mat3b& image)
{
	for (int r = 0; r < image.rows; r++) { // y-coordinate
		for (int c = 0; c < image.cols; c++) { // x-coordinate
			for (int channel = 0; channel < 3; channel++) {
				// DO SOMETHING... example usage
				// unsigned char blue = getPixelValue(image, c, r, 0);
				// unsigned char green = getPixelValue(image, c, r, 1); 
				// unsigned char red = getPixelValue(image, c, r, 2); 
			}
		}
	}
}

/* Read an image into memory. 
Set argument displayOn to true to make sure images are loaded correctly.
One image loaded, set to false so it doesn't interfere with OpenGL window.*/
int readImage(char *filename, cv::Mat3b& image, bool displayOn)
{
	std::cout << "reading image: " << filename << std::endl;
	image = cv::imread(filename);
	if (!image.data) // Check for invalid input                    
	{
		std::cout << "Could not open or find the image." << std::endl;
		return 1;
	}

	if (displayOn)
	{
		cv::imshow("TestWindow", image);
		cv::waitKey(0); // Press any key to enter. 
	}
	return 0;
}

/* OpenCV help:
Access number of rows of image (height): image.rows; 
Access number of columns of image (width): image.cols;
Pixel 0,0 is the upper left corner. Byte order for 3-channel images is BGR. 
*/

void texload(int i, char *filename)
{
	cv::Mat3b bufferBGR;
	readImage(filename, bufferBGR, false);

	int outputImageWidth = 256;
	int outputImageHeight = 256;

	// Resize image with bilinear interpolation. Note this does not maintain aspect ratio:
	// To automatically resize image to outputImageWidth x outputImageHeight upon loading.
	// Uncomment the next line below:
	cv::resize(bufferBGR, bufferBGR,
		cv::Size(outputImageWidth, outputImageHeight), 0.0, 0.0, CV_INTER_LINEAR);

	// OR, crop image with below:
	// Crop from upper left of image [pixel (0,0) in OpenCV], at width x height.
	cv::Rect cropRegion = cv::Rect(0, 0, outputImageWidth, outputImageHeight);
	if (bufferBGR.rows >= outputImageHeight && bufferBGR.cols >= outputImageWidth) {
		bufferBGR = bufferBGR(cropRegion);
	}
	else {
		// If crop fails, default to resize the image.
		cv::resize(bufferBGR, bufferBGR,
			cv::Size(outputImageWidth, outputImageHeight), 0.0, 0.0, CV_INTER_LINEAR);
	}

	// Flip up-down to account for CV / OpenGL 0,0 pixel location.
	// This is why the row traversal is in reverse.
	unsigned char* rgb_buffer = new unsigned char[bufferBGR.rows*bufferBGR.cols * 3]();
	int pixlocation = 0;
	for (int r = bufferBGR.rows - 1; r >= 0; r--) {
		for (int c = 0; c < bufferBGR.cols; c++) {
			rgb_buffer[pixlocation] = bufferBGR.at<cv::Vec3b>(r, c)[2];		// R
			rgb_buffer[pixlocation + 1] = bufferBGR.at<cv::Vec3b>(r, c)[1]; // G
			rgb_buffer[pixlocation + 2] = bufferBGR.at<cv::Vec3b>(r, c)[0]; // B
			pixlocation += 3;
		}
	}

	// Gl texture calls
	glBindTexture(GL_TEXTURE_2D, textures[i]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bufferBGR.cols, bufferBGR.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);
	
	/* Texture parameters, slove the sky box seams */ 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Free memory
	delete[] rgb_buffer;
}

/* helper func: matrix multiplication*/
void mathMatrixMultiplication(GLfloat** matL, GLfloat** matR, GLfloat** res, int rowL, int colL, int colR) {
	// init res mat
	for (int i = 0; i < rowL; i++) {
		for (int j = 0; j < colR; j++) {
			res[i][j] = 0;
		}
	}
	// compute 
	for (int i = 0; i < rowL; i++) {
		for (int j = 0; j < colR; j++) {
			for (int k = 0; k < colL; k++) {
				res[i][j] += matL[i][k] * matR[k][j];
			}
		}
	}
}

/* helper func: compute length of two point */
GLfloat length(pointV p0, pointV p1) {
	return (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y) + (p1.z - p0.z) * (p1.z - p0.z);
}

/* helper func: compute cross product of two vector*/
pointV crossProduct(pointV p0, pointV p1) {
	pointV res;
	res.x = p0.y * p1.z - p0.z * p1.y;
	res.y = p0.z * p1.x - p0.x * p1.z;
	res.z = p0.x * p1.y - p0.y * p1.x;
	return res;
}

/* Catmull-Rom point calculation */
pointV mathCatmullRomPoint(pointV input, int splineIndex, int controlPointIndex, GLfloat u)
{
	// init parameter vector
	GLfloat** parameterV = new GLfloat*[1];
	parameterV[0] = new GLfloat[4];
	parameterV[0][0] = pow(u, 3);
	parameterV[0][1] = pow(u, 2);
	parameterV[0][2] = pow(u, 1);
	parameterV[0][3] = 1;
	
	// init control matrix
	GLfloat** controlM = new GLfloat*[4];	
	GLfloat** intermediateM = new GLfloat*[4];
	for (int i = 0; i < 4; i++) {
		controlM[i] = new GLfloat[3];
		controlM[i][0] = g_Splines[splineIndex].points[controlPointIndex + i].x;
		controlM[i][1] = g_Splines[splineIndex].points[controlPointIndex + i].y;
		controlM[i][2] = g_Splines[splineIndex].points[controlPointIndex + i].z;
		intermediateM[i] = new GLfloat[3];
	}

	// init resulting point vector
	GLfloat** pointV = new GLfloat*[1];
	pointV[0] = new GLfloat[3];

	// compute v
	mathMatrixMultiplication(m_basis, controlM, intermediateM, 4, 4, 3);
	mathMatrixMultiplication(parameterV, intermediateM, pointV, 1, 4, 3);

	input.x = pointV[0][0];
	input.y = pointV[0][1];
	input.z = pointV[0][2];

	// free matrices
	for (int i = 0; i < 4; i++) {
		delete[] intermediateM[i];
		delete[] controlM[i];
	}
	delete[] intermediateM;
	delete[] controlM;
	delete[] pointV[0];
	delete[] pointV;
	delete[] parameterV[0];
	delete[] parameterV;

	return input;
}

/* Catmull-Rom point tagent calculation */
pointV mathCatmullRomTagent(pointV input, int splineIndex, int controlPointIndex, GLfloat u)
{
	// init parameter vector
	GLfloat** parameterV = new GLfloat*[1];
	parameterV[0] = new GLfloat[4];
	parameterV[0][0] = 3 * pow(u, 2);
	parameterV[0][1] = 2 * pow(u, 1);
	parameterV[0][2] = 1;
	parameterV[0][3] = 0;

	// init control matrix
	GLfloat** controlM = new GLfloat*[4];
	GLfloat** intermediateM = new GLfloat*[4];
	for (int i = 0; i < 4; i++) {
		controlM[i] = new GLfloat[3];
		controlM[i][0] = g_Splines[splineIndex].points[controlPointIndex + i].x;
		controlM[i][1] = g_Splines[splineIndex].points[controlPointIndex + i].y;
		controlM[i][2] = g_Splines[splineIndex].points[controlPointIndex + i].z;
		intermediateM[i] = new GLfloat[3];
	}

	// init resulting point vector
	GLfloat** pointV = new GLfloat*[1];
	pointV[0] = new GLfloat[3];

	// compute v  
	mathMatrixMultiplication(m_basis, controlM, intermediateM, 4, 4, 3);
	mathMatrixMultiplication(parameterV, intermediateM, pointV, 1, 4, 3);

	input.x = pointV[0][0];
	input.y = pointV[0][1];
	input.z = pointV[0][2];

	// free matrices
	for (int i = 0; i < 4; i++) {
		delete[] intermediateM[i];
		delete[] controlM[i];
	}
	delete[] intermediateM;
	delete[] controlM;
	delete[] pointV[0];
	delete[] pointV;
	delete[] parameterV[0];
	delete[] parameterV;

	return input;
}

/**
* Level 4 & 5 realization
*/
void initRideAndRail(int sIndex, int pIndex, GLfloat u)
{	
	/**
	* level 4 the ride 
	* reference for camera movement:http://run.usc.edu/cs420-s14/assignments/assign2/assign2_camera.html
	*/
	/* tangent */
	pointV t;
	/* normal */
	pointV n;
	/* binormal */
	pointV b;
	/* arbitary V */
	pointV v(0, 1.0, 0); 

	t = mathCatmullRomTagent(pointV(), sIndex, pIndex, u);
	t.normalize();

	if (spline_vertices[sIndex].size() == 1) {
		// N0 = unit(T0 * V)
		n = crossProduct(t, v);
		n.normalize();
	} else {
		// N1 = unit(B0 * V)
		n = crossProduct(lookAt_ups[sIndex].back(), t);
		n.normalize();
	}
	// B = unit(T * N)
	b = crossProduct(t, n);
	b.normalize();
	// set lookAt func parameters
	pointV eye = spline_vertices[sIndex].back() + b * OFFSET;
	lookAt_eyes[sIndex].push_back(eye);
	lookAt_centers[sIndex].push_back(eye + t);
	lookAt_ups[sIndex].push_back(b);

	/**	
	* level 5 rail cross-section
	* compute track tube
	* reference: http://run.usc.edu/cs420-s14/assignments/assign2/csci480_assign2_crossSection.pdf
	*/
	pointV p = spline_vertices[sIndex].back();
	/* draw two tracks here*/
	pointV pLeft = p + n * (TRACK_WIDTH / 2.0);
	pointV pRight = p - n * (TRACK_WIDTH / 2.0);
	/* compute each verts following the reference*/
	track_left[sIndex].push_back(pLeft + ((n + b) * TRACK_ALPHA));		// top left
	track_left[sIndex].push_back(pLeft + ((-n + b) * TRACK_ALPHA));		// top right
	track_left[sIndex].push_back(pLeft + ((-n - b) * TRACK_ALPHA));		// bot right
	track_left[sIndex].push_back(pLeft + ((n - b) * TRACK_ALPHA));		// bot left
	track_right[sIndex].push_back(pRight + ((n + b) * TRACK_ALPHA));	// top left
	track_right[sIndex].push_back(pRight + ((-n + b) * TRACK_ALPHA));	// top right
	track_right[sIndex].push_back(pRight + ((-n - b) * TRACK_ALPHA));	// bot right
	track_right[sIndex].push_back(pRight + ((n - b) * TRACK_ALPHA));	// bot left
}

/**
* Level 1 realziation with recusive subdivision method
* Reference: http://run.usc.edu/cs420-s14/assignments/assign2/hw2Hints.pdf
*/
void subDivide(int sIndex, int pIndex, GLfloat u0, GLfloat u1, GLfloat maxLineLength)
{	
	GLfloat umid = (u0 + u1) / 2.0;
	pointV x0 = mathCatmullRomPoint(pointV(), sIndex, pIndex, u0);
	pointV x1 = mathCatmullRomPoint(pointV(), sIndex, pIndex, u1);
	if (length(x0, x1) > maxLineLength * maxLineLength) {
		subDivide(sIndex, pIndex, u0, umid, maxLineLength);
		subDivide(sIndex, pIndex, umid, u1, maxLineLength);
	} else {
		if (spline_vertices[sIndex].empty() || spline_vertices[sIndex].back().x != x0.x || spline_vertices[sIndex].back().y != x0.y || spline_vertices[sIndex].back().z != x0.z) {
			spline_vertices[sIndex].push_back(x0);
			initRideAndRail(sIndex, pIndex, u0);
		}
		spline_vertices[sIndex].push_back(x1);
		initRideAndRail(sIndex, pIndex, u1);
	}
}

/* helper func: load textures */
void initTexture() {
	glGenTextures(1, &textures[FRONT]);
	texload(FRONT, JPG_FRONT);
	glGenTextures(1, &textures[BACK]);
	texload(BACK, JPG_BACK);
	glGenTextures(1, &textures[GROUND]);
	texload(GROUND, JPG_GROUND);
	glGenTextures(1, &textures[LEFT]);
	texload(LEFT, JPG_LEFT);
	glGenTextures(1, &textures[RIGHT]);
	texload(RIGHT, JPG_RIGHT);
	glGenTextures(1, &textures[UP]);
	texload(UP, JPG_UP);
	glGenTextures(1, &textures[METAL]);
	texload(METAL, JPG_METAL);
	glGenTextures(1, &textures[WOOD]);
	texload(WOOD, JPG_WOOD);
}

/**
* Level 1 init
*/
void initSpline() {
	for (int i = 0; i < g_iNumOfSplines; i++)
	{
		printf("Num control points in spline %d: %d.\n", i, g_Splines[i].numControlPoints);
	}
	/* init basis matrix  with s set to s = 1/2 */
	m_basis = new GLfloat*[4];
	for (int i = 0; i < 4; i++) {
		m_basis[i] = new GLfloat[4];
	}
	m_basis[0][0] = -s;		m_basis[0][1] = 2.0-s;	m_basis[0][2] = s-2.0;		m_basis[0][3] = s;
	m_basis[1][0] = 2.0*s;	m_basis[1][1] = s-3.0;	m_basis[1][2] = 3.0-2.0*s;	m_basis[1][3] = -s;
	m_basis[2][0] = -s;		m_basis[2][1] = 0;		m_basis[2][2] = s;			m_basis[2][3] = 0;
	m_basis[3][0] = 0;		m_basis[3][1] = 1;		m_basis[3][2] = 0;			m_basis[3][3] = 0;
	
	/* generate spline points */
	for (int i = 0; i < g_iNumOfSplines; i++) {
		spline_vertices.push_back(vector<pointV>());
		lookAt_eyes.push_back(vector<pointV>());
		lookAt_centers.push_back(vector<pointV>());
		lookAt_ups.push_back(vector<pointV>());
		track_left.push_back(vector<pointV>());
		track_right.push_back(vector<pointV>());
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			subDivide(i, j, 0.0, 1.0, MAX_LINE_LENGTH);
		}
	}
}

void myinit()
{	
	glEnable(GL_TEXTURE_2D);
	initTexture();
	initSpline();
	/* setup gl view */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, win_width / win_height, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);

	/* setup light & material*/
	GLfloat light_position[] = { 0.5, 1.0, 0.5, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

	GLfloat no_mat[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat low_shininess[] = { 60.0 };

	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, no_mat);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glColorMaterial(GL_FRONT, GL_SPECULAR);
	glEnable(GL_COLOR_MATERIAL);
}

/* draw tracks with pre-computed verts */
void drawTrack(vector<vector<pointV>> track)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[METAL]);
	for (int i = 0; i < g_iNumOfSplines; i++) {
		if (spline_vertices[i].size() >= 1) {
			// first
			glBegin(GL_POLYGON);
			glVertex3f(track[i][0].x, track[i][0].y, track[i][0].z);
			glVertex3f(track[i][1].x, track[i][1].y, track[i][1].z);
			glVertex3f(track[i][2].x, track[i][2].y, track[i][2].z);
			glVertex3f(track[i][3].x, track[i][3].y, track[i][3].z);
			glEnd();
			// in-between
			for (int j = 4; j < track[i].size(); j += 4) {
				glBegin(GL_TRIANGLE_STRIP);
				// top face
				glTexCoord2f(0.0, 0.0);
				glVertex3f(track[i][j].x, track[i][j].y, track[i][j].z);
				glTexCoord2f(0.0, 1.0);
				glVertex3f(track[i][j - 4].x, track[i][j - 4].y, track[i][j - 4].z);
				glTexCoord2f(1.0, 0.0);
				glVertex3f(track[i][j + 1].x, track[i][j + 1].y, track[i][j + 1].z);
				glTexCoord2f(1.0, 1.0);
				glVertex3f(track[i][j - 3].x, track[i][j - 3].y, track[i][j - 3].z);
				// right face
				glVertex3f(track[i][j + 2].x, track[i][j + 2].y, track[i][j + 2].z);
				glVertex3f(track[i][j - 2].x, track[i][j - 2].y, track[i][j - 2].z);
				// bottom face
				glVertex3f(track[i][j + 3].x, track[i][j + 3].y, track[i][j + 3].z);
				glVertex3f(track[i][j - 1].x, track[i][j - 1].y, track[i][j - 1].z);
				// left face
				glVertex3f(track[i][j].x, track[i][j].y, track[i][j].z);
				glVertex3f(track[i][j - 4].x, track[i][j - 4].y, track[i][j - 4].z);
				glEnd();
			}
			// last
			glBegin(GL_POLYGON);
			glVertex3f(track[i][track[i].size() - 4].x, track[i][track[i].size() - 4].y, track[i][track[i].size() - 4].z);
			glVertex3f(track[i][track[i].size() - 3].x, track[i][track[i].size() - 3].y, track[i][track[i].size() - 3].z);
			glVertex3f(track[i][track[i].size() - 2].x, track[i][track[i].size() - 2].y, track[i][track[i].size() - 2].z);
			glVertex3f(track[i][track[i].size() - 1].x, track[i][track[i].size() - 1].y, track[i][track[i].size() - 1].z);
			glEnd();
		}
	}
}

/* draw crossbars with pre-computed verts */
void drawCrossbar(vector<vector<pointV>> trackL, vector<vector<pointV>> trackR)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[WOOD]);
	for (int i = 0; i < g_iNumOfSplines; i++) {
		if (spline_vertices[i].size() >= 1) {
			for (int j = 4; j < trackL[i].size() - 12; j += 4 * 4) {
				glBegin(GL_TRIANGLE_STRIP);
				// top face
				glTexCoord2f(0.0, 0.0);
				glVertex3f(trackL[i][j + 1].x, trackL[i][j + 1].y, trackL[i][j + 1].z);
				glTexCoord2f(0.0, 1.0);
				glVertex3f(trackL[i][j + 5].x, trackL[i][j + 5].y, trackL[i][j + 5].z);
				glTexCoord2f(1.0, 1.0);
				glVertex3f(trackR[i][j + 4].x, trackR[i][j + 4].y, trackR[i][j + 4].z);
				glTexCoord2f(1.0, 0.0);
				glVertex3f(trackR[i][j].x, trackR[i][j].y, trackR[i][j].z);
				// front face
				glVertex3f(trackL[i][j + 2].x, trackL[i][j + 2].y, trackL[i][j + 2].z);
				glVertex3f(trackR[i][j + 3].x, trackR[i][j + 3].y, trackR[i][j + 3].z);
				// bottom face
				glVertex3f(trackL[i][j + 6].x, trackL[i][j + 6].y, trackL[i][j + 6].z);
				glVertex3f(trackR[i][j + 7].x, trackR[i][j + 7].y, trackR[i][j + 7].z);
				// back face
				glVertex3f(trackL[i][j + 5].x, trackL[i][j + 5].y, trackL[i][j + 5].z);
				glVertex3f(trackR[i][j + 4].x, trackR[i][j + 4].y, trackR[i][j + 4].z);
				glEnd();
			}
		}
	}
}

/**
* Level 2 & 3 realization
*/
void drawGroundSky()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	//GROUND
	glBindTexture(GL_TEXTURE_2D, textures[GROUND]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0, 0);	glVertex3f(-WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(0, 1);	glVertex3f(-WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 1);	glVertex3f(WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 0);	glVertex3f(WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glEnd();
	//up
	glBindTexture(GL_TEXTURE_2D, textures[UP]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(0, 1);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 1);	glVertex3f(WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 0);	glVertex3f(WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glEnd();
	//Back
	glBindTexture(GL_TEXTURE_2D, textures[BACK]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);	glVertex3f(-WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(0, 1);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 1);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 0);	glVertex3f(-WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glEnd();
	//Front
	glBindTexture(GL_TEXTURE_2D, textures[FRONT]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(0, 1); glVertex3f(WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 1); glVertex3f(WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 0);	glVertex3f(WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glEnd();
	//Right
	glBindTexture(GL_TEXTURE_2D, textures[RIGHT]);
	glBegin(GL_QUADS);	
	glTexCoord2f(0, 0);	glVertex3f(-WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(0, 1);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 1); glVertex3f(WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
	glTexCoord2f(1, 0); glVertex3f(WORLD_SIZE, WORLD_SIZE, -WORLD_SIZE);
	glEnd();
	//Back
	glBindTexture(GL_TEXTURE_2D, textures[LEFT]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);	glVertex3f(-WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(0, 1); glVertex3f(-WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 1); glVertex3f(WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
	glTexCoord2f(1, 0); glVertex3f(WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}


void display()
{
	// Clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Set camera
	gluLookAt(
		lookAt_eyes[i_spline][i_point].x,		lookAt_eyes[i_spline][i_point].y,		lookAt_eyes[i_spline][i_point].z,
		lookAt_centers[i_spline][i_point].x,	lookAt_centers[i_spline][i_point].y,	lookAt_centers[i_spline][i_point].z,
		lookAt_ups[i_spline][i_point].x,		lookAt_ups[i_spline][i_point].y,		lookAt_ups[i_spline][i_point].z
	);
	// move camera
	i_point += m_stride;

	// if camera move to the end of cur track
	if (i_point >= spline_vertices[i_spline].size())
	{
		i_point = 0;
		i_spline++;
		i_spline = i_spline >= spline_vertices.size()? 0 : i_spline;
	}

	// draw sky box
	drawGroundSky();
	// draw two tracks
	drawTrack(track_left);
	drawTrack(track_right);
	// draw croos bar
	drawCrossbar(track_left, track_right);

	glutSwapBuffers();
}

// menu func
void menufunc(int value)
{
	switch (value)
	{
	case 0:
		exit(0);
		break;
	}
}

// idle func
void doIdle()
{
	glutPostRedisplay();
}

bool screenshot = true;
/* count screen shot*/
int numScreenshot = 0;
int fps = 15; // desired frame rate

void myTimer(int v)
{
	if (screenshot) {
		char filename[30];
		sprintf(filename, ".\\screenshots\\%03d.jpg", numScreenshot);
		saveScreenshot(filename);
		numScreenshot++;
		if (numScreenshot == 1000) screenshot = false;
	}
	glutTimerFunc(1000 / fps, myTimer, v);
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}
	
	loadSplines(argv[1]);

	glutInit(&argc, (char**)argv);

	/* Init window */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(150, 150);
	glutCreateWindow("Roller Coaster!! Hahaha!!!!");

	glEnable(GL_DEPTH_TEST);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* allow the user to quit using the right mouse button menu */
	int g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* callback for animation */
	glutIdleFunc(doIdle);

	/* callback for keyboard input */
	//glutKeyboardFunc(keyboard);

	// If you need to load textures use below instead of pic library:
	// readImage("spiral.jpg", imageBGR, true);

	// Demonstrates to loop across image and access pixel values:
	// Note this function doesn't do anything, but you may find it useful:
	// loopImage(imageBGR);

	// Rebuilt save screenshot function, but will seg-fault unless you have
	// OpenGL framebuffers initialized. You can use this new function as before:
	// saveScreenshot("test_screenshot.jpg");
	glutTimerFunc(100, myTimer, fps);

	myinit();
	glutMainLoop();

	return 0;
}
