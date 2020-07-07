#include <iostream>
#include "GL/glew.h"
#include "GL/3dgl.h"
#include "GL/glut.h"
#include "GL/freeglut_ext.h"

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// Include GLM extensions
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D models
C3dglModel camera;
C3dglModel table;
C3dglModel vase;
C3dglModel dino;
C3dglModel living;
C3dglModel lamp;
C3dglModel lightbulb;

//textures
C3dglBitmap tableAlbedo;
C3dglBitmap tableNormal;
C3dglBitmap chairAlbedo;
C3dglBitmap chairNormal;
GLuint whiteTextureId;
GLuint blackTextureId;
GLuint grayTextureId;
GLuint tableAlbedoTexId;
GLuint tableNormalTexId;
GLuint chairAlbedoTexId;
GLuint chairNormalTexId;

//shader
C3dglProgram Program;

// camera position (for first person type camera navigation)
mat4 matrixView;			// The View Matrix
float angleTilt = 15;		// Tilt Angle
float angleRot = 0.1f;		// Camera orbiting angle
vec3 cam(0);				// Camera movement values

float vertices[] = {
			-4, 0, -4, 4, 0, -4, 0, 7, 0, -4, 0, 4, 4, 0, 4, 0, 7, 0,
			-4, 0, -4, -4, 0, 4, 0, 7, 0, 4, 0, -4, 4, 0, 4, 0, 7, 0,
			-4, 0, -4, -4, 0, 4, 4, 0, -4, 4, 0, 4 };
float normals[] = {
			 0, 4, -7, 0, 4, -7, 0, 4, -7, 0, 4, 7, 0, 4, 7, 0, 4, 7,
			-7, 4, 0, -7, 4, 0, -7, 4, 0, 7, 4, 0, 7, 4, 0, 7, 4, 0,
			 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 };
unsigned indices[] = {
			 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 13, 14, 15 };

unsigned vertexBuffer = 0;
unsigned normalBuffer = 0;
unsigned indexBuffer = 0;


// structures that represent directional/point lights.
// They pass parameters to shaders. This way we don't duplicate the uniform setting code,
// and also are able to will parameters more verbosily.
struct DirectionalLight
{
    int on = 0;

    vec3 direction = vec3(0, -1, 0);

    vec3 ambient = vec3(0, 0, 0);

    vec3 diffuse = vec3(0, 0, 0);
    float diffuseStrength = 1;

    vec3 specular = vec3(0, 0, 0);
    float specularPower = 1;

    DirectionalLight& enable(bool yes)
    {
        this->on = yes ? 1 : 0;
        return *this;
    }

    DirectionalLight& withAmbient(float r, float g, float b)
    {
        this->ambient = vec3(r, g, b);
        return *this;
    }
    
    DirectionalLight& withDirection(float x, float y, float z)
    {
        this->direction = normalize(vec3(x, y, z));
        return *this;
    }

    DirectionalLight& withDirection(vec3 v) 
    { return this->withDirection(v.x, v.y, v.z); }

    DirectionalLight& withDiffuse(float r, float g, float b, float strength = 1)
    {
        this->diffuse = vec3(r, g, b);
        this->diffuseStrength = strength;
        return *this;
    }

    DirectionalLight& withSpecular(float r, float g, float b, float power = 1)
    {
        this->specular = vec3(r, g, b);
        this->specularPower = power;
        return *this;
    }

    void apply()
    {
	    Program.SendUniform("lightDirectional.on", on);
	    Program.SendUniform("lightDirectional.ambient", ambient.x, ambient.y, ambient.z);
	    Program.SendUniform("lightDirectional.direction", direction.x, direction.y, direction.z);
	    Program.SendUniform("lightDirectional.diffuse", diffuse.x, diffuse.y, diffuse.z);
	    Program.SendUniform("lightDirectional.diffuseStrength", diffuseStrength);
	    Program.SendUniform("lightDirectional.specular", specular.x, specular.y, specular.z);
	    Program.SendUniform("lightDirectional.specularPower", specularPower);
    }
};

