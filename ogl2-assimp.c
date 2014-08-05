/* This sample is based on a sample that is included with ASSIMP. Much
 * of the logic of the program was unchanged. However, texture loading
 * and other miscellaneous changes were made.
 *
 * Changes by: Scott Kuhl
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "kuhl-util.h"
#include "dgr.h"
#include "projmat.h"
#include "viewmat.h"

/** Set this variable to 1 to force this program to scale the entire
 * model and translate it so that we can see the entire model. This is
 * a useful setting to use when you are loading a new model that you
 * are unsure about the units and position of the model geometry. */
#define FIT_TO_VIEW_AND_ROTATE 1
/** The location in 3D space that we want the center of the bounding box to be (if FIT_TO_VIEW_AND_ROTATE is set) or the location that we should put the origin of the model */
float placeToPutModel[3] = { 0,0,-4 };
/** SketchUp produces files that older versions of ASSIMP think 1 unit
 * is 1 inch. However, all of this software assumes that 1 unit is 1
 * meter. So, we need to convert some models from inches to
 * meters. Newer versions of ASSIMP correctly read the same files and
 * give us units in meters. */
#define INCHES_TO_METERS 0

GLuint scene_list = 0; // display list for model
char *modelFilename = NULL;
char *modelTexturePath = NULL;



/* Called by GLUT whenever a key is pressed. */
void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'q':
		case 'Q':
		case 27: // ASCII code for Escape key
			exit(0);
			break;
	}
	glutPostRedisplay();
}


void get_model_matrix(float result[16])
{
	mat4f_identity(result);
	if(FIT_TO_VIEW_AND_ROTATE == 0)
	{
		/* Translate the model to where we were asked to put it */
		float translate[16];
		mat4f_translateVec_new(translate, placeToPutModel);

		/* Do inches to meters conversion if we are asked to. */
		float scale[16];
		mat4f_identity(scale);
		if(INCHES_TO_METERS)
		{
			float inchesToMeters=1/39.3701;
			mat4f_scale_new(scale, inchesToMeters, inchesToMeters, inchesToMeters);
		}
		mat4f_mult_mat4f_new(result, translate, scale);
		
		
		return;
	}
	
	/* Change angle for animation. */
	int count = glutGet(GLUT_ELAPSED_TIME) % 10000; // get a counter that repeats every 10 seconds
	float angle = count / 10000.0 * 360;
	dgr_setget("angle", &angle, sizeof(GLfloat));

	/* Calculate the width/height/depth of the bounding box and
	 * determine which one of the three is the largest. Then, scale
	 * the scene by 1/(largest value) to ensure that it fits in our
	 * view frustum. */
	float bb_min[3], bb_max[3], bb_center[3];
	kuhl_model_bounding_box(modelFilename, bb_min, bb_max, bb_center);
#define mymax(a,b) (a>b?a:b)
	float tmp;
	tmp = bb_max[0] - bb_min[0];
	tmp = mymax(bb_max[1] - bb_min[1], tmp);
	tmp = mymax(bb_max[2] - bb_min[2], tmp);
	tmp = 1.f / tmp;
#undef mymax
	float rotateAnimate[16], scaleBoundBox[16], moveToOrigin[16], moveToLookPoint[16];
	mat4f_translate_new(moveToOrigin, -bb_center[0], -bb_center[1], -bb_center[2]); // move to origin
	mat4f_scale_new(scaleBoundBox, tmp, tmp, tmp); // scale model based on bounding box size
	mat4f_rotate_new(rotateAnimate, angle, 0, 1, 0); // rotate the object
	mat4f_translateVec_new(moveToLookPoint, placeToPutModel);

	mat4f_mult_mat4f_new(result, moveToOrigin, result);
	mat4f_mult_mat4f_new(result, scaleBoundBox, result);
	mat4f_mult_mat4f_new(result, rotateAnimate, result);
	mat4f_mult_mat4f_new(result, moveToLookPoint, result);
}

