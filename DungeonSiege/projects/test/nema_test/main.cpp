#include "gpcore.h"

#include <windows.h>

#include <GL/gl.h>
#include "glut.h"

#include "quat.h"
#include "space_3.h"


//*************************************************************************
//*************************************************************************

// NewMachine test definitions

#include "aspect.h"
#include "channel.h"
#include "controller.h"
#include "mesh.h"

nema::tMesh *testmesh = 0;

void PrepNeMa(void);

//-----------------------------------------------------------

void PrepNeMa() {

	nema::tController myController;
	nema::tChannel myChannel(myController);
//	nema::tAspect myAspect;

	myChannel.Update(1.0f);


	testmesh = new nema::tMesh(10,10);
	delete testmesh;

	testmesh = new nema::tMesh("\\\\conan\\soul\\soulserver\\current\\00\\c_gah_fg.smf");

/*

	// a bunch of quat test cases.....

	tQuat q(5,6,7,8);

	q.X(1);
	q.Y(2);
	q.Z(3);
	q.W(4);

	tQuat i,j,k,l,m;

	i = tQuat(1,1,1,RealPi/4);

	j=i;
	k=i;

	j.Rotate(vector_3(0,0,1),RealPi/3);
	j.Rotate(vector_3(0,0,1),RealPi/3);

	k.RotateZ(RealPi/5);
	k.RotateZ(RealPi/5);

	l = k*j;
	m = j*k;

	i = k;
	k *= j;
	j *= i;

	bool a_test = l == k;
	bool b_test = l == k;
	bool c_test = l != k;
	bool d_test = l != k;

	matrix_3x3	id0_mat(matrix_3x3());
	tQuat		id0_quat(id0_mat);

	matrix_3x3	test_mat0(AxisRotationColumns(vector_3(1,1,1),RealPi/4));

	tQuat		test0_quat(test_mat0);

	matrix_3x3	test_mat1(XRotationColumns(RealPi/3));
	matrix_3x3	test_mat2(YRotationColumns(RealPi/3));
	matrix_3x3	test_mat3(ZRotationColumns(RealPi/3));

	tQuat		test1_quat(test_mat1);
	tQuat		test2_quat(test_mat2);
	tQuat		test3_quat(test_mat3);

	tQuat		test4_quat;
	tQuat		test5_quat;
	tQuat		test6_quat;

	test4_quat.RotateX(RealPi/3);
	test5_quat.RotateY(RealPi/3);
	test6_quat.RotateZ(RealPi/3);


	matrix_3x3	prod_mat1 = test_mat1 * test_mat2;
	tQuat		prod_quat1(prod_mat1);

	tQuat		prod_quat2 = test1_quat * test2_quat;

	tQuat		prod_quat3 = test1_quat;
	prod_quat3 *= test2_quat;

	matrix_3x3	prod_mat2 = test_mat2 * test_mat1;
	tQuat		prod_quat4(prod_mat2);

	tQuat		prod_quat5 = test2_quat * test1_quat;

	tQuat		prod_quat6 = test2_quat;
	prod_quat6 *= test1_quat;

	matrix_3x3	extracted = prod_quat6.BuildMatrix();


	test3_quat  = test1_quat+test2_quat;

	test4_quat  = test1_quat;
	test4_quat += test2_quat;

	test5_quat  = test4_quat;
	test5_quat -= test2_quat;

	test6_quat  = test3_quat-test2_quat;

*/

}



//*************************************************************************
//*************************************************************************


/* size of illumination texture */
#define TEX_RES 512

#define M_SQRT1_2	0.707106781f
#define M_PI		3.141592654f
#define M_PI_2		1.570796327f


/* global state variables */
double  angleX= 0.0;
double  angleY= 0.0;
double  lightAngleX= 45.0;
double  lightAngleY= 0.0;
char	orientation= 'u';
int	xPos, yPos;
int	which;
int	geom= 0;
double	aspect= 1.0;
GLfloat lightDir[4]= {M_SQRT1_2, 0.0, M_SQRT1_2, 0.0};
GLfloat lightColor[4]= {3.0, 3.0, 3.0, 0.0};
GLfloat surfColor[4]= {1.0, 0.0, 0.0, 0.0};
GLfloat ambColor[4]= {0.0, 0.0, 0.0, 0.0};

GLuint tname;
GLubyte	tex[TEX_RES][TEX_RES][4];

/* some forward declarations */
void makeMatrix( float, float, float, float, float, float );
void genTexture( void );


/* re-render the complete geometry */
void
drawAll(void)
{
	/* rotate geometry */
	glRotated( angleY, 0.0, 1.0, 0.0 );
	glRotated( angleX, 1.0, 0.0, 0.0 );
	testmesh->Render();

}


