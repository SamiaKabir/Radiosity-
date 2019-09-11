#include "GL/glut.h"
#include "fstream.h"
#include "math.h"

typedef struct { double x, y, z; } Vertex;
typedef Vertex Vector;
typedef struct { float r, g, b; } Color;

typedef struct {
   int vertices[4]; // array of vertices (index into VertexArray)
   Color emissivity; // emitted radiosity
   Color reflectance; // reflectance
   Vertex center; // center of the patch
   Vector normal; // patch normal
   double area; // area of the patch
   Color radiosity; // radiosity of the patch
   Color unshot; // unshot radiosity of the patch
   int numelements; // number of elements in patch
   int startelement;  // number of first element for this patch in ElementArray
} Patch;

typedef struct{
   int vertices[4];  // vertices of patch (index into VertexArray)
   Vertex center;  // center of the element
   double area;  // area of the element
   Patch* patch;  // Patch that this is an element of
   Color radiosity; // radiosity of the element
} Element;

int NumVertices;
Vertex* VertexArray;
Color* VertexColors;
int NumPatches;
Patch* PatchArray;
int NumElements;
Element* ElementArray;

inline double dot(Vector a, Vector b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

void DoStep(void) {
	// Do one step of Progressive Refinement
}

int LoadData(void) {

	int i,j,k;
	int nverts, vertnum, startvert;
	int elnum;
    Vertex* vtemp;
	ifstream infi("Test.dat");
	Vector v1;
	Vector v2;
	double length1, length2;
	double temparea;

	NumElements=0;

	// read initial vertices
	infi >> nverts;

	vtemp = new Vertex[nverts];
	for(i=0;i<nverts;i++) {
		infi >> vtemp[i].x >> vtemp[i].y >> vtemp[i].z;
	}
    NumVertices = nverts;

	// read patches
	infi >> NumPatches;
	PatchArray = new Patch[NumPatches];
	for (i=0;i<NumPatches;i++) {
		// Read patch i
		infi >> PatchArray[i].vertices[0] >> PatchArray[i].vertices[1]
			 >> PatchArray[i].vertices[2] >> PatchArray[i].vertices[3];
		infi >> PatchArray[i].emissivity.r >> PatchArray[i].emissivity.g
			 >> PatchArray[i].emissivity.b;
		infi >> PatchArray[i].reflectance.r >> PatchArray[i].reflectance.g
			 >> PatchArray[i].reflectance.b;
		infi >> PatchArray[i].numelements;

		NumVertices += (PatchArray[i].numelements + 1) *
			           (PatchArray[i].numelements + 1);
		PatchArray[i].startelement = NumElements;
		NumElements += (PatchArray[i].numelements * PatchArray[i].numelements);

		PatchArray[i].center.x = (vtemp[PatchArray[i].vertices[0]].x +
								  vtemp[PatchArray[i].vertices[1]].x +
								  vtemp[PatchArray[i].vertices[2]].x +
								  vtemp[PatchArray[i].vertices[3]].x)/4.0;
		PatchArray[i].center.y = (vtemp[PatchArray[i].vertices[0]].y +
								  vtemp[PatchArray[i].vertices[1]].y +
								  vtemp[PatchArray[i].vertices[2]].y +
								  vtemp[PatchArray[i].vertices[3]].y)/4.0;
		PatchArray[i].center.z = (vtemp[PatchArray[i].vertices[0]].z +
								  vtemp[PatchArray[i].vertices[1]].z +
								  vtemp[PatchArray[i].vertices[2]].z +
								  vtemp[PatchArray[i].vertices[3]].z)/4.0;

		v1.x = vtemp[PatchArray[i].vertices[1]].x -
			   vtemp[PatchArray[i].vertices[0]].x;
		v1.y = vtemp[PatchArray[i].vertices[1]].y -
			   vtemp[PatchArray[i].vertices[0]].y;
		v1.z = vtemp[PatchArray[i].vertices[1]].z -
			   vtemp[PatchArray[i].vertices[0]].z;
		v2.x = vtemp[PatchArray[i].vertices[3]].x -
			   vtemp[PatchArray[i].vertices[0]].x;
		v2.y = vtemp[PatchArray[i].vertices[3]].y -
			   vtemp[PatchArray[i].vertices[0]].y;
		v2.z = vtemp[PatchArray[i].vertices[3]].z -
			   vtemp[PatchArray[i].vertices[0]].z;
        length1 = sqrt(v1.x*v1.x + v1.y*v1.y + v1.z*v1.z);
		length2 = sqrt(v2.x*v2.x + v2.y*v2.y + v2.z*v2.z);
		PatchArray[i].area = length1*length2;
		v1.x /= length1;
		v1.y /= length1;
		v1.z /= length1;
		v2.x /= length2;
		v2.y /= length2;
		v2.z /= length2;
		PatchArray[i].normal.x = v1.y*v2.z - v1.z*v2.y;
		PatchArray[i].normal.y = v2.x*v1.z - v1.x*v2.z;
		PatchArray[i].normal.z = v1.x*v2.y - v1.y*v2.x;
		PatchArray[i].radiosity.r = PatchArray[i].emissivity.r;
		PatchArray[i].radiosity.g = PatchArray[i].emissivity.g;
		PatchArray[i].radiosity.b = PatchArray[i].emissivity.b;
		PatchArray[i].unshot.r = PatchArray[i].emissivity.r;
		PatchArray[i].unshot.g = PatchArray[i].emissivity.g;
		PatchArray[i].unshot.b = PatchArray[i].emissivity.b;
	}

	// create elements (including new vertices)
	VertexArray = new Vertex[NumVertices];
	VertexColors = new Color[NumVertices];
	ElementArray = new Element[NumElements];
	// Copy original (patch) vertices to beginning of array
	for(i=0;i<nverts;i++) {
		VertexArray[i].x=vtemp[i].x;
		VertexArray[i].y=vtemp[i].y;
		VertexArray[i].z=vtemp[i].z;
		VertexColors[i].r = 0.0;
		VertexColors[i].g = 0.0;
		VertexColors[i].b = 0.0;
	}

	// Form Vertices for new elements
	vertnum = nverts;
	elnum = 0;
	for(i=0;i<NumPatches;i++) {
		v1.x = (VertexArray[PatchArray[i].vertices[1]].x -
			    VertexArray[PatchArray[i].vertices[0]].x) /
			   PatchArray[i].numelements;
		v1.y = (VertexArray[PatchArray[i].vertices[1]].y -
			    VertexArray[PatchArray[i].vertices[0]].y) /
			   PatchArray[i].numelements;
		v1.z = (VertexArray[PatchArray[i].vertices[1]].z -
			    VertexArray[PatchArray[i].vertices[0]].z) /
			   PatchArray[i].numelements;
		v2.x = (VertexArray[PatchArray[i].vertices[3]].x -
			    VertexArray[PatchArray[i].vertices[0]].x) /
			   PatchArray[i].numelements;
		v2.y = (VertexArray[PatchArray[i].vertices[3]].y -
			    VertexArray[PatchArray[i].vertices[0]].y) /
			   PatchArray[i].numelements;
		v2.z = (VertexArray[PatchArray[i].vertices[3]].z -
			    VertexArray[PatchArray[i].vertices[0]].z) /
			   PatchArray[i].numelements;

		startvert = vertnum;

		for(j=0;j<PatchArray[i].numelements+1;j++) {
			for(k=0;k<PatchArray[i].numelements+1;k++) {
				// Create new vertex
				VertexArray[vertnum].x =
					VertexArray[PatchArray[i].vertices[0]].x +
					k * v1.x + j * v2.x;
				VertexArray[vertnum].y =
					VertexArray[PatchArray[i].vertices[0]].y +
					k * v1.y + j * v2.y;
				VertexArray[vertnum].z =
					VertexArray[PatchArray[i].vertices[0]].z +
					k * v1.z + j * v2.z;
				VertexColors[vertnum].r = 0.0;
				VertexColors[vertnum].g = 0.0;
				VertexColors[vertnum].b = 0.0;

				vertnum++;
			}
		}
		temparea = PatchArray[i].area /
			       (PatchArray[i].numelements * PatchArray[i].numelements);
		// Form Elements for new patch
		for(j=0;j<PatchArray[i].numelements;j++) {
			for(k=0;k<PatchArray[i].numelements;k++) {
				// Set vertices
				ElementArray[elnum].vertices[0] = startvert + k +
					               j * (PatchArray[i].numelements + 1);
				ElementArray[elnum].vertices[1] = startvert + (k+1) +
					               j * (PatchArray[i].numelements + 1);
				ElementArray[elnum].vertices[2] = startvert + (k+1) +
					               (j+1) * (PatchArray[i].numelements + 1);
				ElementArray[elnum].vertices[3] = startvert + k +
					               (j+1) * (PatchArray[i].numelements + 1);
//VertexColors[ElementArray[elnum].vertices[0]].r = PatchArray[i].reflectance.r;
//VertexColors[ElementArray[elnum].vertices[0]].g = PatchArray[i].reflectance.g;
//VertexColors[ElementArray[elnum].vertices[0]].b = PatchArray[i].reflectance.b;
//VertexColors[ElementArray[elnum].vertices[1]].r = PatchArray[i].reflectance.r;
//VertexColors[ElementArray[elnum].vertices[1]].g = PatchArray[i].reflectance.g;
//VertexColors[ElementArray[elnum].vertices[1]].b = PatchArray[i].reflectance.b;
//VertexColors[ElementArray[elnum].vertices[2]].r = PatchArray[i].reflectance.r;
//VertexColors[ElementArray[elnum].vertices[2]].g = PatchArray[i].reflectance.g;
//VertexColors[ElementArray[elnum].vertices[2]].b = PatchArray[i].reflectance.b;
//VertexColors[ElementArray[elnum].vertices[3]].r = PatchArray[i].reflectance.r;
//VertexColors[ElementArray[elnum].vertices[3]].g = PatchArray[i].reflectance.g;
//VertexColors[ElementArray[elnum].vertices[3]].b = PatchArray[i].reflectance.b;
				// Set center
				ElementArray[elnum].center.x =
				                (VertexArray[ElementArray[elnum].vertices[0]].x +
				                 VertexArray[ElementArray[elnum].vertices[1]].x +
				                 VertexArray[ElementArray[elnum].vertices[2]].x +
				                 VertexArray[ElementArray[elnum].vertices[3]].x)/4.0;
				ElementArray[elnum].center.y =
				                (VertexArray[ElementArray[elnum].vertices[0]].y +
				                 VertexArray[ElementArray[elnum].vertices[1]].y +
				                 VertexArray[ElementArray[elnum].vertices[2]].y +
				                 VertexArray[ElementArray[elnum].vertices[3]].y)/4.0;
				ElementArray[elnum].center.z =
				                (VertexArray[ElementArray[elnum].vertices[0]].z +
				                 VertexArray[ElementArray[elnum].vertices[1]].z +
				                 VertexArray[ElementArray[elnum].vertices[2]].z +
				                 VertexArray[ElementArray[elnum].vertices[3]].z)/4.0;

				ElementArray[elnum].area = temparea;
				ElementArray[elnum].patch = &PatchArray[i];
				ElementArray[elnum].radiosity = PatchArray[i].radiosity;
				elnum++;
			}
		}

	}

	delete[] vtemp;
	infi.close();
	return 0;
}

void display(void) {
	int i;
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
	for(i=0;i<NumElements;i++) {

		glColor3f(VertexColors[ElementArray[i].vertices[0]].r,
			      VertexColors[ElementArray[i].vertices[0]].g,
				  VertexColors[ElementArray[i].vertices[0]].b);
		glVertex3f(VertexArray[ElementArray[i].vertices[0]].x,
			       VertexArray[ElementArray[i].vertices[0]].y,
				   VertexArray[ElementArray[i].vertices[0]].z);
        glColor3f(VertexColors[ElementArray[i].vertices[1]].r,
                  VertexColors[ElementArray[i].vertices[1]].g,
				  VertexColors[ElementArray[i].vertices[1]].b);
		glVertex3f(VertexArray[ElementArray[i].vertices[1]].x,
			       VertexArray[ElementArray[i].vertices[1]].y,
				   VertexArray[ElementArray[i].vertices[1]].z);
        glColor3f(VertexColors[ElementArray[i].vertices[2]].r,
                  VertexColors[ElementArray[i].vertices[2]].g,
				  VertexColors[ElementArray[i].vertices[2]].b);
		glVertex3f(VertexArray[ElementArray[i].vertices[2]].x,
			       VertexArray[ElementArray[i].vertices[2]].y,
				   VertexArray[ElementArray[i].vertices[2]].z);
        glColor3f(VertexColors[ElementArray[i].vertices[3]].r,
                  VertexColors[ElementArray[i].vertices[3]].g,
				  VertexColors[ElementArray[i].vertices[3]].b);
		glVertex3f(VertexArray[ElementArray[i].vertices[3]].x,
		           VertexArray[ElementArray[i].vertices[3]].y,
				   VertexArray[ElementArray[i].vertices[3]].z);
	}
	glEnd();

	glFlush();
}

void init(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	LoadData();
}

void mouse(int button, int state, int x, int y) {
    static int pressed = 0;

	if (button == GLUT_LEFT_BUTTON) {
		if ((state == GLUT_DOWN) && (pressed == 0)) {
			DoStep();
			pressed = 1;
		} else if ((state == GLUT_UP) && (pressed == 1)) {
			pressed = 0;
		}
	}
}

void reshape(int w, int h) {
	glViewport(0,0,(GLsizei)w,(GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-5.0, 5.0, -4.0, 4.0, 10.1, 25.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(20.0, 5.0, 4.0,
		      10.0, 5.0, 4.0,
			  0.0, 0.0, 1.0);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB|GLUT_DEPTH);
	glutInitWindowSize(500,400);
	glutInitWindowPosition(100,100);
	glutCreateWindow("Samia Kabir - Assignment 3");
	init();
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutReshapeFunc(reshape);
	glutMainLoop();
	return 0;
}
