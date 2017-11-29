/*
CSCI 480
Assignment 3 Raytracer

Name: <Your name here>
*/

#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

/************************** MACRO *****************************************/
// image size
#define WIDTH 640
#define HEIGHT 480

// objects limits
#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

// display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

// the field of view of the camera
#define FOV 60.0

// anti aliasing
#define SUPERSAMPLING_LEVEL 3
#define SOFT_SHADOW_LIMITS  15
double SOFT_SHADOW_OFFSET = 0.1;
/************************** STRUCT *****************************************/
struct Vector {
	double x;
	double y;
	double z;

	Vector()
		: x(0.0), y(0.0), z(0.0)
	{}

	Vector(double X, double Y, double Z)
		:x(X), y(Y), z(Z)
	{}

	bool operator==(const Vector& rhs)
	{
		return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
	}

	Vector operator-(const Vector& rhs)
	{
		return Vector(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
	}

	Vector operator+(const int rhs)
	{
		return Vector(this->x + rhs, this->y + rhs, this->z + rhs);
	}

	Vector operator+(const Vector& rhs)
	{
		return Vector(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
	}

	Vector operator*(const Vector& rhs)
	{
		return Vector(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
	}

	Vector operator/(const Vector& rhs)
	{
		return Vector(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
	}

	Vector operator*(const double rhs)
	{
		return Vector(x * rhs, y * rhs, z * rhs);
	}

	void operator=(const Vector &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
	}

	void operator+=(const Vector& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
	}

	void operator/=(const int rhs)
	{
		x /= rhs;
		y /= rhs;
		z /= rhs;
	}

	void normalize() {
		double length = sqrt(x * x + y * y + z * z); 
		x /= length;
		y /= length;
		z /= length;
	}
};

struct Vertex
{
	Vector position;
	Vector color_diffuse;
	Vector color_specular;
	Vector normal;
	double shininess;
};

typedef struct _Triangle
{
	struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
	Vector position;
	Vector color_diffuse;
	Vector color_specular;
	double shininess;
	double radius;
} Sphere;

struct Light
{
	Vector position;
	Vector color;
};

struct Ray {
	Vector origin;
	Vector direction;

	Ray()
		:origin(), direction()
	{}

	Ray(Vector ori, Vector dir)
		:origin(ori), direction(dir)
	{}
};

/************************** VARIABLES *****************************************/

unsigned char buffer[HEIGHT][WIDTH][3];
char *filename = 0;
int mode = MODE_DISPLAY;

// objects
Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
Vector ambient_light;

// num of objects
int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

// camera pos
Vector cam_pos(0.0, 0.0, 0.0);

// viewport limit
double viewPort_minX, viewPort_maxX, viewPort_minY, viewPort_maxY;
double viewPort_width, viewPort_height;

Vector** pixels;

const double PI = 3.141592653;

Vector softShadowBias[15] =
{
	Vector(0.0, 0.0, 0.0),
	Vector(SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET),
	Vector(-SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET),
	Vector(SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET),
	Vector(-SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET),
	Vector(SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET),
	Vector(-SOFT_SHADOW_OFFSET, SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET),
	Vector(SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET),
	Vector(-SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET, -SOFT_SHADOW_OFFSET),
	Vector(SOFT_SHADOW_OFFSET, 0.0, 0.0),
	Vector(-SOFT_SHADOW_OFFSET, 0.0, 0.0),
	Vector(0.0, SOFT_SHADOW_OFFSET, 0.0),
	Vector(0.0, -SOFT_SHADOW_OFFSET, 0.0),
	Vector(0.0, 0.0, SOFT_SHADOW_OFFSET),
	Vector(0.0, 0.0, -SOFT_SHADOW_OFFSET)
};


/************************** METHODS *****************************************/
// helper methods
Vector power(Vector v, int i);
Vector cross(Vector a, Vector b);
Vector get_ray_pos_at_length(Ray thisRay, double t);
Vector get_reflected_vec(Vector i, Vector n);
Vector average_color(int x, int y);
double dot(Vector a, Vector b);
double distance(Vector a, Vector b);
void clamp(Vector &color);
void save_jpg();
void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

// functional methods
void draw_scene();
void init_viewport();
Ray fire_ray(double y, double x);
void trace_ray(Ray ray, Vector &color);
bool is_intersected_with_light(Ray ray, int index, double &distance);
bool is_intersected_with_sphere(Ray ray, int index, double &distance);
bool is_interesected_with_triangle(Ray ray, int index, double &distance, double &alpha, double &beta, double &gamma);
void get_phong_color(Vector intersection, Vector normal, Vector viewer, Vector &phongColor, Vector kd, Vector ks, double sh);

/************************** METHODS IMPLEMENTATION *****************************************/

void draw_scene()
{
	// init viewport parameters
	init_viewport();

	// allacate memory for saving pixel value
	pixels = new Vector* [HEIGHT * SUPERSAMPLING_LEVEL];
	for (int i = 0; i < HEIGHT * SUPERSAMPLING_LEVEL; i++) {
		pixels[i] = new Vector [WIDTH * SUPERSAMPLING_LEVEL];
	}

	// fire ray & trace ray
	for (int i = 0; i < HEIGHT * SUPERSAMPLING_LEVEL; i++) { // y axis 
		for (int j = 0; j < WIDTH * SUPERSAMPLING_LEVEL; j++) { // x axis
			Ray ray = fire_ray(i, j);
			trace_ray(ray, pixels[i][j]);
		}
	}
	
	// simple image output
	for (int x = 0; x < WIDTH; x++) {
		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (int y = 0; y < HEIGHT; y++) { 
			Vector color = average_color(y, x);
			plot_pixel(x, y, color.x, color.y, color.z);
		}
		glEnd();
		glFlush();
	}
	
	if (mode == MODE_JPEG)
	{
		save_jpg();
	}
	printf("Done!\n"); fflush(stdout);
}

void init_viewport() {
	double aspect_ratio = (double) WIDTH / (double) HEIGHT;
	double fov = (double) FOV;

	viewPort_maxY = tan(fov / 2 * PI / 180);
	viewPort_minY = -viewPort_maxY;
	viewPort_maxX = aspect_ratio * viewPort_maxY;
	viewPort_minX = -viewPort_maxX;

	viewPort_width = viewPort_maxX * 2;
	viewPort_height = viewPort_maxY * 2;
}

// fire ray
Ray fire_ray(double y, double x) {
	double Y = viewPort_minY + y * viewPort_height / (HEIGHT * SUPERSAMPLING_LEVEL);
	double X = viewPort_minX + x * viewPort_width / (WIDTH * SUPERSAMPLING_LEVEL);
	Vector pixelPos(X, Y, -1.0);
	// ray direction
	Vector ray_dir = pixelPos - cam_pos;
	ray_dir.normalize();
	return Ray(cam_pos, ray_dir);
}

// get average color, if supersampling
Vector average_color(int y, int x) {
	Vector color;
	for (int i = 0; i < SUPERSAMPLING_LEVEL; i++) {
		for (int j = 0; j < SUPERSAMPLING_LEVEL; j++) {
			color += pixels[(y * SUPERSAMPLING_LEVEL) + j][(x * SUPERSAMPLING_LEVEL) + i];
		}
	}
	color /= SUPERSAMPLING_LEVEL * SUPERSAMPLING_LEVEL;
	return color;
}

// get nearest object
void get_nearest_obj(Ray &ray, bool &intersected, int &obj_type, int &obj_id, double &nearest_distance) {
	// if intersected with light source
	for (int i = 0; i < num_lights; i++) {
		double cur_distance = DBL_MAX;
		if (is_intersected_with_light(ray, i, cur_distance)) {
			if (cur_distance < nearest_distance) {
				intersected = true;
				nearest_distance = cur_distance;
				obj_type = 1;
				obj_id = i;
			}
		}
	}
	// if intersected with spheres
	for (int i = 0; i < num_spheres; i++) {
		double cur_distance = DBL_MAX;
		if (is_intersected_with_sphere(ray, i, cur_distance)) {
			if (cur_distance < nearest_distance) {
				intersected = true;
				nearest_distance = cur_distance;
				obj_type = 2;
				obj_id = i;
			}
		}
	}
	// if intersected with triangles
	for (int i = 0; i < num_triangles; i++) {
		double alpha, beta, gamma;
		double cur_distance = DBL_MAX;
		if (is_interesected_with_triangle(ray, i, cur_distance, alpha, beta, gamma)) {
			if (cur_distance < nearest_distance) {
				intersected = true;
				nearest_distance = cur_distance;
				obj_type = 3;
				obj_id = i;
			}
		}
	}
};

// determin phong color for sphere
void get_sphere_color(Ray &ray, int obj_id, Vector &color) {
	double cur_distance = DBL_MAX;
	if (is_intersected_with_sphere(ray, obj_id, cur_distance)) {
		Vector intersection = get_ray_pos_at_length(ray, cur_distance);
		Vector normal = intersection - spheres[obj_id].position;
		normal.normalize();

		Vector phong(0.0, 0.0, 0.0);
		// coefficient kd and ks for diffuse and specular
		Vector kd = spheres[obj_id].color_diffuse;
		Vector ks = spheres[obj_id].color_specular;
		// shiniess
		double sh = spheres[obj_id].shininess;
		// viewer vector
		Vector viewer = ray.direction * -1;
		viewer.normalize();
		get_phong_color(intersection, normal, viewer, phong, kd, ks, sh);
		color = phong * 255.0;
	}
}

void get_triangle_color(Ray &ray, int obj_id, Vector &color) {
	//Vector normal;
	double alpha, beta, gamma;
	double distance_to_triangle = DBL_MAX;
	if (is_interesected_with_triangle(ray, obj_id, distance_to_triangle, alpha, beta, gamma)) {
			Vector intersection = get_ray_pos_at_length(ray, distance_to_triangle);
			Triangle triangle = triangles[obj_id];
			Vertex p0 = triangles[obj_id].v[0];
			Vertex p1 = triangles[obj_id].v[1];
			Vertex p2 = triangles[obj_id].v[2];
			// average the normal Vector
			Vector normal;
			normal.x = p0.normal.x * alpha + p1.normal.x * beta + p2.normal.x * gamma;
			normal.y = p0.normal.y * alpha + p1.normal.y * beta + p2.normal.y * gamma;
			normal.z = p0.normal.z * alpha + p1.normal.z * beta + p2.normal.z * gamma;
			// average coefficient kd and ks for diffuse and specular
			Vector kd, ks;
			kd.x = p0.color_diffuse.x * alpha + p1.color_diffuse.x * beta + p2.color_diffuse.x * gamma;
			kd.y = p0.color_diffuse.y * alpha + p1.color_diffuse.y * beta + p2.color_diffuse.y * gamma;
			kd.z = p0.color_diffuse.z * alpha + p1.color_diffuse.z * beta + p2.color_diffuse.z * gamma;
			ks.x = p0.color_specular.x * alpha + p1.color_specular.x * beta + p2.color_specular.x * gamma;
			ks.y = p0.color_specular.y * alpha + p1.color_specular.y * beta + p2.color_specular.y * gamma;
			ks.z = p0.color_specular.z * alpha + p1.color_specular.z * beta + p2.color_specular.z * gamma;
			// average shininess of the sphere
			double shininess = p0.shininess * alpha + p1.shininess * beta + p2.shininess * gamma;
			// viewr vector
			Vector viewer = ray.direction * -1;
			viewer.normalize();
			// calculate color
			Vector phong(0.0, 0.0, 0.0);
			get_phong_color(intersection, normal, viewer, phong, kd, ks, shininess);
			color = phong * 255.0;
	}
}

void trace_ray(Ray ray, Vector &color) {
	// init
	bool intersected = false;
	// 1 light source 2 sphere 3 triangle
	int obj_type = -1;
	int obj_id = -1;
	double nearest_distance = DBL_MAX;

	// find the nearest intersected object
	get_nearest_obj(ray, intersected, obj_type, obj_id, nearest_distance);

	if (!intersected) {
		color = Vector(255.0, 255.0, 255.0);
		return;
	}

	// calculate color
	if (obj_type == 1) {
		color = lights[obj_id].color * 255.0;
	}
	else if (obj_type == 2) {
		get_sphere_color(ray, obj_id, color);
	}
	else if (obj_type == 3) {
		get_triangle_color(ray, obj_id, color);
	}

	color += ambient_light * 255;
	clamp(color);
}

void get_phong_color(Vector intersection, Vector normal, Vector v, Vector &color, Vector kd, Vector ks, double sh) {
	// fire a shadow ray to each light source
	// check if in shadow
	for (int i = 0; i < num_lights; i++) {
		Vector temp_color;
		for (int j = 0; j < SOFT_SHADOW_LIMITS; j++) {
			bool in_shadow = false;
			Vector light_source_pos = lights[i].position + softShadowBias[j];
			Vector origin = intersection;
			Vector direction = light_source_pos - origin;
			direction.normalize();

			Ray shadowRay(origin, direction);
			double distance_to_light = distance(light_source_pos, origin);

			// if there is spheres before light source
			for (int i = 0; i < num_spheres; i++) {
				double cur_distance = DBL_MAX;
				if (is_intersected_with_sphere(shadowRay, i, cur_distance)) {
					if (cur_distance <= distance_to_light) {
						in_shadow = true;
					}
				}
			}
			// if there is triangle before light source
			for (int i = 0; i < num_triangles; i++) {
				double cur_distance = DBL_MAX, alpha, beta, gamma;
				if (is_interesected_with_triangle(shadowRay, i, cur_distance, alpha, beta, gamma)) {
					if (cur_distance <= distance_to_light) {
						in_shadow = true;
					}
				}
			}
			if (in_shadow) {
				continue;
			}

			// diffuse part 
			double LN = dot(direction, normal);
			LN = LN < 0.0 ? 0.0 : LN;

			// specular part
			Vector r = get_reflected_vec(direction, normal);
			r.normalize();
			double RV = dot(r, v);
			RV = RV < 0.0 ? 0.0 : RV;

			temp_color += lights[i].color * (kd * LN + ks * pow(RV, sh));
		}
		temp_color /= 1.0 * SOFT_SHADOW_LIMITS;
		color += temp_color;
	}
}

// if ray intersected with light source
bool is_intersected_with_light(Ray ray, int index, double &distance) {
	if (lights[index].position == ray.origin) {
		return false;
	}
	Vector dis = (lights[index].position - ray.origin) / ray.direction;
	if (dis.x == dis.y && dis.y == dis.z) {
		distance = dis.x;
		return true;
	}
	return false;
}

// if the Ray is intersected with sphere
bool is_intersected_with_sphere(Ray ray, int index, double &distance) {
	double r = spheres[index].radius;
	// b = 2(xd(x0-xc) + yd(y0-yc) + zd(z0-zc))
	Vector B = ray.direction * (ray.origin - spheres[index].position);
	double b = 2.0 * (B.x + B.y + B.z);
	// c = (x0- xc)^2 + (y0- yc)^2 + (z0- zc)^2 - r ^ 2
	Vector C = power(ray.origin - spheres[index].position, 2);
	double c = C.x + C.y + C.z - pow(r, 2.0);
	
	// a = 1
	double delta = pow(b, 2.0) - 4.0 * c;
	if (delta < 0.0) {
		return false;
	}

	// slove t0 and t1
	double t0 = (-b + sqrt(delta)) / 2;
	double t1 = (-b - sqrt(delta)) / 2;
	// if both negative, abort
	if (t0 <= 0 && t1 <= 0) {
		return false;
	} else if (t0 > 0 && t1 > 0) {
		distance = std::min(t0, t1);
	} else {
		distance = std::max(t0, t1); 
	}
	if (distance < 0.0001) { 
		return false; 
	}
	return true;
}

// check if ray intersected with a specified triangle
bool is_interesected_with_triangle(Ray ray, int index, double &t, double &alpha, double &beta, double &gamma) {
	/* 
		Step1: Ray-plane intersction check
	*/
	// get three vertices
	Vector p1 = triangles[index].v[0].position;
	Vector p2 = triangles[index].v[1].position;
	Vector p3 = triangles[index].v[2].position;
	
	// plane normal
	Vector normal = cross(p2 - p1, p3 - p2);
	normal.normalize();

	double ND = dot(normal, ray.direction);
	// if n.d == 0, parallel to plane
	if (ND == 0.0) {
		return false;
	}
	// t = -( n.p0 + d / n.d )
	t = -1.0 * (dot(ray.origin - p1, normal)) / ND;
	// if t <= 0, behind ray origin
	if (t <= 0.001) { 
		return false;
	}

	/*
		Step2: Point in triangle testing
	*/
	// barycentric method
	// area ABC = 1/2 |(B-A) cross (C-A)|
	Vector p = get_ray_pos_at_length(ray, t);
	double a = dot(cross(p2 - p1, p3 - p1), normal);
	double a1 = dot(cross(p2 - p1, p - p1), normal);
	double a2 = dot(cross(p3 - p2, p - p2), normal);
	double a3 = dot(cross(p1 - p3, p - p3), normal);
	if (a1 >= 0 && a2 >= 0 && a3 >= 0) {
		alpha = a2 / a; 
		beta = a3 / a; 
		gamma = 1.0 - alpha - beta;
		return true;
	}
	return false;
}

// p(t) = p0 + dt 
Vector get_ray_pos_at_length(Ray r, double t) {
	return r.origin + r.direction * t;
}

// get reflected vector
// r = 2(l.n)n - l
Vector get_reflected_vec(Vector l, Vector n) {
	double LN = dot(l, n);
	Vector r = n * 2 * LN - l ;
	return r;
}

// power of vector
Vector power(Vector v, int i) {
	return Vector(pow(v.x, i), pow(v.y, i), pow(v.z, i));
}

// cross product
Vector cross(Vector i, Vector j) {
	Vector res;
	res.x = i.y * j.z - i.z * j.y;
	res.y = i.z * j.x - i.x * j.z;
	res.z = i.x * j.y - i.y * j.x;
	return res;
}

// dot product
double dot(Vector i, Vector j) {
	Vector k = i * j;
	return k.x + k.y + k.z;
}

// distance of two point
// sqrt( (i - j)^2 )
double distance(Vector i, Vector j) {
	Vector k = power(i - j, 2);
	return sqrt(k.x + k.y + k.z);
}

void clamp(Vector &color) {
	color.x = std::max(std::min(color.x, 255.0), 0.0);
	color.y = std::max(std::min(color.y, 255.0), 0.0);
	color.z = std::max(std::min(color.z, 255.0), 0.0);
}

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}

void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}

/* Write a jpg image from buffer*/
void save_jpg()
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferBGR = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	// unsigned char buffer[HEIGHT][WIDTH][3];
	for (int r = 0; r < HEIGHT; r++) {
		for (int c = 0; c < WIDTH; c++) {
			for (int chan = 0; chan < 3; chan++) {
				unsigned char red = buffer[r][c][0];
				unsigned char green = buffer[r][c][1];
				unsigned char blue = buffer[r][c][2];
				bufferBGR.at<cv::Vec3b>(r,c) = cv::Vec3b(blue, green, red);
			}
		}
	}
	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

void parse_check(char *expected,char *found)
{
  if(stricmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}

void parse_doubles(FILE*file, char *check, Vector& p)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p.x, &p.y, &p.z);
	printf("%s %lf %lf %lf\n", check, &p.x, &p.y, &p.z);
}

void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i",&number_of_objects);