struct PointLight
{
    const int index;

    int on = 0;

    vec3 position = vec3(0, 0, 0);

    vec3 ambient = vec3(0, 0, 0);

    vec3 diffuse = vec3(0, 0, 0);
    float diffuseStrength = 1;

    vec3 specular = vec3(0, 0, 0);
    float specularPower = 0;

    float radius = 1.f;
    float cutoff = 0.f;

    PointLight(int index) : index(index)
    { }
    
    PointLight& enable(bool yes)
    {
        this->on = yes ? 1 : 0;
        return *this;
    }

    PointLight& withAmbient(float r, float g, float b)
    {
        this->ambient = vec3(r, g, b);
        return *this;
    }
    
    PointLight& withPosition(float x, float y, float z)
    {
        this->position = vec3(x, y, z);
        return *this;
    }

    PointLight& withPosition(vec3 v) 
    { return this->withPosition(v.x, v.y, v.z); }

    PointLight& withDiffuse(float r, float g, float b, float strength = 1)
    {
        this->diffuse = vec3(r, g, b);
        this->diffuseStrength = strength;
        return *this;
    }

    PointLight& withSpecular(float r, float g, float b, float power = 1)
    {
        this->specular = vec3(r, g, b);
        this->specularPower = power;
        return *this;
    }

    PointLight& withRadius(float radius)
    {
        this->radius = radius;
        return *this;
    }

    PointLight& withCutoff(float cutoff)
    {
        this->cutoff = cutoff;
        return *this;
    }

    void apply()
    {
        string uniformBase = "lightPoint[";
        uniformBase += to_string(index);
        uniformBase += "]";

        Program.SendUniform(uniformBase + ".on", on);
	    Program.SendUniform(uniformBase + ".ambient", ambient.x, ambient.y, ambient.z);
	    Program.SendUniform(uniformBase + ".position", position.x, position.y, position.z);
	    Program.SendUniform(uniformBase + ".diffuse", diffuse.x, diffuse.y, diffuse.z);
	    Program.SendUniform(uniformBase + ".diffuseStrength", diffuseStrength);
	    Program.SendUniform(uniformBase + ".specular", specular.x, specular.y, specular.z);
	    Program.SendUniform(uniformBase + ".specularPower", specularPower);
	    Program.SendUniform(uniformBase + ".radius", radius);
	    Program.SendUniform(uniformBase + ".cutoff", cutoff);
    }
};

// material structure; kind same as above, to avoid duplication of uniforms code.
struct Material
{
    vec3 ambient = vec3(1, 1, 1);

    vec3 diffuse = vec3(1, 1, 1);
    
    float shininess = 0.f;

    vec3 emissive = vec3(0, 0, 0);
 
    GLuint diffuseTextureId = -1;
    GLuint normalTextureId = -1;

    Material()
    { }

    Material& withAmbientColour(float r, float g, float b)
    {
        this->ambient = vec3(r, g, b);
        return *this;
    }

    Material& withDiffuseColour(float r, float g, float b)
    {
        this->diffuse = vec3(r, g, b);
        return *this;
    }
    
    Material& withShininess(float value)
    {
        this->shininess = value;
        return *this;
    }

    Material& withEmissiveColour(float r, float g, float b, float intensity)
    {
        this->emissive = vec3(r, g, b) * intensity;
        return *this;
    }

    Material& withDiffuseTexture(GLuint diffuseTextureId)
    {
        this->diffuseTextureId = diffuseTextureId;
        return *this;
    }

    Material& withNormalTexture(GLuint normalTextureId)
    {
        this->normalTextureId = normalTextureId;
        return *this;
    }

