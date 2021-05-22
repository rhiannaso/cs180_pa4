/*
 * Program 4 example with diffuse and spline camera PRESS 'g'
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>
#define PI 3.1415927

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong has diffuse
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

    //Our shader program for skybox
	std::shared_ptr<Program> cubeProg;

	//our geometry
	shared_ptr<Shape> sphere;

	shared_ptr<Shape> theDog;

	shared_ptr<Shape> cube;

    shared_ptr<Shape> roadObj;
    shared_ptr<Shape> lampMesh;
    vector<shared_ptr<Shape>> houseMesh;
    vector<shared_ptr<Shape>> treeMesh;
    vector<shared_ptr<Shape>> carMesh;

    vector<tinyobj::material_t> houseMat;
    vector<tinyobj::material_t> carMat;
    map<string, shared_ptr<Texture>> textureMap;

    float driveTheta = 0;

    //skybox data
    vector<std::string> faces {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg"
    }; 
    unsigned int cubeMapTexture;

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	//ground VAO
	GLuint GroundVertexArrayID;

	//the image to use as a texture (ground)
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;	
	shared_ptr<Texture> brownWood;
    shared_ptr<Texture> brick;
    shared_ptr<Texture> blackText;
    shared_ptr<Texture> lightWood;
    shared_ptr<Texture> leaf;
    shared_ptr<Texture> road;
    shared_ptr<Texture> bumpBrick;
    shared_ptr<Texture> whiteText;
    shared_ptr<Texture> tiles;
    shared_ptr<Texture> stone;
    shared_ptr<Texture> lamp;
    shared_ptr<Texture> whiteWood;
    shared_ptr<Texture> glass;

	//animation data
	float lightTrans = 0;

	//camera
	double g_phi, g_theta;
	vec3 view = vec3(0, 0, 1);
	vec3 strafe = vec3(1, 0, 0);
	vec3 g_eye = vec3(0, 0.5, 5);
	vec3 g_lookAt = vec3(0, 0.5, -4);
    float speed = 0.3;

	Spline splinepath[2];
	bool goCamera = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans -= 0.5;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans += 0.5;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
			goCamera = !goCamera;
		}
        if (key == GLFW_KEY_W && action == GLFW_PRESS){
			view = g_lookAt - g_eye;
            g_eye = g_eye + (speed*view);
            g_lookAt = g_lookAt + (speed*view);
		}
        if (key == GLFW_KEY_A && action == GLFW_PRESS){
            view = g_lookAt - g_eye;
            strafe = cross(view, vec3(0, 1, 0));
			g_eye = g_eye - (speed*strafe);
            g_lookAt = g_lookAt - (speed*strafe);
		}
        if (key == GLFW_KEY_S && action == GLFW_PRESS){
			view = g_lookAt - g_eye;
            g_eye = g_eye - (speed*view);
            g_lookAt = g_lookAt - (speed*view);
		}
        if (key == GLFW_KEY_D && action == GLFW_PRESS){
            view = g_lookAt - g_eye;
            strafe = cross(view, vec3(0, 1, 0));
			g_eye = g_eye + (speed*strafe);
            g_lookAt = g_lookAt + (speed*strafe);
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}


	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
        g_theta -= deltaX/100;
        if (g_phi < DegToRad(80) && g_phi > -DegToRad(80)) {
            g_phi += deltaY/100;
        }
        if (g_phi >= DegToRad(80) && deltaY < 0) {
            g_phi += deltaY/100;
        }
        if (g_phi <= DegToRad(80) && deltaY > 0) {
            g_phi += deltaY/100;
        }
        computeLookAt();
	}

    float DegToRad(float degrees) {
        return degrees*(PI/180.0);
    }

    void computeLookAt() {
        float radius = 1.0;
        float x = radius*cos(g_phi)*cos(g_theta);
        float y = radius*sin(g_phi);
        float z = radius*cos(g_phi)*cos((PI/2.0)-g_theta);
        g_lookAt = vec3(x, y, z) + g_eye;
    }

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

    void setTextures(const std::string& resourceDirectory) {
        //read in a load the texture
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/grass_low.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_REPEAT, GL_REPEAT);

  		texture1 = make_shared<Texture>();
  		texture1->setFilename(resourceDirectory + "/skyBox/back.jpg");
  		texture1->init();
  		texture1->setUnit(1);
  		texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

  		brownWood = make_shared<Texture>();
  		brownWood->setFilename(resourceDirectory + "/cartoonWood.jpg");
  		brownWood->init();
  		brownWood->setUnit(2);
  		brownWood->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        brick = make_shared<Texture>();
  		brick->setFilename(resourceDirectory + "/brickHouse/campiangatebrick1.jpg");
  		brick->init();
  		brick->setUnit(3);
  		brick->setWrapModes(GL_REPEAT, GL_REPEAT);
        textureMap.insert(pair<string, shared_ptr<Texture>>("campiangatebrick1.jpg", brick));

        blackText = make_shared<Texture>();
  		blackText->setFilename(resourceDirectory + "/brickHouse/063.jpg");
  		blackText->init();
  		blackText->setUnit(4);
  		blackText->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        textureMap.insert(pair<string, shared_ptr<Texture>>("063.JPG", blackText));

        lightWood = make_shared<Texture>();
  		lightWood->setFilename(resourceDirectory + "/brown_wood.jpg");
  		lightWood->init();
  		lightWood->setUnit(5);
  		lightWood->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        leaf = make_shared<Texture>();
  		leaf->setFilename(resourceDirectory + "/tree/maple_leaf.png");
  		leaf->init();
  		leaf->setUnit(6);
  		leaf->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        road = make_shared<Texture>();
  		road->setFilename(resourceDirectory + "/asphalt3.jpg");
  		road->init();
  		road->setUnit(7);
  		road->setWrapModes(GL_REPEAT, GL_REPEAT);

        bumpBrick = make_shared<Texture>();
  		bumpBrick->setFilename(resourceDirectory + "/brickHouse/campiangatebrick1_bump.jpg");
  		bumpBrick->init();
  		bumpBrick->setUnit(8);
  		bumpBrick->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        textureMap.insert(pair<string, shared_ptr<Texture>>("campiangatebrick1_bump.jpg", bumpBrick));

        whiteText = make_shared<Texture>();
  		whiteText->setFilename(resourceDirectory + "/brickHouse/HighBuild_texture.jpg");
  		whiteText->init();
  		whiteText->setUnit(9);
  		whiteText->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        textureMap.insert(pair<string, shared_ptr<Texture>>("HighBuild_texture.jpg", whiteText));

        tiles = make_shared<Texture>();
  		tiles->setFilename(resourceDirectory + "/brickHouse/panTiles_1024_more_red.jpg");
  		tiles->init();
  		tiles->setUnit(10);
  		tiles->setWrapModes(GL_REPEAT, GL_REPEAT);
        textureMap.insert(pair<string, shared_ptr<Texture>>("panTiles_1024_more_red.jpg", tiles));

        stone = make_shared<Texture>();
  		stone->setFilename(resourceDirectory + "/brickHouse/stones006x04.jpg");
  		stone->init();
  		stone->setUnit(11);
  		stone->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        textureMap.insert(pair<string, shared_ptr<Texture>>("stones006x04.jpg", stone));

        lamp = make_shared<Texture>();
  		lamp->setFilename(resourceDirectory + "/diffuse_streetlamp.jpg");
  		lamp->init();
  		lamp->setUnit(12);
  		lamp->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        whiteWood = make_shared<Texture>();
  		whiteWood->setFilename(resourceDirectory + "/white_wood.jpg");
  		whiteWood->init();
  		whiteWood->setUnit(13);
  		whiteWood->setWrapModes(GL_REPEAT, GL_REPEAT);

        glass = make_shared<Texture>();
  		glass->setFilename(resourceDirectory + "/glass.jpg");
  		glass->init();
  		glass->setUnit(13);
  		glass->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    }

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		g_theta = -PI/2.0;

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		//prog->addAttribute("vertTex"); //silence error


		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatShine");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

        setTextures(resourceDirectory);

        // Initialize the GLSL program that we will use for texture mapping
		cubeProg = make_shared<Program>();
		cubeProg->setVerbose(true);
		cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		cubeProg->init();
		cubeProg->addUniform("P");
		cubeProg->addUniform("V");
		cubeProg->addUniform("M");
		cubeProg->addUniform("skybox");
		cubeProg->addAttribute("vertPos");
		cubeProg->addAttribute("vertNor");

  		// init splines up and down
       splinepath[0] = Spline(glm::vec3(-6,3,5), glm::vec3(-1,0,5), glm::vec3(1, 5, 5), glm::vec3(3,3,5), 5);
       splinepath[1] = Spline(glm::vec3(3,3,5), glm::vec3(4,1,5), glm::vec3(-0.75, 0.25, 5), glm::vec3(0,0,5), 5);
    
	}

    unsigned int createSky(string dir, vector<string> faces) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);
        for(GLuint i = 0; i < faces.size(); i++) {
            unsigned char *data = stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            } else {
                cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        cout << " creating cube map any errors : " << glGetError() << endl;
        return textureID;
    }

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapes[0]);
			sphere->measure();
			sphere->init();
		}

		// Initialize bunny mesh.
		vector<tinyobj::shape_t> TOshapesB;
 		vector<tinyobj::material_t> objMaterialsB;
		//load in the mesh and make the shape(s)
 		rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/dog.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {	
			theDog = make_shared<Shape>();
			theDog->createShape(TOshapesB[0]);
			theDog->measure();
			theDog->init();
		}

        rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			cube = make_shared<Shape>();
			cube->createShape(TOshapesB[0]);
			cube->measure();
			cube->init();
		}

        rc = tinyobj::LoadObj(TOshapes, carMat, errStr, (resourceDirectory + "/car/car.obj").c_str(), (resourceDirectory + "/car/").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>();
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                // findMin(tmp->min.x, tmp->min.y, tmp->min.z, "house");
                // findMax(tmp->max.x, tmp->max.y, tmp->max.z, "house");
                
                carMesh.push_back(tmp);
            }
		}

        rc = tinyobj::LoadObj(TOshapes, houseMat, errStr, (resourceDirectory + "/brickHouse/CH_building1.obj").c_str(), (resourceDirectory + "/brickHouse/").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>();
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                // findMin(tmp->min.x, tmp->min.y, tmp->min.z, "house");
                // findMax(tmp->max.x, tmp->max.y, tmp->max.z, "house");
                
                houseMesh.push_back(tmp);
            }
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/streetlamp.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
            lampMesh = make_shared<Shape>();
            lampMesh->createShape(TOshapes[0]);
            lampMesh->measure();
            lampMesh->init();
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/tree/MapleTree.obj").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
            for (int i = 0; i < TOshapes.size(); i++) {
                shared_ptr<Shape> tmp = make_shared<Shape>();
                tmp->createShape(TOshapes[i]);
                tmp->measure();
                tmp->init();

                // findMin(tmp->min.x, tmp->min.y, tmp->min.z, "santa");
                // findMax(tmp->max.x, tmp->max.y, tmp->max.z, "santa");

                treeMesh.push_back(tmp);
            }
		}

        rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/road.obj").c_str());
        if (!rc) {
			cerr << errStr << endl;
		} else {
			roadObj = make_shared<Shape>();
			roadObj->createShape(TOshapes[0]);
			roadObj->measure();
			roadObj->init();
		}


        cubeMapTexture = createSky("../resources/sky/", faces);

		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 20;
		float g_groundY = -0.25;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
     void drawGround(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	texture0->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
  		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

     //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

    	switch (i) {
    		case 0: //purple
    			glUniform3f(curS->getUniform("MatAmb"), 0.096, 0.046, 0.095);
    			glUniform3f(curS->getUniform("MatDif"), 0.96, 0.46, 0.95);
    			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 120.0);
    		break;
    		case 1: // pink
    			glUniform3f(curS->getUniform("MatAmb"), 0.063, 0.038, 0.1);
    			glUniform3f(curS->getUniform("MatDif"), 0.63, 0.38, 1.0);
    			glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.2, 0.5);
    			glUniform1f(curS->getUniform("MatShine"), 4.0);
    		break;
    		case 2: 
    			glUniform3f(curS->getUniform("MatAmb"), 0.004, 0.05, 0.09);
    			glUniform3f(curS->getUniform("MatDif"), 0.04, 0.5, 0.9);
    			glUniform3f(curS->getUniform("MatSpec"), 0.02, 0.25, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 27.9);
    		break;
  		}
	}

    void SetGenericMat(shared_ptr<Program> curS, float ambient[3], float diffuse[3], float specular[3], float shininess, string type) {
        if (type == "house") {
            glUniform1f(curS->getUniform("MatShine"), shininess);
        } else {
            if (type == "car") {
                glUniform3f(curS->getUniform("MatAmb"), 0.05, 0.05, 0.05);
            } else {
                glUniform3f(curS->getUniform("MatAmb"), ambient[0], ambient[1], ambient[2]);
            }
            glUniform3f(curS->getUniform("MatDif"), diffuse[0], diffuse[1], diffuse[2]);
            glUniform3f(curS->getUniform("MatSpec"), specular[0], specular[1], specular[2]);
            glUniform1f(curS->getUniform("MatShine"), shininess);
        }
    }

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

   	/* camera controls - do not change */
	void SetView(shared_ptr<Program>  shader) {
  		glm::mat4 Cam = glm::lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
  		glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
	}

   	/* code to draw waving hierarchical model */
   	void drawHierModel(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog) {
   		// simplified for releaes code
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, 0, -6));
			Model->scale(vec3(2.3));
			setModel(prog, Model);
			sphere->draw(prog);
		Model->popMatrix();
   	}

    void drawHouse(shared_ptr<MatrixStack> Model, shared_ptr<Program> drawProg) {
        Model->pushMatrix();
            Model->pushMatrix();
                Model->translate(vec3(0, -1.25, -7));
                Model->scale(vec3(0.25, 0.25, 0.25));

                setModel(drawProg, Model);
                for (int i=0; i < houseMesh.size(); i++) {
                    int mat = houseMesh[i]->getMat()[0];
                    SetGenericMat(drawProg, houseMat[mat].ambient, houseMat[mat].diffuse, houseMat[mat].specular, houseMat[mat].shininess, "house");
                    if (houseMat[mat].diffuse_texname != "") {
                        if (i == 260 || i == 264) {
                            glUniform1i(drawProg->getUniform("flip"), 0);
                        } else {
                            glUniform1i(drawProg->getUniform("flip"), 1);
                        } 
                        if (houseMat[mat].diffuse_texname == "063.JPG") {
                            lightWood->bind(drawProg->getUniform("Texture0"));
                        } else {
                            textureMap.at(houseMat[mat].diffuse_texname)->bind(drawProg->getUniform("Texture0"));
                        }
                    } else {
                        if (mat == 10) {
                            glass->bind(drawProg->getUniform("Texture0"));
                        } else if (mat == 4) {
                            blackText->bind(drawProg->getUniform("Texture0"));
                        } else {
                            whiteWood->bind(drawProg->getUniform("Texture0"));
                        }
                    }
                    houseMesh[i]->draw(drawProg);
                }
            Model->popMatrix();
        Model->popMatrix();
    }

    void drawTree(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog, float xPos, float zPos) {
        Model->pushMatrix();
            Model->pushMatrix();
                Model->translate(vec3(xPos, -1.25, zPos));
                Model->scale(vec3(0.09, 0.09, 0.09));

                setModel(prog, Model);
                for (int i=0; i < treeMesh.size(); i++) {
                    if (i == 0) {
                        lightWood->bind(prog->getUniform("Texture0"));
                    } else {
                        leaf->bind(prog->getUniform("Texture0"));
                    }
                    treeMesh[i]->draw(prog);
                }
            Model->popMatrix();
        Model->popMatrix();
    }

    void drawLamps(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog) {
        Model->pushMatrix();

            float zVals[4] = {15, 9, 3, -3};
            lamp->bind(prog->getUniform("Texture0"));

            for (int l=0; l < 4; l++) {
                Model->pushMatrix();
                    Model->translate(vec3(4.5, -1.25, zVals[l]));
                    Model->scale(vec3(1, 1, 1));
                    setModel(prog, Model);
                    lampMesh->draw(prog);
                Model->popMatrix();
            }

            for (int r=0; r < 4; r++) {
                Model->pushMatrix();
                    Model->translate(vec3(-4.5, -1.25, zVals[r]));
                    Model->rotate(3.14159, vec3(0, 1, 0));
                    Model->scale(vec3(1, 1, 1));
                    setModel(prog, Model);
                    lampMesh->draw(prog);
                Model->popMatrix();
            }
        Model->popMatrix();
    }

    void drawCar(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog) {
        driveTheta = 1.5*sin(glfwGetTime());

        Model->pushMatrix();
            Model->translate(vec3(2.7, -0.85, 2));
            Model->translate(vec3(0, 0, driveTheta));
            Model->scale(vec3(0.3, 0.3, 0.3));

            setModel(prog, Model);
            float diffuse[3] = {0.840000, 0.332781, 0.311726};
            for (int i=0; i < carMesh.size(); i++) {
                int mat = carMesh[i]->getMat()[0];
                SetGenericMat(prog, carMat[mat].ambient, carMat[mat].diffuse, carMat[mat].specular, carMat[mat].shininess, "car");
                carMesh[i]->draw(prog);
            }
        Model->popMatrix();
    }

    void drawRoad(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog) {
        Model->pushMatrix();
            Model->pushMatrix();
                Model->translate(vec3(0, -1.24, 5));
                Model->rotate(3.1416, vec3(0, 0, 1));
                // Model->scale(vec3(2, 0.01, 5));
                Model->scale(vec3(1.9, 0.1, 3));

                glUniform1i(prog->getUniform("flip"), 0);
                road->bind(prog->getUniform("Texture0"));
                setModel(prog, Model);
                roadObj->draw(prog);
            Model->popMatrix();
        Model->popMatrix();
    }

   	void updateUsingCameraPath(float frametime)  {

   	  if (goCamera) {
       if(!splinepath[0].isDone()){
       		splinepath[0].update(frametime);
            g_eye = splinepath[0].getPosition();
        } else {
            splinepath[1].update(frametime);
            g_eye = splinepath[1].getPosition();
        }
      }
   	}

	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		//update the camera position
		updateUsingCameraPath(frametime);

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// Draw the doggos
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		SetView(texProg);
		glUniform3f(texProg->getUniform("lightPos"), 2.0+lightTrans, 9.0, 6);
        glUniform1i(texProg->getUniform("flip"), 1);

        drawHouse(Model, texProg);
        drawTree(Model, texProg, 5, -4);
        drawTree(Model, texProg, -5, -4);
        vector<vector<float>> pos = {{6, 2}, {-12, 4}, {-10, -6}, {12, 7}, {8, -12}, {-8, 10}, {14, -2}, {-6, -1}, {-5, 4}, {8, 10}, {7, -4}};
        for (int i=0; i < pos.size(); i++) {
            drawTree(Model, texProg, pos[i][0], pos[i][1]);
        }
        drawLamps(Model, texProg);
        drawRoad(Model, texProg);

		// //draw big background sphere
		// glUniform1i(texProg->getUniform("flip"), 0);
		// texture1->bind(texProg->getUniform("Texture0"));
		// Model->pushMatrix();
		// 	Model->loadIdentity();
		// 	Model->scale(vec3(8.0));
		// 	setModel(texProg, Model);
		// 	sphere->draw(texProg);
		// Model->popMatrix();

        glUniform1i(texProg->getUniform("flip"), 1);
		drawGround(texProg);

		texProg->unbind();

        //to draw the sky box bind the right shader
        cubeProg->bind();
        //set the projection matrix - can use the same one
        glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        //set the depth function to always draw the box!
        glDepthFunc(GL_LEQUAL);
        //set up view matrix to include your view transforms
        //(your code likely will be different depending
        SetView(cubeProg);
        //set and send model transforms - likely want a bigger cube
        Model->pushMatrix();
        Model->scale(vec3(35, 35, 35));
        glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE,value_ptr(Model->topMatrix()));
        Model->popMatrix();
        //bind the cube map texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

        //draw the actual cube
        cube->draw(cubeProg);

        //set the depth test back to normal!
        glDepthFunc(GL_LESS);
        //unbind the shader for the skybox
        cubeProg->unbind(); 

		//use the material shader
		prog->bind();
		//set up all the matrices
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		SetView(prog);
		glUniform3f(prog->getUniform("lightPos"), 2.0+lightTrans, 9.0, 6);
        drawCar(Model, prog);
		//draw the waving HM
		// SetMaterial(prog, 1);
		// drawHierModel(Model, prog);
		prog->unbind();

		// Pop matrix stacks.
		Projection->popMatrix();

	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