  printf("number of objects: %i\n",number_of_objects);
  char str[200];

  parse_doubles(file,"amb:",ambient_light);

  for(i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
      if(stricmp(type,"triangle")==0)
	{

	  printf("found triangle\n");
	  int j;

	  for(j=0;j < 3;j++)
	    {
	      parse_doubles(file,"pos:",t.v[j].position);
	      parse_doubles(file,"nor:",t.v[j].normal);
	      parse_doubles(file,"dif:",t.v[j].color_diffuse);
	      parse_doubles(file,"spe:",t.v[j].color_specular);
	      parse_shi(file,&t.v[j].shininess);
	    }

	  if(num_triangles == MAX_TRIANGLES)
	    {
	      printf("too many triangles, you should increase MAX_TRIANGLES!\n");
	      exit(0);
	    }
	  triangles[num_triangles++] = t;
	}
      else if(stricmp(type,"sphere")==0)
	{
	  printf("found sphere\n");

	  parse_doubles(file,"pos:",s.position);
	  parse_rad(file,&s.radius);
	  parse_doubles(file,"dif:",s.color_diffuse);
	  parse_doubles(file,"spe:",s.color_specular);
	  parse_shi(file,&s.shininess);

	  if(num_spheres == MAX_SPHERES)
	    {
	      printf("too many spheres, you should increase MAX_SPHERES!\n");
	      exit(0);
	    }
	  spheres[num_spheres++] = s;
	}
      else if(stricmp(type,"light")==0)
	{
	  printf("found light\n");
	  parse_doubles(file,"pos:",l.position);
	  parse_doubles(file,"col:",l.color);

	  if(num_lights == MAX_LIGHTS)
	    {
	      printf("too many lights, you should increase MAX_LIGHTS!\n");
	      exit(0);
	    }
	  lights[num_lights++] = l;
	}
      else
	{
	  printf("unknown type in scene description:\n%s\n",type);
	  exit(0);
	}
    }
  return 0;
}

void display()
{

}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
      draw_scene();
      if(mode == MODE_JPEG)
	save_jpg();
    }
  once=1;
}

int main (int argc, char ** argv)
{
	if (argc<2 || argc > 4) {
		printf("usage: %s <scenefile> [jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 2) {
		mode = MODE_DISPLAY;
	} else if (argc == 3) {
		if (argv[argc - 1] == "recursion") {
			//is_reflect = true;
		} else {
			mode = MODE_JPEG;
			filename = argv[2];
		}
	}
	else if (argc == 4) {
		if (argv[argc - 1] == "recursion") {
			//is_reflect = true;
		}
		mode = MODE_JPEG;
		filename = argv[2];
	}

	glutInit(&argc, argv);
	loadScene(argv[1]);

	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	int window = glutCreateWindow("Assign3: ray tracer");
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
}