    void sendTextureUniform(const std::string& uniform, int index, int id)
    {
        GLuint textureIdToBind = (id != -1) ? id : whiteTextureId;
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, textureIdToBind);
        Program.SendUniform(uniform, index);
    }

    void apply()
    {
	    Program.SendUniform("material.ambient", ambient.x, ambient.y, ambient.z);
	    Program.SendUniform("material.diffuse", diffuse.x, diffuse.y, diffuse.z);
	    Program.SendUniform("material.shininess", shininess);
	    Program.SendUniform("material.emissive", emissive.x, emissive.y, emissive.z);

        sendTextureUniform("material.diffuseTexture", 0, diffuseTextureId);
        sendTextureUniform("material.normalTexture", 1, normalTextureId);
    }
};

// same as above. avoid matrix calculation.
struct Model
{
    C3dglModel& model;
    vec3 position;
    float rotation;
    vec3 rotationAxis = vec3(0.0f, 1.0f, 0.0f);

    bool allRotations;
    vec3 rotations;

    float scale = 1.f;
    int mesh = -1;

    Model(C3dglModel& model) : model(model)
    { }

    Model& withPosition(float x, float y, float z)
    {
        this->position = vec3(x, y, z);
        return *this;
    }
    
    Model& withPosition(vec3 v)
    { return this->withPosition(v.x, v.y, v.z); }

    Model& withRotation(float angle)
    {
        this->allRotations = false; // when using rotation axis, disable all rotations
        this->rotation = angle;
        return *this;
    }

    Model& withRotationAxis(float x, float y, float z)
    {
        this->allRotations = false; // when using rotation axis, disable all rotations
        this->rotationAxis = vec3(x, y, z);
        return *this;
    }

    Model& withEuler(float x, float y, float z)
    {
        this->allRotations = true;
        this->rotations = vec3(x, y, z);
        return *this;
    }

    Model& withScale(float scale)
    {
        this->scale = scale;
        return *this;
    }

    Model& withMesh(int meshIndex)
    {
        this->mesh = meshIndex;
        return *this;
    }

    Model& withoutMesh()
    {
        this->mesh = -1;
        return *this;
    }

    // calculate model matrix. 
    // relative to parent if one set.
    mat4 getMatrix()
    {
        mat4 m(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	    m = ::translate(m, this->position);

        if (allRotations)
        {
            // rotate around all axes, y, z, x order
	        m = ::rotate(m, radians(this->rotations.y), vec3(0, 1, 0));
	        m = ::rotate(m, radians(this->rotations.z), vec3(0, 0, 1));
	        m = ::rotate(m, radians(this->rotations.x), vec3(1, 0, 0));
        }
        else
        {
	        m = ::rotate(m, radians(this->rotation), this->rotationAxis);
        }

	    m = ::scale(m, vec3(this->scale, this->scale, this->scale));
        
	    return m;
    }

    void draw(Material& material)
    {
        material.apply();

        mat4 m = matrixView * getMatrix();

        if (this->mesh == -1)
        {
            for (int i = 0; i < this->model.getMeshCount(); ++i)
	            this->model.render(i, m);
        }
        else this->model.render(this->mesh, m);
    }
};


// generate a single pixel texture.
GLuint generateSingleColorGLTexture(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    glActiveTexture(GL_TEXTURE0);

    GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
    
    // single pixel - no need for filtering and always repeat.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLubyte colors[4] = { r, g, b, a };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,GL_UNSIGNED_BYTE, colors);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
		cerr << "GLEW Error: " << glewGetErrorString(error) << endl;
        return -1;
    }

    return id;
}

// load texture from bitmap. by default linear/repeating
GLuint loadGLTexture(C3dglBitmap& bitmap, GLint format = GL_RGBA, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT)
{
    glActiveTexture(GL_TEXTURE0);

    GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
    
    // set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap.GetWidth(), bitmap.GetHeight(), 0, format, GL_UNSIGNED_BYTE, bitmap.GetBits());

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
		cerr << "GLEW Error: " << glewGetErrorString(error) << endl;
        return -1;
    }

    return id;
}


bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glEnable(GL_TEXTURE_2D);	// enable texturing
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// Initialise Shaders
	C3dglShader VertexShader;
	C3dglShader FragmentShader;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/basic.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/basic.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!Program.Create()) return false;
	if (!Program.Attach(VertexShader)) return false;
	if (!Program.Attach(FragmentShader)) return false;
	if (!Program.Link()) return false;
	if (!Program.Use(true)) return false;

	// load your 3D models here!
	if (!table.load("models\\table.obj")) return false;
	if (!vase.load("models\\vase.obj")) return false;
	if (!dino.load("models\\Dinosaur_V02.obj")) return false;
	if (!living.load("models\\LivingRoom.obj")) return false;
	if (!lamp.load("models\\lamp.obj")) return false;
	if (!lightbulb.load("models\\Lightbulb.obj")) return false;

    // load textures
    if (!tableAlbedo.load("models\\table_albedo.jpg", GL_RGBA)) return false;
    if (!tableNormal.load("models\\table_normal.png", GL_RGBA)) return false;
    if (!chairAlbedo.load("models\\chair_albedo.jpg", GL_RGBA)) return false;
    if (!chairNormal.load("models\\chair_normal.png", GL_RGBA)) return false;
    
    whiteTextureId = generateSingleColorGLTexture(255, 255, 255, 255);
    blackTextureId = generateSingleColorGLTexture(0, 0, 0, 0);
    grayTextureId = generateSingleColorGLTexture(127, 127, 127, 127);

    tableAlbedoTexId = loadGLTexture(tableAlbedo);
    tableNormalTexId = loadGLTexture(tableNormal);
    chairAlbedoTexId = loadGLTexture(chairAlbedo);
    chairNormalTexId = loadGLTexture(chairNormal);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1.f), radians(angleTilt), vec3(1.f, 0.f, 0.f));
	matrixView *= lookAt(
		vec3(0.0, 5.0, 10.0),
		vec3(0.0, 5.0, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.18f, 0.25f, 0.22f, 1.0f);   // deep grey background

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Shift+AD or arrow key to auto-orbit" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << "  1, 2 to toggle lamp lights on/off" << endl;
	cout << "  <, > to toggle lamp light colour" << endl;
	cout << "  -, + to decrease/increase light intensity" << endl;
	cout << "  p to toggle lamp lighting mode (1 - default, 2 - low intensity/high specular, 3 - low intensity/high cutoff)" << endl;
	cout << "  o to toggle directional light on/off" << endl;
	cout << endl;
    
	glutSetVertexAttribCoord3(Program.GetAttribLocation("aVertex"));
	glutSetVertexAttribTexCoord2(Program.GetAttribLocation("aTexCoord"));
	glutSetVertexAttribNormal(Program.GetAttribLocation("aNormal"));

	// prepare vertex data
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
	// prepare normal data
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);

	// prepare indices array
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	return true;
}

void done()
{
}


// list of colors that light can have.
#define LIGHT_COLORS 7
vec3 LightColorArray[] = {
    vec3(1, 1, 1),
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1),
    vec3(1, 1, 0),
    vec3(0, 1, 1),
    vec3(1, 0, 1)
};

// light presets - control intensity scale, specular scale, radius and cutoff
struct LightPreset
{
    float intensity;
    float specularPower;
    float radius;
    float cutoff;

    LightPreset(float intensity, float specularPower, float radius, float cutoff) :
        intensity(intensity),
        specularPower(specularPower),
        radius(radius),
        cutoff(cutoff)
    { }
};

#define LIGHT_PRESETS 3
LightPreset LightPresetArray[] = {
    LightPreset(1,      1,     1,    1),
    LightPreset(0.5,  0.025,   2,   40),
    LightPreset(0.2,    1,   1.2,  190),
};

// selected preset...
int currentLightPreset = 0;

