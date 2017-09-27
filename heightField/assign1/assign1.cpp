// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 480 Computer Graphics
  Assignment 1: Height Fields
  C++ starter code
*/
#include <iostream>
#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

/*window size*/
int win_width = 640;
int win_height = 480;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};
float g_vAutoRotate = 0.0;

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;
Pic * g_pColorInfo;

/* state flags*/
bool ifLight = false; // Experiment with material and lighting properties.
bool ifColorInput = false; // Support color (bpp=3) in input images.
bool ifOffset = false; // Render wireframe on top of solid triangles (use glPolygonOffset to avoid z-buffer fighting).
bool ifColorInfo = false; // Color the vertices based on color values taken from another image of equal size. However, your code should still support also smooth color gradients as per the core requirements.
bool ifTextureMap = false; // Texturemap the surface with an arbitrary image.
bool ifDefromable = false; // Allow the user to interactively deform the landscape.
bool screenshot = false;
GLuint m_textureObjectOne;

/* count screen shot*/
int numScreenshot = 200;
int n =15; // desired frame rate

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;
  if (filename == NULL)  return;
  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);
  printf("File to save to: %s\n", filename);
  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }
  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");
  pic_free(in);
}

void myinit()
{
  /* setup gl view */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, win_width/win_height, 0.1, 1000.0);
  glMatrixMode(GL_MODELVIEW);
  
  /* setup light & material*/
  GLfloat light_position[] = { 0.5, 1.0, 0.5, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
         
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);   
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

  GLfloat no_mat[] = {0.0, 0.0, 0.0, 1.0};
  GLfloat mat_specular[] =  {1.0, 1.0, 1.0, 1.0};
  GLfloat low_shininess[] = {60.0};

  glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat); 
  glMaterialfv(GL_FRONT, GL_DIFFUSE, no_mat); 
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);
  glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
  
  glColorMaterial(GL_FRONT,GL_SPECULAR);
  glEnable(GL_COLOR_MATERIAL);
}

/* method to convert color value to height*/
GLfloat colorToHeight(int x, int y){
	if (ifColorInput) // if support color input
	{
	  GLfloat R =(GLfloat) PIC_PIXEL(g_pHeightData,x,y,0);
      GLfloat G =(GLfloat) PIC_PIXEL(g_pHeightData,x,y,1);
      GLfloat B =(GLfloat) PIC_PIXEL(g_pHeightData,x,y,2);
	  return (GLfloat)(R+G+B)/(3*255.0);
	}

	return (GLfloat) PIC_PIXEL(g_pHeightData,x,y,0)/255.0;
}

void draw()
{
  int pic_width = g_pHeightData ->nx;
  int pic_height = g_pHeightData ->ny;

  for (int i = 0; i < pic_height-1; i++){
	  glBegin(GL_TRIANGLE_STRIP);
	  for (int j = 0; j < pic_width; j++){
		  //'top' vertex
		  GLfloat top_color = colorToHeight(j,i);
		  GLfloat top_height = top_color/4;
		  GLfloat top[3] = {(GLfloat) j/(pic_width-1), (GLfloat) (-1)*i/(pic_height-1), top_height};
		  
		  //'bottom' vertex
		  GLfloat bottom_color = colorToHeight(j,i+1);
		  GLfloat bottom_height = bottom_color/4;
		  GLfloat bottom[3] = {(GLfloat) j/(pic_width-1), (GLfloat) (-1)*(i+1)/(pic_height-1), bottom_height};
		  
		  // sequential top,bottom vert pairs generates a tri-strip
		  glColor3f(1, top_color,1 ); // color from the same image
		  if (ifColorInfo) {
			GLfloat R =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i,0)/255.0;
			GLfloat G =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i,1)/255.0;
			GLfloat B =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i,2)/255.0;
			glColor3f(R,G,B); // color from another image
		  }
		  glTexCoord2f((GLfloat) j/(pic_width-1), (GLfloat)i/(pic_height-1)); // texturemap
		  glVertex3fv(top); // vertex
		  glColor3f(1, bottom_color,1);
		  if (ifColorInfo) {
			GLfloat R =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i+1,0)/255.0;
			GLfloat G =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i+1,1)/255.0;
			GLfloat B =(GLfloat) PIC_PIXEL(g_pColorInfo,j,i+1,2)/255.0;
			glColor3f(R,G,B);
		  }
		  glTexCoord2f((GLfloat) j/(pic_width-1),  (GLfloat)(i+1)/(pic_height-1));
		  glVertex3fv(bottom);
	  }
	  glEnd();
  }
}