/* render one frame */
void
redraw(void)
{
  /* clear buffers */
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glEnable( GL_TEXTURE_2D );

  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
//  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
  glBindTexture(GL_TEXTURE_2D,tname);

  /* standard view point and viewing direction */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt( 0.0, 0.0, 2.5, 0.0, 0.9, 0.0, 0.0, 1.0, 0.0 );

  /* update light direction */
/*
  glLightfv( GL_LIGHT0, GL_POSITION, lightDir );
  glEnable( GL_LIGHTING );
*/


  drawAll();

  glutSwapBuffers();
}


/* callback for menu buttons */
void
menu( int which )
{
/*
  switch( which )
  {
    default:
      break;
  }
*/
  glutPostRedisplay();
}


/* callback for key presses */
void
keyboard(unsigned char c, int x, int y)
{
  switch( c )
  {
    case 27:
    case 'q':
      exit(0);
      break;
    case 'r':
      /* rotation of geometry */
      angleX= angleY= 0.0;

      /* orientation of directional light */
      lightAngleX= 45.0; lightAngleY= 0.0;
      lightDir[0]= lightDir[2]= M_SQRT1_2; lightDir[1]= 0.0;
      makeMatrix( lightDir[0], lightDir[1], lightDir[2], 0.0, 0.0, -1.0 );
      
    case 'w':
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      break;
    case 's':
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      break;
    default:
      break;
  }
  glutPostRedisplay();
}


/* callback for mouse motions */
void
move(int x, int y)
{
  switch( which )
  {
    case 0:
      /* button 0: rotate geometry */
      angleY+= (xPos-x)/2.0;
      angleX+= (yPos-y)/2.0;
      xPos= x; yPos= y;
      
      while( angleX> 360.0 )
	angleX-= 360.0;
      while( angleX< 0.0 )
	angleX+= 360.0;
      while( angleY> 360.0 )
	angleY-= 360.0;
      while( angleY< 0.0 )
	angleY+= 360.0;
      
      break;
      
    case 1:
      /* button 1: change light firection */
      lightAngleY+= (yPos-y)/2.0;
      lightAngleX+= (xPos-x)/2.0;
      xPos= x; yPos= y;
      
      while( lightAngleX> 360.0 )
	lightAngleX-= 360.0;
      while( lightAngleX< 0.0 )
	lightAngleX+= 360.0;
      while( lightAngleY> 360.0 )
	lightAngleY-= 360.0;
      while( lightAngleY< 0.0 )
	lightAngleY+= 360.0;

      lightDir[0]= cos(lightAngleX*M_PI/180.0) * cos(lightAngleY*M_PI/180.0);
      lightDir[1]= cos(lightAngleX*M_PI/180.0) * sin(lightAngleY*M_PI/180.0);
      lightDir[2]= sin(lightAngleX*M_PI/180.0);
      makeMatrix( lightDir[0], lightDir[1], lightDir[2], 0.0, 0.0, -1.0 );
      
      break;
  }
      
  glutPostRedisplay();
}


/* callback for mouse buttons */
void
mouse( int button, int state, int x, int y )
{
  /* store current mouse position and button state */
  xPos= x; yPos= y;
  which= button;
}


/* callback for changes in window geometry */
void
reshape( int w, int h )
{
  glViewport( 0, 0, w, h );
  aspect= (double)w/h;
  
  /* a generic perspective xform */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45, aspect, 1, 10.0);
}


void
genTexture(void)
{
  int		i, j;

  for( i= 0 ; i< TEX_RES ; i++ )
  {
    for( j= 0 ; j< TEX_RES ; j++ )
    {
      
		float u = i - TEX_RES/2.0f;
		float v = j - TEX_RES/2.0f;

		float dist = sqrt(u*u+v*v);

		int	bucket = (int)(dist/(TEX_RES/16));

		int color = bucket % 6;

		switch (color) {

		case 0:
			tex[i][j][0] = 0x00;
			tex[i][j][1] = 0x00;
			tex[i][j][2] = 0x80;
			tex[i][j][3] = 0xff;
			break;
		case 2:
			tex[i][j][0] = 0x00;
			tex[i][j][1] = 0x80;
			tex[i][j][2] = 0x00;
			tex[i][j][3] = 0xff;
			break;
		case 4:
			tex[i][j][0] = 0x80;
			tex[i][j][1] = 0x00;
			tex[i][j][2] = 0x00;
			tex[i][j][3] = 0xff;
			break;
		case 1:
		case 3:
		case 5:
		default:
			tex[i][j][0] = 0xc0;
			tex[i][j][1] = 0xc0;
			tex[i][j][2] = 0xc0;
			tex[i][j][3] = 0xff;
		}

    }
  }
 

/*  printf( "P6\n%d %d\n255\n", TEX_RES, TEX_RES );
  fwrite( tex, sizeof( char ), TEX_RES*TEX_RES*3, stdout );*/
}