// whether directional light is on or off
bool dirLightOn = true;

// holds state about lamp lights, i.e. whether its on, its colour, intensity, etc.
// it also manages the time it takes for the lamp to switch intensity/colours.
// duration - how long it takes to switch
// time - currenr time; updated every frame, but never > duration
// factor - 0 to 1, scale of intensity/colour change (0 - no intensity, no change; 1 - max intensity, fully changed)
struct LightState
{
    bool on;

    bool changingIntensity;
    bool changingColour;

    vec3 position;

    float duration;
    float time;
    float factor;

    int prevColorIndex, colorIndex;

    float intensity;
    float specularPower;
    float radius;
    float cutoff;

    PointLight light;

    LightState(int index, bool on, float duration, int colorIndex, float intensity, float specularPower, float radius, float cutoff) : 
        on(on), 
        duration(duration), time(duration), factor(1),
        prevColorIndex(colorIndex), colorIndex(colorIndex), 
        intensity(intensity), specularPower(specularPower),
        radius(radius), cutoff(cutoff),
        light(index)
    {
    }

    // resets the light - inverts the time (so if time = duration, then time = 0)
    // this ensures that if we toggle light while its already changing, 
    // it starts from where it is, rather than quickly jump to inital state
    void reset()
    {
        time = duration - time;
        update(0);
    }

    // updates the light. advances its internal timer, so that we can properly calculate intensity/color change
    void update(float dt)
    {
        // we scale light's original properties with preset's values
        float lightIntensity = intensity * LightPresetArray[currentLightPreset].intensity;
        float lightSpecularPower = specularPower * LightPresetArray[currentLightPreset].specularPower;
        float lightRadius = radius * LightPresetArray[currentLightPreset].radius;
        float lightCutoff = cutoff * LightPresetArray[currentLightPreset].cutoff;

        float speed = on ? 1 : 4;

        time = fmin(fmax(time + dt * speed, 0), duration);
        factor = time / duration;
        
        float lightScale = on ? 1 : 0;
        vec3 lightColour;

        if (factor < 1)
        {
            if (changingIntensity)
                lightScale = on ? factor : (1 - factor);

            if (changingColour)
            {
                vec3 prevColour = LightColorArray[prevColorIndex];
                vec3 nextColour = LightColorArray[colorIndex];

                // lerp between prev colour and next colour
                lightColour = prevColour + (nextColour - prevColour) * factor;
            }
            else
            {
                lightColour = LightColorArray[colorIndex];
            }
        }
        else
        {
            changingIntensity = false;
            changingColour = false;

            // ensure that if we have reached duration, color does not change anymore (when toggling on/off)
            prevColorIndex = colorIndex;

            lightColour = LightColorArray[colorIndex];
        }

        // update light properties and apply it
        light.withAmbient(0, 0, 0)
            .withPosition(position)
            .withDiffuse(lightColour.x, lightColour.y, lightColour.z, lightIntensity * lightScale)
            .withSpecular(lightColour.x, lightColour.y, lightColour.z, lightSpecularPower * lightScale)
            .withRadius(lightScale * lightRadius)
            .withCutoff(lightCutoff)
            .enable(true)
            .apply();
    }
};

#define LAMP_LIGHTS 2
LightState lightState[LAMP_LIGHTS] = {
    LightState(0, true, 1.0f, 0, 1, 500, 1.3, 0.005f),
    LightState(1, true, 1.0f, 1, 1, 500, 1.3, 0.005f),
};

void toggleLight(int light)
{
    lightState[light].reset();
    lightState[light].changingIntensity = true;
    lightState[light].on = !lightState[light].on;
}

