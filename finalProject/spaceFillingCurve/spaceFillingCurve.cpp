// Space Filling Curve:
// 1. Create a spherical mesh.
// 1A. Distribute vertices evenly on edge of sphere.
// 1B. Generate triangle list. 
// 1C. Generate indices.

// 2. Apply dual of simplest subdivision. 
// 2A. Find edge midpoints.
// 2B. Determine edge connections between vertices to find faces.
// 2C. Connect every edge midpoint to four midpoints which share both a vertex and a face with the original edge.
//
// 3. Reinsert old edges into submesh.
//
// 4. Find centroid of faces for each triangle in submesh to be curve control point.
//
// 5. Connect center points of triangles along the "new" edges to begin creating the curve mesh.
//
// 6. If an old edge is between two separate curves, flip it to instead connect the other two vertices of the new edge it lies between, and recalculate the curve.

// AlloLib Libraries:
#include "al/app/al_App.hpp" // App library.
#include "al/graphics/al_Shapes.hpp" // Shapes library.
#include "al/graphics/al_Isosurface.hpp" // Isosurface library.
#include <queue> // Priority queue library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

int numVertices = 100;

#define PI 3.14159265

struct tri {
  Vec3f v0, v1, v2;
};

struct MyApp : public App {
  Mesh oldMesh, subVertices; // The original mesh, the subdivided mesh, and the final curve to be displayed.
  
  void onCreate() override {
    nav().pos(0, 0, 0); // Set camera at center of scene.

    // 1. Create a Spherical Mesh:
    oldMesh.primitive(Mesh::POINTS);

    // 1A. Distribute vertices on surface of sphere:
    for(int i = 0; i < numVertices; i++){
      float theta = acos(1 - 2 * float(i) / numVertices); // Calculate angles based on point number to be equally distributed along sphere's surface.
      float phi = PI * (1 + sqrt(5)) * float(i); // The golden ratio.
      oldMesh.vertex(Vec3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta))); // Calculate the point on the sphere's surface.
      oldMesh.color(HSV(1.0, 0.0, 1.0)); // Color each vertex white.
    }

    // 1B. Generate triangle list:
    int numSlices = int(sqrt(oldMesh.vertices().size()));
    int numTri = 2 * (numSlices - 1) * (numSlices - 1);
    tri oldTri[numTri];
    for (int i = 0; i < numSlices - 1; i++) {
      for (int j = 0; j < numSlices - 1; j++){
            // For each vertex of our sphere, generate a quad based on neighboring vertices.
            Vec3f v0 = oldMesh.vertices()[i * numSlices + j]; // The vertex being calculated from.
            Vec3f v1 = oldMesh.vertices()[i * numSlices + j + 1]; // The next vertex in the same row.
            Vec3f v2 = oldMesh.vertices()[(i + 1) * numSlices + j]; // The vertex directly below it.
            Vec3f v3 = oldMesh.vertices()[(i + 1) * numSlices + j + 1]; // The next vertex in the row below it.
            // Number the triangles formed from splitting the quad in half.
            int triIndex1 = (i * (numSlices - 1) + j) * 2;
            int triIndex2 = ((i * (numSlices - 1) + j) * 2) + 1;
            // Store the vertices for each triangle formed from splitting the quad in half.
            oldTri[triIndex1].v0 = v0;
            oldTri[triIndex1].v1 = v1;
            oldTri[triIndex1].v2 = v2;
            oldTri[triIndex2].v0 = v1;
            oldTri[triIndex2].v1 = v2;
            oldTri[triIndex2].v2 = v3;
      }
    }


    // 2. Apply Dual of Simplest Subdivision:
    // subVertices.primitive(Mesh::POINTS); // Call subVertices mesh as a primitive displaying points.

    // 2A. Find edge midpoints:
    // oldMesh.forEachFace([&](int v1, int v2, int v3){
      // Get vertices for each face:
      // Vec3f vertex1 = oldMesh.vertices()[v1];
      // Vec3f vertex2 = oldMesh.vertices()[v2];
      // Vec3f vertex3 = oldMesh.vertices()[v3];

      // Find midpoint between each by averaging every possible combination of vertices:
      // Vec3f midPoint1 = (vertex1 + vertex2) / 2;
      // Vec3f midPoint2 = (vertex2 + vertex3) / 2;
      // Vec3f midPoint3 = (vertex3 + vertex1) / 2;

      // Assign midpoints to the subVertices mesh to be visualizes as points.
    //   subVertices.vertex(midPoint1);
    //   subVertices.color(HSV(1.0, 0.0, 1.0));
    //   subVertices.vertex(midPoint2);
    //   subVertices.color(HSV(1.0, 0.0, 1.0));
    //   subVertices.vertex(midPoint3);
    //   subVertices.color(HSV(1.0, 0.0, 1.0));
    // });

    // for (int i = 0; i < subVertices.vertices().size(); i++){
    //   cout << subVertices.vertices()[i] << endl;
    // }
  }

  void onAnimate(double dt) override{
  }

  // Draw Graphics to Screen:
  void onDraw(Graphics& g) override {
    static Light light; // Create a light named light.
    g.clear(0); // Clear the graphics buffer.
   
    // Establish lighting for scene:
    // light.pos(0, 0, 0);
    // gl::depthTesting(true);
    // g.lighting(true);
    // g.light(light);
    // light.ambient(HSV(1.0, 0.0, 1.0));

    // Draw mesh to scene:
    g.polygonMode(true ? GL_LINE : GL_FILL); // Make the mesh wireframe.
    g.meshColor();
    g.pointSize(5.0);
    g.lineWidth(10.0);
    g.draw(oldMesh);
  }
};