void display()
{
  /* draw 1x1 cube about origin */
  /* replace this code with your height field implementation */
  /* you may also want to precede it with your 
rotation/translation/scaling */
	
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // set modelview matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0,-1.0,2.0,0.5,-0.5,0.5,1.0,1.0,0.0);
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  glRotatef(g_vAutoRotate, 0.5,-0.5, 1.0);
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

  //glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
  //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

  /* light control*/
  if (ifLight){
	  glEnable(GL_LIGHTING);
	  glEnable(GL_LIGHT0);
  }
  else{
	  glDisable(GL_LIGHTING);
      glDisable(GL_LIGHT0);
  }

  /* texture map set up*/
  if (ifTextureMap){
	  glEnable (GL_TEXTURE_2D);
	  
	  glGenTextures (1, &m_textureObjectOne);
	  glBindTexture (GL_TEXTURE_2D, m_textureObjectOne);
	  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  /*specify texture*/
	  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, g_pColorInfo->nx, g_pColorInfo->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, g_pColorInfo->pix);
	  glBindTexture (GL_TEXTURE_2D, m_textureObjectOne);
  }
  else {
	  glDisable (GL_TEXTURE_2D);
  }

  /* offset control */
  if (ifOffset){
	glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    draw();
    glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	draw();
  }
  else {
	draw();
  }

  glFlush();
  glutSwapBuffers(); // double buffer flush

}


/*
bool ifLight = false; // Experiment with material and lighting properties.
bool ifColorInput = false; // Support color (bpp=3) in input images.
bool ifOffset = false; // Render wireframe on top of solid triangles (use glPolygonOffset to avoid z-buffer fighting).
bool ifColorInfo = false; // Color the vertices based on color values taken from another image of equal size. However, your code should still support also smooth color gradients as per the core requirements.
bool ifTextureMap = false; // Texturemap the surface with an arbitrary image.
bool ifDefromable = false; // Allow the user to interactively deform the landscape.
*/

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
	case 1:
		ifLight = !ifLight;
		break;
	case 2:
		ifColorInput = !ifColorInput;
		break;
	case 3:
		ifOffset = !ifOffset;
		break;
	case 4:
		ifColorInfo = !ifColorInfo;
		if(!g_pColorInfo) ifColorInfo = false;
		break;
	case 5:
		ifTextureMap = !ifTextureMap;
		if(!g_pColorInfo) ifTextureMap = false;
		break;
	case 6:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		ifOffset = false;
		break;
	case 7:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		ifOffset = false;
		break;
	case 8:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		ifOffset = false;
		break;
  }
}

// use keyborad to switch Render Mode
void mykey(unsigned char key, int x, int y)
{
	switch (key) {
	case 'a': case 'A':
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		ifOffset = false;
		break;
	case 's': case 'S':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		ifOffset = false;
		break;
	case 'd': case 'D':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		ifOffset = false;
		break;
	case 'q': case 'Q':
		ifLight = !ifLight;
		break;
	case 'w': case 'W':
		ifColorInput = !ifColorInput;
		break;
	case 'e': case 'E':
		ifOffset = !ifOffset;
		break;
	case 'r': case 'R':
		ifColorInfo = !ifColorInfo;
		if(!g_pColorInfo) ifColorInfo = false;
		break;
	case 't': case 'T':
		ifTextureMap = !ifTextureMap;
		if(!g_pColorInfo) ifTextureMap = false;
		break;
	case 'p': case 'P':
		screenshot = !screenshot;
		break;
	default: break;
	}
	glutPostRedisplay();
}

void doIdle()
{
  /* do some stuff... */
  //g_vLandRotate[3] += 0.5;
  //g_vLandRotate[2] += 0.5f;
  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void myTimer (int v)
{
	if(screenshot){
	  char filename[30];
	  sprintf(filename, ".\\screenshots\\%03d.jpg",numScreenshot);
      saveScreenshot(filename);
      numScreenshot++;
	  if(numScreenshot == 300) screenshot = false;
	}
  glutTimerFunc(1000/n, myTimer, v);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to spiral.jpg.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your texture name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s heightfield.jpg\n", argv[0]);
		exit(1);
	}

	g_pHeightData = jpeg_read((char*)argv[1], NULL);
	if (!g_pHeightData)
	{
	    printf ("error reading %s.\n", argv[1]);
	    exit(1);
	}

	if (argc>2)
	{
		g_pColorInfo = jpeg_read((char*)argv[2], NULL);
		if (!g_pColorInfo)
		{
			printf ("error reading %s.\n", argv[1]);
			exit(1);
		}
	}
	glutInit(&argc,(char**)argv);
  
	/*
		create a window here..should be double buffered and use depth testing
  
	    the code past here will segfault if you don't have a window set up....
	    replace the exit once you add those calls.
	*/
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
    // set window size
    glutInitWindowSize(win_width, win_height);
    // set window position
    glutInitWindowPosition(0, 0);
    // creates a window
    glutCreateWindow("Height Field! Hahaha!");
   
	glEnable(GL_DEPTH_TEST);            // enable depth buffering
    glShadeModel(GL_SMOOTH);

	//exit(0);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);
  
	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("POINT",6);
	glutAddMenuEntry("LINE",7);
	glutAddMenuEntry("FILL",8);
	glutAddMenuEntry("Light(ON/OFF)",1);
	glutAddMenuEntry("Color input(ON/OFF)",2);
	glutAddMenuEntry("LINE on top of FILL(ON/OFF)",3);
	glutAddMenuEntry("Coloured by another image(ON/OFF)",4);
	glutAddMenuEntry("Texturemap(ON/OFF)",5);
	glutAddMenuEntry("Quit",0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for keyborad  */
	glutKeyboardFunc(mykey);
	/* timer function*/
	glutTimerFunc(100,myTimer,n);

	/* do initialization */
	myinit();

	glutMainLoop();
	return 0;
}