void toggleLightsColor(int offset)
{
    for (int i = 0; i < LAMP_LIGHTS; ++i)
    {
        lightState[i].reset();
        lightState[i].changingColour = true;

        int colorIndex = lightState[i].colorIndex + offset;
        
        // wrap the color index. 
        // if we go back, make sure we land on last, if we exceed first
        // if we go forward, make sure we land on first, if we exceed last
        colorIndex = 
            (colorIndex < 0) ? (LIGHT_COLORS + colorIndex) 
          : (colorIndex > LIGHT_COLORS) ? (colorIndex % LIGHT_COLORS) 
          : colorIndex;

        lightState[i].prevColorIndex = lightState[i].colorIndex;
        lightState[i].colorIndex = colorIndex;
    }
}

// updating lights (key actions)...
void decreaseLightsIntensity()
{
    for (int i = 0; i < LAMP_LIGHTS; ++i)
        lightState[i].intensity = fmax(lightState[i].intensity - 0.25, 0.25);
}

void increaseLightsIntensity()
{
    for (int i = 0; i < LAMP_LIGHTS; ++i)
        lightState[i].intensity = fmin(lightState[i].intensity + 0.25, 10);
}

void updateLights(float dt)
{
    for (int i = 0; i < LAMP_LIGHTS; ++i)
        lightState[i].update(dt);
}


float totalTime = 0;

