// Space Filling Curve:
// 1. Start with a spherical mesh.
// - Apply golden ratio to evenly distribute points on the sphere's surface.
// - Draw lines based on three nearest neighbors for each point to create the mesh.

// 2. Apply dual of simplest subdivision. Label these new edges "new."
// - Find four nearest neighbors to each vertex.
//
// 3. Reinsert old edges to create triangles. Label these old edges "old."
//
// 4. Find curve control vertices at center of each triangle through averaging of vertices.
//
// 5. Connect center points of triangles along the "new" edges to begin creating the curve mesh.
//
// 6. If an old edge is between two separate curves, flip it to instead connect the other two vertices of the new edge it lies between, and recalculate the curve.

// AlloLib Libraries:
#include "al/app/al_App.hpp" // App library.
#include "al/graphics/al_Shapes.hpp" // Shapes library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

#define PI 3.14159265
#define TWO_PI 3.14159265 * 2

const int numVertices = 500;
Vec3f sphereVertices[numVertices];

struct MyApp : public App {
  Mesh oldMesh, newMesh, curveMesh; // The original mesh, the subdivided mesh, and the final curve to be displayed.

  void onCreate() override {
    oldMesh.primitive(Mesh::LINES);
    nav().pos(0, 0, 0);

    // 1. Create a Spherical Mesh:
    // Apply golden ratio to evenly distribute points on the sphere's surface:
    for(int i = 0; i < numVertices; i++){
      float theta = acos(1 - 2 * float(i) / numVertices); // Calculate angles based on point number to be equally distributed along sphere's surface.
      float phi = PI * (1 + sqrt(5)) * float(i); // The golden ratio.
      sphereVertices[i] = Vec3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)); // Calculate the point on the sphere's surface.
    }

    // Draw lines based on three nearest neighbors for each point to create the mesh:
    for (int i = 0; i < numVertices; i++){
      oldMesh.vertex(sphereVertices[i]);
      oldMesh.color(HSV(float(i) / numVertices, 1.0, 1.0));
      float dist = 0.0; // Initialize the distance.
      float nearest1 = 5.0; // Initialize the nearest neighbor.
      Vec3f p1, p2, p3;
      for (int j = i + 1; j < numVertices; j++){
        dist = (sphereVertices[i] - sphereVertices[j]).mag();
        if (dist < nearest1) {
          nearest1 = dist;
          p1 = sphereVertices[j];
        }
      }
      oldMesh.vertex(p1);
    }

    // 2. Apply simplest subdivision to obtain new mesh:
    // Find four nearest neighbors to each vertex:
    int numVertices = oldMesh.vertices().size();
  }

  void onAnimate(double dt) override{
  }

  // Draw Graphics to Screen:
  void onDraw(Graphics& g) override {
    static Light light; // Create a light named light.
    g.clear(0); // Clear the graphics buffer.
   
    // Establish lighting for scene:
    light.pos(0, 0, 0);
    gl::depthTesting(true);
    g.lighting(true);
    g.light(light);
    light.ambient(HSV(1.0, 0.0, 1.0));

    // Draw mesh to scene:
    g.polygonMode(true ? GL_LINE : GL_FILL); // Make the mesh wireframe.
    g.meshColor();
    g.draw(oldMesh);
  }
};

// Function which runs the app:
int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}