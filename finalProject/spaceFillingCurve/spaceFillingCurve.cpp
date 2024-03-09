// TO DO:
// 1. Create a spherical triangle strip mesh.
// 2. Aggregate the vertices which form each triangular face.
// 3. Find average of vertices to get center of mesh.
// 4. Draw initial curve from center to center.


// AlloLib Libraries:
#include "al/app/al_App.hpp" // App library.
#include "al/graphics/al_Shapes.hpp" // Shapes library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

#define PI 3.14159265
#define TWO_PI 3.14159265 * 2

float rate = 0.1;
float radius = 5.0;
const int numTravelers = 6;
float timer, timer2 = 0.0;

Vec3f p1, p2, p3;

struct MyApp : public App {
  Mesh normalMesh, sphereMesh, curveMesh;

  void onCreate() override {
    normalMesh.primitive(Mesh::LINES);
    nav().pos(0, 0, 0);
    addIcosphere(sphereMesh, 1, 1);
    int numVertices = sphereMesh.vertices().size();
    for (int i = 0; i < numVertices; i++) {
      sphereMesh.color(HSV(float(i) / numVertices, 0.3, 1));
    }
    sphereMesh.generateNormals();
    sphereMesh.createNormalsMesh(normalMesh, 0.1, true);
    auto normalVert = normalMesh.vertices();
    for (int i = 0; i < normalVert.size(); i++) {
      if (i % 2 == 0) {
        curveMesh.vertex(normalVert[i]);
        cout << i << endl;
      }
    }
  };

  void onAnimate(double dt) override{
        // timer += rate;
        // timer2 += rate * 1.125;
        //mesh.vertex(x, y, z);
        // x = radius * cos(timer2) * sin(timer);
        // y = radius * sin(timer2) * sin(timer);
        // z = radius * cos(timer);
        // mesh.vertex(x, y, z);
  }

  // Draw Graphics to Screen:
  void onDraw(Graphics& g) override {
    static Light light;
    g.clear(0); // Clear the graphics buffer.
    g.polygonMode(true ? GL_LINE : GL_FILL); // Make the mesh wireframe.
    //g.draw(normalMesh); // Draw the mesh.
    //g.draw(curveMesh); // Draw the mesh.
    light.pos(0, 0, 0);
    gl::depthTesting(true);
    g.lighting(true);
    g.light(light);
    light.ambient(HSV(1.0, 0.0, 1.0));
    g.meshColor();
    g.draw(sphereMesh);
    
  }
};

// Function which runs the app:
int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}