void render()
{
	// this global variable controls the animation
	float theta = glutGet(GLUT_ELAPSED_TIME) * 0.2f;
	float beta = glutGet(GLUT_ELAPSED_TIME) * 0.015f;

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    

    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt;

    // calculate elapsed time. on first frame just use 1/60, cuz otherwise it jumps heavily
    if (totalTime == 0)
        dt = 0.016f;
    else dt = time - totalTime;
      
    totalTime = time;

    
	// setup the View Matrix (camera)
	mat4 m = rotate(mat4(1.f), radians(angleTilt), vec3(1.f, 0.f, 0.f));// switch tilt off
	m = translate(m, cam);												// animate camera motion (controlled by WASD keys)
	m = rotate(m, radians(-angleTilt), vec3(1.f, 0.f, 0.f));			// switch tilt on
	m = m * matrixView;
	//m = rotate(m, radians(angleRot), vec3(0.f, 1.f, 0.f));				// animate camera orbiting
	matrixView = m;
	Program.SendUniform("matrixView", matrixView);



    // apply directional light
    DirectionalLight()
        .enable(dirLightOn)
        .withAmbient(0, 0, 0)
        .withDiffuse(1, 1, 1, 0.2)
        .withDirection(0, 2, 1)
        .withSpecular(0.7, 0.7, 0.7, 800)
        .apply();



    // set lightbulb light positions
    lightState[0].position = vec3(-2.57f, 4.05f, 5.f);
    lightState[1].position = vec3(0.365f, 4.05f, 6.0f);

    // update lightbulb lights...
    updateLights(dt);


    /////////////////
    // MATERIALS
    Material lightbuld1Material =
        Material()
            .withShininess(10)
            .withAmbientColour(0, 0, 0)
            .withDiffuseColour(0, 0, 0)
            .withEmissiveColour(
                lightState[0].light.diffuse.x, lightState[0].light.diffuse.y, lightState[0].light.diffuse.z, 
                fmax(lightState[0].light.diffuseStrength, 0.1));
    
    Material lightbuld2Material =
        Material()
            .withShininess(10)
            .withAmbientColour(0, 0, 0)
            .withDiffuseColour(0, 0, 0)
            .withEmissiveColour(
                lightState[1].light.diffuse.x, lightState[1].light.diffuse.y, lightState[1].light.diffuse.z, 
                fmax(lightState[1].light.diffuseStrength, 0.1));
    
    Material chairMaterial =
        Material()
            .withShininess(10)
            .withDiffuseTexture(chairAlbedoTexId)
            .withNormalTexture(chairNormalTexId);
    
    Material lampMaterial =
        Material()
            .withShininess(1)
            .withAmbientColour(0.8f, 0.8f, 0.2f)
            .withDiffuseColour(0.4f, 0.4f, 0.2f);
    
    Material tableMaterial =
        Material()
            .withShininess(10)
            .withDiffuseTexture(tableAlbedoTexId)
            .withNormalTexture(tableNormalTexId);
    
    Material vaseMaterial =
        Material()
            .withShininess(50)
            .withAmbientColour(0.0f, 1.0f, 0.0f)
            .withDiffuseColour(0.2f, 0.2f, 0.6f);
    
    Material dinoMaterial =
        Material()
            .withShininess(50)
            .withAmbientColour(1.0f, 1.0f, 0.0f)
            .withDiffuseColour(0.2f, 0.2f, 0.6f);


    ////////////////
    // MODLELS
	//chair 1
    Model(table)
        .withPosition(0, 0, 5.5f)
        .withRotation(180.f)
        .withScale(0.004f)
        .withMesh(0)
        .draw(chairMaterial);

	//chair 2
    Model(table)
        .withPosition(0.5f, 0, 5.0f)
        .withRotation(90.f)
        .withScale(0.004f)
        .withMesh(0)
        .draw(chairMaterial);

	//chair 3
    Model(table)
        .withPosition(0, 0, 4.5f)
        .withRotation(0)
        .withScale(0.004f)
        .withMesh(0)
        .draw(chairMaterial);

	//chair 4
    Model(table)
        .withPosition(-0.5f, 0, 5.0f)
        .withRotation(270.f)
        .withScale(0.004f)
        .withMesh(0)
        .draw(chairMaterial);

	//table
    Model(table)
        .withPosition(0, 0, 5.0f)
        .withRotation(0)
        .withScale(0.004f)
        .withMesh(1)
        .draw(tableMaterial);

    
    //lamp 1
    Model(lamp)
        .withPosition(-2.0f, 3.045f, 4.0f)
        .withRotation(60)
        .withScale(0.025f)
        .draw(lampMaterial);
    
    Model(lightbulb)
        .withPosition(-2.57f, 4.05f, 5.f)
      //  .withRotation(155.0)
      //  .withRotationAxis(0, 0, 1)
        .withEuler(0, 60, 155)
        .withScale(0.25f)
        .draw(lightbuld1Material);
    

    //lamp 2
    Model(lamp)
        .withPosition(1.5f, 3.045f, 6.0f)
        .withRotation(0)
        .withScale(0.025f)
        .draw(lampMaterial);

    Model(lightbulb)
        .withPosition(0.365f, 4.05f, 6.0f)
        .withRotation(155.0)
        .withRotationAxis(0, 0, 1)
        .withScale(0.25f)
        .draw(lightbuld2Material);


	//vase
    Model(vase)
        .withPosition(0, 3, 5.0f)
        .withRotation(0)
        .withScale(0.1f)
        .draw(vaseMaterial);
    
	//dino
    Model(dino)
        .withPosition(-0.5f, 3.735f, 4.0f)
        .withRotation(theta)
        .withScale(0.005f)
        .draw(dinoMaterial);


    // cannot update the followin cuz they dont use the model class.
    // just keep them as they were
	//teapot
	// setup materials - blue
	Program.SendUniform("material.ambient", 0.2f, 0.2f, 0.8f);
	Program.SendUniform("material.diffuse", 0.2, 0.2, 0.6);

	m = matrixView;
	m = translate(m, vec3(1.2f, 3.3, 5.15f));
	m = rotate(m, radians(-theta), vec3(0.0f, 1.0f, 0.0f));
	Program.SendUniform("matrixModelView", m);
	glutSolidTeapot(0.4);

    // pyramid
	Program.SendUniform("material.ambient", 1.0f, 0.0f, 0.0f);
	Program.SendUniform("material.diffuse", 0.2, 0.2, 0.6);
	
	m = matrixView;
	m = translate(m, vec3(-0.5f, 3.735, 4.0f));
	m = rotate(m, radians(theta), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(-0.1f, -0.1f, -0.1f));
	Program.SendUniform("matrixModelView", m);

	// Get Attribute Locations
	GLuint attribVertex = Program.GetAttribLocation("aVertex");
	GLuint attribNormal = Program.GetAttribLocation("aNormal");

	// Enable vertex attribute arrays
	glEnableVertexAttribArray(attribVertex);
	// glEnableVertexAttribArray(attribTexCorrds);
	glEnableVertexAttribArray(attribNormal);

	// Bind (activate) the vertex buffer and set the pointer to it
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(attribVertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
   
	// Bind (activate) the normal buffer and set the pointer to it
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Draw triangles – using index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

	// Disable arrays
	glDisableVertexAttribArray(attribVertex);
	glDisableVertexAttribArray(attribNormal);


	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void reshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	mat4 matrixProjection = perspective(radians(60.f), ratio, 0.02f, 1000.f);
	
	// Setup the Projection Matrix
	Program.SendUniform("matrixProjection", matrixProjection);

}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': cam.z = std::max(cam.z * 1.05f, 0.01f); break;
	case 's': cam.z = std::min(cam.z * 1.05f, -0.01f); break;
	case 'a': cam.x = std::max(cam.x * 1.05f, 0.01f); angleRot = 0.1f; break;
	case 'd': cam.x = std::min(cam.x * 1.05f, -0.01f); angleRot = -0.1f; break;
	case 'e': cam.y = std::max(cam.y * 1.05f, 0.01f); break;
	case 'q': cam.y = std::min(cam.y * 1.05f, -0.01f); break;
    
    case '1': toggleLight(0); break;
    case '2': toggleLight(1); break;

    case ',': toggleLightsColor(-1); break;
    case '.': toggleLightsColor(+1); break;

    case '-': decreaseLightsIntensity(); break;
    case '=': increaseLightsIntensity(); break;

    case 'p': currentLightPreset = (currentLightPreset+1) % LIGHT_PRESETS; break;
    case 'o': dirLightOn = !dirLightOn; break;
	}
	// speed limit
	cam.x = std::max(-0.15f, std::min(0.15f, cam.x));
	cam.y = std::max(-0.15f, std::min(0.15f, cam.y));
	cam.z = std::max(-0.15f, std::min(0.15f, cam.z));
	// stop orbiting
	if ((glutGetModifiers() & GLUT_ACTIVE_SHIFT) == 0) angleRot = 0;
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': cam.z = 0; break;
	case 'a':
	case 'd': cam.x = 0; break;
	case 'q':
	case 'e': cam.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