void display()
{
	dgr_update();

	// Clear the screen to black, clear the depth buffer
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST); // turn on depth testing

	/* Turn on lighting. By default, the light is where the camera
	 * is. */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// Apply light to back and front faces the same way
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	/* Normal vectors should be normalized for proper
	   lighting. GL_NORMALIZE makes OpenGL normalize all normal vectors
	   (regardless of if they already are!). Even if your normal vectors
	   are normalized, they can still be scaled by glScale()---so
	   GL_NORMALIZE is a good idea. Improperly scaled normal vectors can
	   often result in unexpected lighting. */
	glEnable(GL_NORMALIZE);

	/* If you encounter a model that looks incorrectly rendered, it
	   may be caused by vertices on a polygon being defined clockwise
	   (CW) instead of counter-clock-wise (CCW). Try commenting out
	   the following line. */
	// glFrontFace(GL_CW);

	kuhl_errorcheck();
	

    for(int view=0; view<viewmat_num_viewports(); view++)
    {
	    /* Where is the viewport that we are drawing onto and what is its size? */
	    int viewport[4];
	    viewmat_get_viewport(viewport, view);
	    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	    /* Get the view frustum information. */
	    float f[6];
	    projmat_get_frustum(f, viewport[2], viewport[3]);
	    
	    /* Get the projection matrix, update view frustum if necessary. */
	    float viewMat[16];
	    viewmat_get(viewMat, f, view);
	    
	    /* Communicate matricies to OpenGL */
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glFrustum(f[0], f[1], f[2], f[3], f[4], f[5]);
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
	    glMultMatrixf(viewMat);
	    kuhl_errorcheck();
	    float modelMat[16];
	    get_model_matrix(modelMat);
	    glMultMatrixf(modelMat);

	    // If the display list has not been made yet, create a new one and
	    // fill it with scene contents. Note: OpenGL display lists have
	    // been deprecated in OpenGL 3. For more information, see:
	    // http://stackoverflow.com/questions/4113989/
	    if(scene_list == 0) {
		    // make sure the scene & textures are loaded.
		    if(kuhl_draw_model_file_ogl2(modelFilename, modelTexturePath) == 0)
			    exit(EXIT_FAILURE);

		    // Create a display list
		    scene_list = glGenLists(1);
		    glNewList(scene_list, GL_COMPILE);
		    // now begin at the root node of the imported data and traverse
		    // the scenegraph by multiplying subsequent local transforms
		    // together on GL's matrix stack.
		    kuhl_draw_model_file_ogl2(modelFilename, modelTexturePath);
		    glEndList();
	    }
	    glCallList(scene_list);
    }
	    

	int time = glutGet(GLUT_ELAPSED_TIME);
	float fps = kuhl_getfps(time);
	if(time % 1000 == 0)
		printf("Frames per second: %0.1f\n", fps);
		
	
	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

	glFlush();
	glFinish();
	
	/* Display the buffer we just drew (necessary for double buffering). */
	glutSwapBuffers();

	/* Ask GLUT to call display() again. We shouldn't call display()
	 * ourselves recursively because it will not leave time for GLUT
	 * to call other callback functions for when a key is pressed, the
	 * window is resized, etc. */
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	if(argc == 2)
	{
		modelFilename = argv[1];
		modelTexturePath = NULL;
	}
	else if(argc == 3)
	{
		modelFilename = argv[1];
		modelTexturePath = argv[2];
	}
	else
	{
		printf("Usage:\n"
		       "%s modelFile     - Textures are assumed to be in the same directory as the model.\n"
		       "- or -\n"
		       "%s modelFile texturePath\n", argv[0], argv[0]);
		exit(1);
	}

	/* set up our GLUT window */
	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	/* Ask GLUT to for a double buffered, full color window that
	 * includes a depth buffer */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow(argv[0]); // set window title to executable name

	/* Initialize GLEW */
	GLenum glewError = glewInit();
	if(glewError != GLEW_OK)
	{
		fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(glewError));
		exit(EXIT_FAILURE);
	}
	kuhl_errorcheck();

	// setup callbacks
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);

	float initPos[3] = {0,0,3};
	float initLook[3] = {0,0,0};
	float initUp[3] = {0,1,0};

	// Initialize DGR
	dgr_init();
	projmat_init();
	viewmat_init(initPos, initLook, initUp);

	// Clear the screen while things might be loading
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
	
	/* Tell GLUT to start running the main loop and to call display(),
	 * keyboard(), etc callback methods as needed. */
	glutMainLoop();
	exit(EXIT_SUCCESS);
}