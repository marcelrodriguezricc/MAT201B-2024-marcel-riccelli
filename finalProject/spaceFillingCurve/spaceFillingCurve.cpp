// Space Filling Curve:
// 1. Start with a spherical mesh.
// - Apply golden ratio to evenly distribute points on the sphere's surface.
//
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
#include <queue> // Priority queue library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

#define PI 3.14159265
#define TWO_PI 3.14159265 * 2

// Data structure to store nearest neighbors:
struct nn {
  Vec3f n1, n2, n3, n4;
};

// Custom comparator for the priority queue:
struct CompareDist {
    bool operator()(pair<Vec3f, float> const& p1, pair<Vec3f, float> const& p2) { 
        return p1.second > p2.second; // Returns a boolean (true / false) based on distance from p1 being greater than than distance from p2.
    }
};

struct MyApp : public App {
  Mesh ogMesh, facePoints, edgePoints; // The original mesh, the subdivided mesh, and the final curve to be displayed.
  priority_queue<pair<Vec3f, float>, vector<pair<Vec3f, float>>, CompareDist> pq; // Create a priority queue that holds pairs of Vec3f vertices and the float distance between them, ordered by the custom comparator.
  Vec3f vertex1, vertex2, vertex3, centroid;

  void onCreate() override {
    nav().pos(0, 0, 0);

    // 1. Create a Spherical Mesh:

    addIcosphere(ogMesh, 1, 2);
    for (int i = 0; i < ogMesh.vertices().size(); i++){
      ogMesh.color(HSV(1.0, 1.0, 1.0));
    }

    facePoints.primitive(Mesh::POINTS);
    ogMesh.forEachFace([&](int v1, int v2, int v3) {
      vertex1 = ogMesh.vertices()[v1];
      vertex2 = ogMesh.vertices()[v2];
      vertex3 = ogMesh.vertices()[v3];
      centroid = (vertex1 + vertex2 + vertex3) / 3.0f;
      facePoints.color(HSV(1.0, 0.0, 1.0));
      facePoints.vertex(centroid);
    });

    nn nearestFP[facePoints.vertices().size()]; // Array of nearest neighbors for each facepoint.

    // Find nearest neighbors:
    for (int i = 0; i < facePoints.vertices().size(); i++) {
      while (!pq.empty()) { // If the priority queue is not empty...
        pq.pop(); // Pop the top element until it's empty.
      }
      for (int j = 0; j < facePoints.vertices().size(); j++){
        if (i != j){ // Do not calculate for self.
          float dist = (facePoints.vertices()[i] - facePoints.vertices()[j]).mag(); // Calculate the distance between two vertices.
          pq.push(make_pair(facePoints.vertices()[j], dist)); // Push a pair of vertices and their distance to the priority queue.
          }
        }
      // Assign the nearest vertices in priority queue to the nearestNeighbors array:
      nearestFP[i].n1 = pq.top().first; // Assign the nearest vertex to the nearestNeighbors array.
      pq.pop(); // Pop the top element from the priority queue so now the second nearest vertex is at the top.
      nearestFP[i].n2 = pq.top().first; // Assign the second nearest vertex to n2, and so on...
      pq.pop();
    }

    // Set edge points to the average of two neighboring face points:
    edgePoints.primitive(Mesh::POINTS);
    for (int i = 0; i < facePoints.vertices().size(); i++) {
      edgePoints.color(HSV(0.3, 1.0, 1.0));
      Vec3f avg = (facePoints.vertices()[i] + nearestFP[i].n1) / 2.0f;
      edgePoints.vertex(avg);
    }
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
    g.pointSize(5.0);
    g.lineWidth(5.0);
    g.draw(ogMesh);
    g.draw(facePoints);
    g.draw(edgePoints);
  }
};

// Function which runs the app:
int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}