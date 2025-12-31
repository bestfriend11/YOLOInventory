/* AUTHOR:       Wolfgang Heidrich
 *		 Computer Graghics Group
 *               University of Erlangen
 *               Email: heidrich@informatik.uni-erlangen.de
 * ========================================================================
 *               Copyright (c) 1998 University of Erlangen
 * ========================================================================
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice and the names of the authors
 * appear in all copies and that both that the copyright notice and
 * warranty disclaimer appear in supporting documentation, and that the
 * names of the University of Erlangen or any of their entities not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * 
 * The University of Erlangen disclaims all warranties with regard to
 * this software, including all implied warranties of merchantability and
 * fitness.  In no event shall the University of Erlangen be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in an
 * action of contract, negligence or other tortuous action, arising out
 * of or in connection with the use or performance of this software.
 * ========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <malloc.h>
#include <time.h>
#include <math.h>

#include <windows.h>

#include <GL/gl.h>
#include "glut.h"



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
int	renderLightVec= 1;
int	quality= 100;//72;
int	xPos, yPos;
int	which;
int	geom= 0;
double	aspect= 1.0;
GLfloat lightDir[4]= {M_SQRT1_2, 0.0, M_SQRT1_2, 0.0};
GLfloat lightColor[4]= {3.0, 3.0, 3.0, 0.0};
GLfloat surfColor[4]= {1.0, 0.0, 0.0, 0.0};
GLfloat ambColor[4]= {0.0, 0.0, 0.0, 0.0};
int	lighting= 1;
int	metal= 0;

/* some forward declarations */
void makeMatrix( float, float, float, float, float, float );
void genGeometry( void );
void genTexture( float r, float g, float b, float Kd, float Ks, float rough );


/* re-render the complete geometry */
void
drawAll(void)
{
  /* if enabled, draw a green line from the origin in the direction of
     the light */
  if( renderLightVec )
  {
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_LIGHTING );
    glBegin( GL_LINES );
    glColor3f( 0.0, 1.0, 0.0 );
    glVertex3f( 0.0, 0.0, 0.0 );
    glVertex3f( 2.0*lightDir[0], 2.0*lightDir[1], 2.0*lightDir[2] );
    glEnd();
    if( lighting )
      glEnable( GL_LIGHTING );
    glEnable( GL_TEXTURE_2D );
  }
  
  /* rotate geometry */
  glRotated( angleY, 0.0, 1.0, 0.0 );
  glRotated( angleX, 1.0, 0.0, 0.0 );

  /* draw geometry */
  glCallList( 1 );
}


/* render one frame */
void
redraw(void)
{
  /* clear buffers */
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  /* adjust texture matrix */
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glPushMatrix();
  glRotated( angleY, 0.0, 1.0, 0.0 );
  glRotated( angleX, 1.0, 0.0, 0.0 );

  /* standard view point and viewing direction */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt( 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );

  /* update light direction */
  glLightfv( GL_LIGHT0, GL_POSITION, lightDir );

  drawAll();

  glutSwapBuffers();
}