/* one-time initialization */
void
init( void )
{

  /* texture modes */

  glEnable( GL_TEXTURE_2D );
  glEnable( GL_DEPTH_TEST );

  genTexture( );

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  glGenTextures(1,&tname);
  glBindTexture(GL_TEXTURE_2D, tname);

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, TEX_RES, TEX_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex );

  glPolygonMode( GL_FRONT, GL_FILL );
  glPolygonMode( GL_BACK, GL_LINE );



  // light and shading settings 
  glLightfv( GL_LIGHT0, GL_DIFFUSE, lightColor );
  glLightfv( GL_LIGHT0, GL_SPECULAR, lightColor );
  glLightfv( GL_LIGHT0, GL_AMBIENT, ambColor );
  glEnable( GL_LIGHT0 );
  glEnable( GL_LIGHTING );
  glColorMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE );
  glEnable( GL_COLOR_MATERIAL );
  glLightModelf( GL_LIGHT_MODEL_TWO_SIDE, 1.0 );
  
  glClearColor( 1.0, 1.0, 1.0, 0.0 );


  /* a generic perspective xform */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45, aspect, 1, 10.0);

}


/* create the transformation matrix for a given viewing direction and
   light direction */
void
makeMatrix( float lx, float ly, float lz, float vx, float vy, float vz )
{
  GLfloat	matrix[16];

  matrix[0]=	0.5 * lx;
  matrix[4]=	0.5 * ly;
  matrix[8]=	0.5 * lz;
  matrix[12]=	0.5;
  
  matrix[1]=	0.5 * vx;
  matrix[5]=	0.5 * vy;
  matrix[9]=	0.5 * vz;
  matrix[13]=	0.5;
  
  matrix[2]=	0.0;
  matrix[6]=	0.0;
  matrix[10]=	0.0;
  matrix[14]=	0.0;

  matrix[3]=	0.0;
  matrix[7]=	0.0;
  matrix[11]=	0.0;
  matrix[15]=	1.0;
  
  glLoadMatrixf( matrix );
  glPushMatrix();
  
}


/* main program: argument parsing and some initializations */
void
main( int argc, char **argv )
{

	PrepNeMa();
	
  int	mainMenu;
  

  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow("NeMa Test");
  
  makeMatrix( lightDir[0], lightDir[1], lightDir[2], 0.0f, 0.0f, -1.0f );
  init();
  
  /* register callbacks */
  glutDisplayFunc(redraw);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(move);
  glutReshapeFunc(reshape);

  /* create menus */
  mainMenu= glutCreateMenu( menu );
  glutAddMenuEntry( "YourFuncHere", 5 );
  glutAttachMenu( GLUT_RIGHT_BUTTON );
/*
  glutAddMenuEntry( "Sphere", 5 );
  glutAddMenuEntry( "Torus", 6 );
  glutAddMenuEntry( "Cylinder", 7 );
  glutAddMenuEntry( "Disk", 8 );
  polyModeMenu= glutCreateMenu( menu );
  glutAddMenuEntry( "Wireframe", 3 );
  glutAddMenuEntry( "Solid", 4 );
  glutAddMenuEntry( "Toggle Light Vector", 9 );
  orientationMenu= glutCreateMenu( menu );
  glutAddMenuEntry( "u", 1 );
  glutAddMenuEntry( "v", 2 );
  shadowMenu= glutCreateMenu( menu );
  glutAddMenuEntry( "on", 10 );
  glutAddMenuEntry( "off", 11 );
  materialMenu= glutCreateMenu( menu );
  glutAddMenuEntry( "plastic", 12 );
  glutAddMenuEntry( "metal", 13 );
  glutCreateMenu( menu );
  glutAddSubMenu( "Geometry", geomMenu );
  glutAddSubMenu( "Rendering Mode", polyModeMenu );
0  glutAddSubMenu( "Scratch Orientation", orientationMenu );
  glutAddSubMenu( "Self Shadowing", shadowMenu );
  glutAddSubMenu( "Material", materialMenu );
  glutAddMenuEntry( "Reset", 19 );
  glutAddMenuEntry( "Exit", 20 );
*/
  glutAttachMenu( GLUT_RIGHT_BUTTON );
  
  glutMainLoop();
}