// Function which runs the app:
int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

//
// CODE FROM OLD ATTEMPTS THAT COULD BE USEFUL LATER ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ~~~~~~~~~ Distribute Vertices on Sphere's Surface:
//// 1. Create a Spherical Mesh:
//// Apply golden ratio to evenly distribute points on the sphere's surface:
//    for(int i = 0; i < numVertices; i++){
//      float theta = acos(1 - 2 * float(i) / numVertices); // Calculate angles based on point number to be equally distributed along sphere's surface.
//      float phi = PI * (1 + sqrt(5)) * float(i); // The golden ratio.
//      sphereVertices[i] = Vec3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)); // Calculate the point on the sphere's surface.
//    }
// ~~~~~~~~~ Nearest Neighbor Structure:
//
// struct nn {
//   Vec3f n1, n2, n3, n4;
// };
//
// ~~~~~~~~~ Custom Comparator for Priority Queue:
//
// struct CompareDist {
//     bool operator()(pair<Vec3f, float> const& p1, pair<Vec3f, float> const& p2) { 
//         return p1.second > p2.second; // Returns a boolean (true / false) based on distance from p1 being greater than than distance from p2.
//     }
// };
//
// ~~~~~~~~~ Nearest Neighbors using Priority Queue:
// priority_queue<pair<Vec3f, float>, vector<pair<Vec3f, float>>, CompareDist> pq; // Create a priority queue that holds pairs of Vec3f vertices and the float distance between them, ordered by the custom comparator.
// nn nearestFP[facePoints.vertices().size()]; // Array of nearest neighbors for each facepoint.
// // Find nearest neighbors:
// for (int i = 0; i < facePoints.vertices().size(); i++) {
//   while (!pq.empty()) { // If the priority queue is not empty...
//     pq.pop(); // Pop the top element until it's empty.
//   }
//   for (int j = 0; j < facePoints.vertices().size(); j++){
//     if (i != j){ // Do not calculate for self.
//       float dist = (facePoints.vertices()[i] - facePoints.vertices()[j]).mag(); // Calculate the distance between two vertices.
//       pq.push(make_pair(facePoints.vertices()[j], dist)); // Push a pair of vertices and their distance to the priority queue.
//       }
//     }
//   // Assign the nearest vertices in priority queue to the nearestNeighbors array:
//   nearestFP[i].n1 = pq.top().first; // Assign the nearest vertex to the nearestNeighbors array.
//   pq.pop(); // Pop the top element from the priority queue so now the second nearest vertex is at the top.
//   nearestFP[i].n2 = pq.top().first; // Assign the second nearest vertex to n2, and so on...
//   pq.pop();
// }