/* callback for menu buttons */
void
menu( int which )
{
  switch( which )
  {
	case 0:
      break;

    case 1:
      /* orientation: u */
      orientation= 'u';
      genGeometry();
      break;

    default:
      break;
  }
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
      
      break;
    case 'u':
      orientation= 'u';
      genGeometry();
      break;
    case 'v':
      orientation= 'v';
      genGeometry();
      break;
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


/* create sphere geometry with texture coordinates and normal vectors */
void
makeSphere( int resU, int resV )
{
  double s, t, s1;
  double phi, theta, phi1;
  double tx, ty, tz, tlen;
  double h;
  int	i, j;

  /* this isn't very efficient, but it is not called very often */
  for( i= 0 ; i< resU ; i++ )
  {
    s= (double)i/(double)resU;
    s1= (i+1== resU) ? 1.0 : (double)(i+1)/(double)resU;
    phi= s*2.0*M_PI;
    phi1= (i+1== resU) ? 0.0 : s1*2.0*M_PI;
    
    glBegin( GL_QUAD_STRIP );
    for( j= 0 ; j<= resV ; j++ )
    {
      t= (j== resV) ? 1.0 : (double)j/(double)resV;
      theta= (j== 0) ? -M_PI_2 : ((j== resV) ? M_PI_2 : (t-0.5)*M_PI);
      h= cos( theta );
      
      if( orientation== 'u' )
      {
	/* tangent vector in th eparametric 'u' direction */
	tx= -cos(phi)*sin(theta);
	ty= -sin(phi)*sin(theta);
	tz= cos(theta);
	tlen= sqrt( tx*tx + ty*ty + tz*tz );
	glTexCoord3d( tx/tlen, ty/tlen, tz/tlen );
      }
      else if( orientation== 'v' )
	/* tangent vector in 'v' direction */
	glTexCoord3d( -sin( phi ), cos( phi ), 0.0 );
      /* normal and point are identical for unit spheres */
      glNormal3d( cos( phi )*h, sin( phi )*h, sin( theta ) );
      glVertex3d( cos( phi )*h, sin( phi )*h, sin( theta ) );

      /* same as above for phi1 instead of phi */
      if( orientation== 'u' )
      {
	tx= -cos(phi1)*sin(theta);
	ty= -sin(phi1)*sin(theta);
	tz= cos(theta);
	tlen= sqrt( tx*tx + ty*ty + tz*tz );
	glTexCoord3d( tx/tlen, ty/tlen, tz/tlen );
      }
      else if( orientation== 'v' )
	glTexCoord3d( -sin( phi1 ), cos( phi1 ), 0.0 );
      glNormal3d( cos( phi1 )*h, sin( phi1 )*h, sin( theta ) );
      glVertex3d( cos( phi1 )*h, sin( phi1 )*h, sin( theta ) );
    }
    glEnd();
  }
}


/* create torus geometry with texture coordinates and normal vectors */
void
makeTorus( double minRadius, double maxRadius, int resU, int resV )
{
  double s, t, s1;
  double phi, theta, phi1;
  double tx, ty, tz, tlen;
  double h;
  int	 i, j;
  
  /* this isn't very efficient, but it is not called very often */
  for( i= 0 ; i< resU ; i++ )
  {
    s= (i== 0) ? 0.0 : (double)i/(double)resU;
    s1= (i+1== resU) ? 1.0 : (double)(i+1)/(double)resU;
    phi= s*2.0*M_PI;
    phi1= (i+1== resU) ? 0.0 : s1*2.0*M_PI;
    
    glBegin( GL_QUAD_STRIP );
    for( j= 0 ; j<= resV ; j++ )
    {
      t= (j== 0) ? 0.0 : ((j== resV) ? 1.0 : (double)j/(double)resV);
      theta= (j== 0) ? 0.0 : ((j== resV) ? 0.0 : t*2.0*M_PI);
      h= cos( theta )*minRadius + maxRadius;
      
      if( orientation== 'u' )
      {
	/* tangent vector in the parametric 'u' direction */
	tx= -cos(phi)*sin(theta)*minRadius;
	ty= -sin(phi)*sin(theta)*minRadius;
	tz= cos(theta)*minRadius;
	tlen= sqrt( tx*tx + ty*ty + tz*tz );
	glTexCoord3d( tx/tlen, ty/tlen, tz/tlen );
      }
      else if( orientation== 'v' )
	/* tangent vector in the parametric 'v' direction */
	glTexCoord3d( -sin( phi ), cos( phi ), 0.0 );
      glNormal3d( cos( phi )*cos( theta ),
		  sin( phi )*cos( theta ),
		  sin( theta ) );
      glVertex3d( h*cos( phi ), h*sin( phi ), sin( theta )*minRadius );

      /* same as above for phi1 instead of phi */
      if( orientation== 'u' )
      {
	tx= -cos(phi1)*sin(theta)*minRadius;
	ty= -sin(phi1)*sin(theta)*minRadius;
	tz= cos(theta)*minRadius;
	tlen= sqrt( tx*tx + ty*ty + tz*tz );
	glTexCoord3d( tx/tlen, ty/tlen, tz/tlen );
      }
      else if( orientation== 'v' )
	glTexCoord3d( -sin( phi1 ), cos( phi1 ), 0.0 );
      glNormal3d( cos( phi1 )*cos( theta ),
		  sin( phi1 )*cos( theta ),
		  sin( theta ) );
      glVertex3d( h*cos( phi1 ), h*sin( phi1 ), sin( theta )*minRadius );
    }
    glEnd();
  }
}


void
makeCylinder( int resU, int resV )
{
  double s, s1;
  double phi, phi1;
  double x, x1, y, y1, z;
  int	 i, j;
  
  /* this isn't very efficient, but it is not called very often */
  for( i= 0 ; i< resU ; i++ )
  {
    s= (i== 0) ? 0.0 : (double)i/(double)resU;
    s1= (i+1== resU) ? 1.0 : (double)(i+1)/(double)resU;
    
    phi= s*2.0*M_PI;
    x= cos( phi );
    y= sin( phi );
    phi1= (i+1== resU) ? 0.0 : s1*2.0*M_PI;
    x1= cos( phi1 );
    y1= sin( phi1 );
    
    glBegin( GL_QUAD_STRIP );
    for( j= 0 ; j<= resV ; j++ )
    {
      z= (double)j/(double)resV - 0.5;
      
      if( orientation== 'u' )
	glTexCoord3d( 0.0, 0.0, 1.0 );
      else if( orientation== 'v' )
	glTexCoord3d( -y, x, 0.0 );
      glNormal3d( x, y, 0.0 );
      glVertex3d( x, y, z );

      /* same as above for phi1 instead of phi */
      if( orientation== 'u' )
	glTexCoord3d( 0.0, 0.0, 1.0 );
      else if( orientation== 'v' )
	glTexCoord3d( -y1, x1, 0.0 );
      glNormal3d( x1, y1, 0.0 );
      glVertex3d( x1, y1, z );
    }
    glEnd();
  }  
}


void
makeDisk( int resU, int resV )
{
  double s, s1;
  double phi, phi1;
  double x, x1, y, y1, r;
  int	 i, j;
  
  /* this isn't very efficient, but it is not called very often */
  for( i= 0 ; i< resU ; i++ )
  {
    s= (i== 0) ? 0.0 : (double)i/(double)resU;
    s1= (i+1== resU) ? 1.0 : (double)(i+1)/(double)resU;
    
    phi= s*2.0*M_PI;
    x= cos( phi );
    y= sin( phi );
    phi1= (i+1== resU) ? 0.0 : s1*2.0*M_PI;
    x1= cos( phi1 );
    y1= sin( phi1 );
    
    glBegin( GL_QUAD_STRIP );
    for( j= 0 ; j<= resV ; j++ )
    {
      r= (double)j/(double)resV;

      if( orientation== 'u' )
	glTexCoord3d( x, y, 0.0 );
      else if( orientation== 'v' )
	glTexCoord3d( -y, x, 0.0 );
      glNormal3d( 0.0, 0.0, -1.0 );
      glVertex3d( r*x, r*y, 0.0 );

      /* same as above for phi1 instead of phi */
      if( orientation== 'u' )
	glTexCoord3d( x1, y1, 0.0 );
      else if( orientation== 'v' )
	glTexCoord3d( -y1, x1, 0.0 );
      glNormal3d( 0.0, 0.0, -1.0 );
      glVertex3d( r*x1, r*y1, 0.0 );
    }
    glEnd();
  }  
}


  
/* create the desired geometry */
void
genGeometry( void )
{
  glNewList( 1, GL_COMPILE );
  glColor3f( 1.0, 1.0, 1.0 );
  switch( geom )
  {
    case 0:
      makeSphere( quality, quality/2 );
      break;
    case 1:
      makeTorus( 0.5, 1.0, quality, quality/2 );
      break;
    case 2:
      makeCylinder( quality, quality/2 );
      break;
    case 3:
      makeDisk( quality, quality/2 );
      break;
  }      
  glEndList();
}


/* one-time initialization */
void
init( void )
{
  /* texture modes */
  glEnable( GL_TEXTURE_2D );
  glEnable( GL_DEPTH_TEST );
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  glMatrixMode( GL_TEXTURE );
  glPushMatrix(); /* will be poped away in "makeMatrix" */

  /* light and shading settings */
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

  genGeometry();
}


/* generate the texture corresponding to the anisotropic shading model
   this is fone only once */
void
genTexture( float r, float g, float b, float Kd, float Ks, float roughness )
{
  GLubyte	tex[TEX_RES][TEX_RES][3];
  float		diff, spec, phong;
  float		lt, vt;
  float		h;
  int		i, j;

  for( i= 0 ; i< TEX_RES ; i++ )
  {
    lt= 1.0 - 2.0 * (float)i/(float)(TEX_RES-1);
    diff= sqrt( 1-lt*lt );
    
    for( j= 0 ; j< TEX_RES ; j++ )
    {
      vt= 2.0 * (float)j/(float)(TEX_RES-1) - 1.0;
      spec= diff*sqrt( 1-vt*vt ) - lt*vt;

      /* standard phong model */
      phong= Ks*pow( spec< 0.0 ? 0.0 : spec, 1.0/roughness );
      
      h= (r*Kd*diff + (metal?r:1.0)*phong)*255.0 + 0.5;
      tex[i][j][0]= h< 0.0 ? 0 : (h> 255.0 ? 255 : (int)h);
      h= (g*Kd*diff + (metal?g:1.0)*phong)*255.0 + 0.5;
      tex[i][j][1]= h< 0.0 ? 0 : (h> 255.0 ? 255 : (int)h);
      h= (b*Kd*diff + (metal?b:1.0)*phong)*255.0 + 0.5;
      tex[i][j][2]= h< 0.0 ? 0 : (h> 255.0 ? 255 : (int)h);
    }
  }
  
  glTexImage2D( GL_TEXTURE_2D, 0, /*GL_RGB8_EXT*/ 3, TEX_RES, TEX_RES, 0,
		GL_RGB, GL_UNSIGNED_BYTE, tex );

/*  printf( "P6\n%d %d\n255\n", TEX_RES, TEX_RES );
  fwrite( tex, sizeof( char ), TEX_RES*TEX_RES*3, stdout );*/
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
  
  glMatrixMode( GL_TEXTURE );
  glPopMatrix();
  glLoadMatrixf( matrix );
  glPushMatrix();
  
  glMatrixMode( GL_MODELVIEW );
}


/* main program: argument parsing and some initializations */
void
main( int argc, char **argv )
{
  int	mainMenu;
  
  if( argc!= 2 && argc!= 3 )
  {
    geom= 1;
//	fprintf( stderr,
//	     "usage: %s [sphere|torus|cylinder|disk] [<quality>]\n", argv[0] );
//   exit( 1 );
  }

  else if( !strcmp( argv[1], "sphere" ) )
    geom= 0;
  else if( !strcmp( argv[1], "torus" ) )
    geom= 1;
  else if( !strcmp( argv[1], "cylinder" ) )
    geom= 2;
  else if( !strcmp( argv[1], "disk" ) )
    geom= 3;
  else
  {
	  fprintf( stderr, "Unknown geometry: %s\n", argv[1] );
	  exit( 1 );

  }

  if( argc== 3 )
    quality= atoi( argv[2] );
  
  glutInit(&argc, argv);
  glutInitWindowSize(350, 350);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow("Shaderren");
  
  genTexture( surfColor[0], surfColor[1], surfColor[2], 0.2f, 0.8f, 0.025f );
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
  glutAddMenuEntry( "Sphere", 5 );
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
  glutAddSubMenu( "Scratch Orientation", orientationMenu );
  glutAddSubMenu( "Self Shadowing", shadowMenu );
  glutAddSubMenu( "Material", materialMenu );
  glutAddMenuEntry( "Reset", 19 );
  glutAddMenuEntry( "Exit", 20 );
*/
  glutAttachMenu( GLUT_RIGHT_BUTTON );
  
  glutMainLoop();
}





//***************************************************************

#include "controller.h"


nema::Controller myController;