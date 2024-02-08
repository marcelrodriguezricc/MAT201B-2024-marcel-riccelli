// Call libraries:
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Complex.hpp"
#include "al/math/al_Vec.hpp"

// Determine namespaces:
using namespace al;
using namespace std;
#include <fstream>
#include <vector>

// A function which generates a random Vec3:
Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

// I don't know what this means:
string slurp(string fileName); 

// AlloApp Constructor:
struct AlloApp : App {

  // Variable Parameters:
  Parameter pointSize{"/pointSize", "", 1.5, 0.05, 2.0};
  Parameter timeStep{"/timeStep", "", 0.2, 0.01, 0.6};
  Parameter drag{"/drag", "", 1.5, 0.01, 15.0};
  Parameter spring{"/spring", "", 1.5, 0.01, 15.0};
  Parameter coulombs{"/coulombs", "", 0.016, 0.001, 1.0};

  // Calling the shader program:
  ShaderProgram pointShader;

  // Calling the mesh:
  Mesh mesh;  

  // Declaring our variables:
  vector<Vec3f> velocity;
  vector<Vec3f> acceleration;
  vector<float> mass;
  const int numParticles = 1000;
  float charge[1000];
  HSV colorSelector[1000];

  void onInit() override {
    // Set up the GUI with our variable parameters:
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize); 
    gui.add(timeStep);
    gui.add(drag);
    gui.add(spring);
    gui.add(coulombs);
  }

  // Set initial conditions of the simulation:
  void onCreate() override {
    // Compile Shaders:
    pointShader.compile(slurp("../point-vertex.glsl"), slurp("../point-fragment.glsl"), slurp("../point-geometry.glsl"));

    // A variable which generates a random color:
    auto randomColor = []() { return HSV(rnd::uniform(), 1.0f, 1.0f); };

    // Assign a primitive type of Points to our mesh:
    mesh.primitive(Mesh::POINTS);
    

    // The foor loop which creates 1000 random verticies of random color:
    for (int i = 0; i < numParticles; i++) {

      // Generate the random verticies of random colors:
      mesh.vertex(randomVec3f(5));
      colorSelector[i] = randomColor();
      mesh.color(colorSelector[i]);

      charge[i] = abs(rnd::normal());

      // What's going on here?
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5) m = 0.5;
      mass.push_back(m);

      // Using a simplified volume/size relationship:
      mesh.texCoord(pow(m, 1.0f / 3), 0);  // s, t

      // Separate state arrays:
      velocity.push_back(randomVec3f(0.1));
      acceleration.push_back(randomVec3f(1));
    }

    // Camera positioning:
    nav().pos(0, 0, 25);
  }

  // What does this mean?
  bool freeze = false;

  // Animation loop:
  void onAnimate(double dt) override {
    if (freeze) return;

    // Time step variable:
    dt = timeStep;
    vector<Vec3f> &position(mesh.vertices()); // Create an array of vec3s named position, and fill it with the position of the mesh verticies.

    // Spring and Damp Force:
    for (int i = 0; i < velocity.size(); i++) {
      Vec3f pos = mesh.vertices()[i];
      Vec3f rest = mesh.vertices()[i];
      Vec3f dampForce = velocity[i] * drag; // Dampen accelleration.
      Vec3f springForce = (rest.normalize() - pos) * spring;
      acceleration[i] += springForce - dampForce;  // Hookes.
    }

    // Coulombs Force:
    for (int j = 0; j < velocity.size(); j++) {
        for (int k = j + 1; k < velocity.size(); k++) {
          float magnitude = dist(position[j], position[k]);
          Vec3f coulombsForce = position[k] - position[j];
          coulombsForce.normalize();
          coulombsForce *= coulombs * (charge[j] * charge[k] / pow(magnitude, 2));
          acceleration[k] += coulombsForce;
          acceleration[j] -= coulombsForce;
        }
        
    }
    
    // Accumulate velocity and position with Semi-Implicit Euler:
    for (int i = 0; i < velocity.size(); i++) { // For each member of the velocity array...
      velocity[i] += acceleration[i] / mass[i] * dt;
      position[i] += velocity[i] * dt;
    }

    // Clear all accelerations (imporant):
    for (auto &a : acceleration) a.set(0);
  }
  

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      freeze = !freeze;
    }

    if (k.key() == '1') {
      for (int i = 0; i < velocity.size(); i++) {
        acceleration[i] = randomVec3f(1) / mass[i] * 100.0; // Apply a random force.
      }
    }

    if (k.key() == '2') {
      for (int i = 0; i < velocity.size(); i++) {
        charge[i] = colorSelector[i].h; // Change the charge of each particle according to hue.
      }
    }

    if (k.key() == '3') {
      for (int i = 0; i < velocity.size(); i++) {
        charge[i] = abs(rnd::normal()); // Change charge back to a random floating point number between 0.0 and 1.0.
      }
    }
    return true;
 }

  // Compiling the shader which draws our simulation to the screen:
  void onDraw(Graphics &g) override {
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);
  }
};

// Run the app:
int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

// I don't know what this is:
string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