bool bJustClicked = false;
void onMouse(int button, int state, int x, int y)
{
	bJustClicked = (state == GLUT_DOWN);
	glutSetCursor(bJustClicked ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
}

// handle mouse move
void onMotion(int x, int y)
{
	if (bJustClicked)
		bJustClicked = false;
	else
	{
		glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

		// find delta (change to) pan & tilt
		float deltaPan = 0.25f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
		float deltaTilt = 0.25f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

		// View = Tilt * DeltaPan * Tilt^-1 * DeltaTilt * View;
		angleTilt += deltaTilt;
		mat4 m = mat4(1.f);
		m = rotate(m, radians(angleTilt), vec3(1.f, 0.f, 0.f));
		m = rotate(m, radians(deltaPan), vec3(0.f, 1.f, 0.f));
		m = rotate(m, radians(-angleTilt), vec3(1.f, 0.f, 0.f));
		m = rotate(m, radians(deltaTilt), vec3(1.f, 0.f, 0.f));
		matrixView = m * matrixView;
	}
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(800, 600);
	glutCreateWindow("CI5520 3D Graphics Programming");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cerr << "GLEW Error: " << glewGetErrorString(err) << endl;
		return 0;
	}
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;

	// register callbacks
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);

	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version: " << glGetString(GL_VERSION) << endl;

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		cerr << "Application failed to initialise" << endl;
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	done();

	return 1;
}

