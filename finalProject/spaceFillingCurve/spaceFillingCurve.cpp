// Space Filling Curve:
//
// TASKS:
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 1. Create a mesh.
// 1A. Generate the vertices.
// 1B. Assign an index to each vertex.
//
// 2. Apply dual of simplest subdivision. 
// 2A. Find edge midpoints of original mesh.
// 2B. Assign them to be the vertices of a new mesh.
// 2C. Assign an index to each vertex.
//
// 3. Find centroid of faces for each triangle in newly subdivided mesh to be curve control point.
// 3A. Average the vertices of of both the original and new mesh to find each centroid
// 
// 4. Generate the initial curve.
// - Initial curve should visit every centroid in each face (hamiltonian cycle).
//
// 5. Connect the curves through from each face to the next face to form a single curve.
// - The curve should not intersect itself when crossing to the next face.
// 
// 6. Use the curve to generate the path for a constant diameter ribbon. Animate the ribbon being drawn over time.
// 6A. Calculate tangent vector.
// 6B. Calculate normal vector.
// 6C. Calculate binormal vector.
// - Is there a way to find the vertex based on index number?
//
// NOTES:
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 1. Do we want to segment the initial mesh into slices to give paths for multiple curves at once?
// 2. Generating a quad mesh generates a more aesthetically pleasing curve.
// - A cube subdivided several times creates a quad sphere.
// 3. How can we randomize the hamiltonian cycle for greater variation and visual interest?

// AlloLib Libraries:
#include "al/app/al_DistributedApp.hpp" // For distributing tasks across multiple machines in AlloSphere.
#include "al/app/al_App.hpp" // App library.
#include "al/graphics/al_Shapes.hpp" // Shapes library.
#include "al/graphics/al_Isosurface.hpp" // Isosurface library.
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp" // For distributing state across multiple machines in AlloSphere.
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"
#include <queue> // Priority queue library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

// Global Variables:
int numVertices = 100; // Number of vertices to be distributed on the sphere's surface.
float rate = 0.05; // Rate of animation.
float timer; // Timer for animation.
int travel = 0; // Travel index for animation.

#define PI 3.14159265

// State structure for the distributed app.
struct State {
  Pose pose; // The pose of the camera.
};

// A structure to contain relevant information for each face of the original mesh:
struct face {
  int numFace; // The index number of the face on the original mesh.
  Vec3f v1, v2, v3; // The vertices which form the face from the original mesh.
  Vec3f mp1, mp2, mp3; // The subdivision midpoints.
  Vec3f c1, c2, c3, c4; // The curve control centroids.
};

// Main App Class:
struct RayApp : public DistributedAppWithState<State> {
  Mesh oldMesh, oldVertices, newMesh, newVertices, combinedMesh, combinedVertices, curveControl, curve1, curve2, ribbon; // The original mesh, the subdivided mesh, and the final curve to be displayed.
  
  void onCreate() override {
    auto cuttleboneDomain =
    CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    nav().pos(0, 0, 0); // Set camera at center of scene.

    // 1. Create a mesh:
    addSphere(oldMesh, 1, 32, 32); // Create a sphere mesh with 16 slices and 16 stacks.
    for (int i = 0; i < oldMesh.vertices().size(); i++){
      oldMesh.color(HSV(1.0, 0.0, 1.0)); // Set the color of the mesh to be white.
    }

    // Create a mesh for the vertices of the original mesh for problem solving:
    oldVertices.primitive(Mesh::POINTS);
    for (int i = 0; i < oldMesh.vertices().size(); i++){
      oldVertices.vertex(oldMesh.vertices()[i]); // Copy the vertices of the original mesh.
      oldVertices.color(HSV(1.0, 0.0, 1.0)); // Set the color of the vertices to be white.
    }

    // 2. Apply simplest subdivision:
    newMesh.primitive(Mesh::TRIANGLES); // Set the primitive type of the new mesh to have triangles as faces.

    // Find number of faces:
    int numFaces = 0;
    oldMesh.forEachFace([&](int v1, int v2, int v3){
      numFaces++; // Count all the faces to determine array size to contain n amount of the face data structure.
    });

    // Find edge midpoints and store all original vertices, midpoints, and face number, in the face data structure:
    face faces[numFaces]; // Create an array for the face data structure.
    int numFace = 0; // Initialize the face counter.
    oldMesh.forEachFace([&](int v1, int v2, int v3){
      faces[numFace].numFace = numFace; // Store the face number.
      // Store the vertices which make up each face:
      faces[numFace].v1 = oldMesh.vertices()[v1];
      faces[numFace].v2 = oldMesh.vertices()[v2];
      faces[numFace].v3 = oldMesh.vertices()[v3];
      // Calculate the midpoints of each vertex and store them:
      faces[numFace].mp1 = (faces[numFace].v1 + faces[numFace].v2) / 2.0f;
      faces[numFace].mp2 = (faces[numFace].v2 + faces[numFace].v3) / 2.0f;
      faces[numFace].mp3 = (faces[numFace].v3 + faces[numFace].v1) / 2.0f;
      // Assign the midpoint vertices to the new mesh:
      newMesh.vertex(faces[numFace].mp1);
      newMesh.vertex(faces[numFace].mp2);
      newMesh.vertex(faces[numFace].mp3);
      // Assign the color to the new mesh:
      newMesh.color(HSV(1.0, 0.0, 0.5));
      newMesh.color(HSV(1.0, 0.0, 0.5));
      newMesh.color(HSV(1.0, 0.0, 0.5));
      // Increment the face counter:
      numFace++;
    });

    // Create a mesh for the vertices of the new mesh for problem solving:
    newVertices.primitive(Mesh::POINTS);
    for (int i = 0; i < newMesh.vertices().size(); i++){
      newVertices.vertex(newMesh.vertices()[i]); // Copy the vertices of the new mesh.
      newVertices.color(HSV(1.0, 0.0, 0.5)); // Set the color of the vertices to be gray.
    }

    // 3. Find centroid for each newly formed triangle to be curve control point:
    curveControl.primitive(Mesh::POINTS);
    numFace = 0; // Reset the face counter.
    oldMesh.forEachFace([&](int v1, int v2, int v3){
      HSV curveColor = HSV(float(numFace) / float(numFaces), 1.0, 1.0); // Create a color for the curve control points based on the face number.
      // Calculate the centroids of each face:
      faces[numFace].c1 = (faces[numFace].v1 + faces[numFace].mp1 + faces[numFace].mp3) / 3.0f;
      faces[numFace].c2 = (faces[numFace].v2 + faces[numFace].mp1 + faces[numFace].mp2) / 3.0f;
      faces[numFace].c3 = (faces[numFace].v3 + faces[numFace].mp2 + faces[numFace].mp3) / 3.0f;
      faces[numFace].c4 = (faces[numFace].mp1 + faces[numFace].mp2 + faces[numFace].mp3) / 3.0f;
      // Assign the centroids to be vertices of the curve control mesh:
      curveControl.vertex(faces[numFace].c1);
      curveControl.vertex(faces[numFace].c2);
      curveControl.vertex(faces[numFace].c3);
      curveControl.vertex(faces[numFace].c4);
      // Assign the generated color to the curve control mesh:
      curveControl.color(curveColor);
      curveControl.color(curveColor);
      curveControl.color(curveColor);
      curveControl.color(curveColor);
      // Increment the face counter:
      numFace++;
    });

    // 4. Create initial curves:
    curve1.primitive(Mesh::LINES);
    numFace = 0; // Reset the face counter.
    oldMesh.forEachFace([&](int v1, int v2, int v3){
      HSV curveColor = HSV(float(numFace) / float(numFaces), 1.0, 1.0);
      // Assign the curve control points to be the vertices of the initial curve:
      curve1.vertex(faces[numFace].c1);
      curve1.vertex(faces[numFace].c2);
      curve1.vertex(faces[numFace].c3);
      curve1.vertex(faces[numFace].c4);
      // Set the color of the initial curve:
      curve1.color(curveColor);
      curve1.color(curveColor);
      curve1.color(curveColor);
      curve1.color(curveColor);
      // Index the vertices to determine the path of the curve:
      curve1.index(numFace * 4 + 1);
      curve1.index(numFace * 4);
      curve1.index(numFace * 4);
      curve1.index(numFace * 4 + 3);
      curve1.index(numFace * 4 + 3);
      curve1.index(numFace * 4 + 2);
      curve1.index(numFace * 4 + 2);
      // 5. Connect from face to face to form a single curve:
      if(numFace != numFaces - 1) { // Exclude the final connection as there is no next face...
        curve1.index((numFace + 1) * 4 + 1); // Connect to control points sequentially in a loop.
      }
      // Increment the face counter:
      numFace++;
    });
    
    // 6. Use the curve to generate the path for a constant diameter ribbon:
    // 6A. Calculate tangent vector
    curve2.primitive(Mesh::LINES);
    for (int i = 0; i < curve1.vertices().size(); i++){
      Vec3f tangent = (curve1.vertices()[i + 1] - curve1.vertices()[i]).normalize(); // Calculate the tangent vector.
      Vec3f normal = (curve1.vertices()[i] - Vec3f(0, 0, 0)).normalize(); // Set the normal vector to be the y-axis.
      Vec3f binormal = cross(tangent, normal); // Calculate the binormal vector.
      Vec3f v = curve1.vertices()[i] + binormal * 0.005; // Calculate the position of the ribbon.
      curve2.vertex(v); // Assign the position to the ribbon.
      curve2.color(HSV(1.0, 0.0, 1.0)); // Set the color of the ribbon.
      travel++; // Increment the travel index.
    }
    

    ribbon.primitive(Mesh::TRIANGLE_STRIP); // Set the primitive type of the curve to be lines.
    travel = 0;
  }

  // 7. Animate the curve being drawn over time:
  void onAnimate(double dt) override{
    if(isPrimary()){ // If the app is the primary instance...
      state().pose.set(nav()); // Set the state's pose to the camera's pose.
    } else { // If the app is not the primary instance...
      nav().set(state().pose); // Set the camera's pose to the state's pose.
    }
    // timer += rate; // Increment the timer by the rate.
    // if(timer < 1.0f){
    //   Vec3f curve1Pos = lerp(curve1.vertices()[travel], curve1.vertices()[travel + 1], timer); // Linearly interpolate between the vertices of the initial curve over time.
    //   Vec3f curve2Pos = lerp(curve2.vertices()[travel], curve2.vertices()[travel + 1], timer); // Linearly interpolate between the vertices of the initial curve over time.
    if (travel < curve1.indices().size()){
      Vec3f curve1Pos = curve1.vertices()[travel]; // Assign the interpolated position to the curve.
      Vec3f curve2Pos = curve2.vertices()[travel]; // Assign the interpolated position to the curve.
      ribbon.vertex(curve1Pos); // Assign the interpolated position to the curve.
      ribbon.vertex(curve2Pos); // Assign the interpolated position to the curve.
      ribbon.color(HSV(float(travel) / float(curve1.vertices().size()), 0.5, 1.0)); // Set the color of the curve to be a gradient.
      ribbon.color(HSV(float(travel) / float(curve2.vertices().size()), 0.5, 1.0)); // Set the color of the curve to be a gradient.
      ribbon.smooth(1.0f, 1);
      travel++;
    }
    // }
    // else {
    //   timer = 0.0f; // Reset the timer.
    //   travel++; // Increment the travel index.
    // }
  }

  // Draw Graphics to Screen:
  void onDraw(Graphics& g) override {
    static Light light; // Create a light named light.
    g.clear(0); // Clear the graphics buffer.
   
    // Establish lighting for scene:
    light.pos(0, 0, 0); // Set the position of the light.
    gl::depthTesting(true); // Enable depth testing.
    g.lighting(true); // Enable lighting.
    g.light(light); // Assign light to the scene.
    light.ambient(HSV(1.0, 0.0, 1.0)); // Set the ambient light color.

    // Draw mesh to scene:
    g.polygonMode(true ? GL_LINE : GL_FILL); // Make the mesh wireframe.
    g.meshColor(); // Set the color of the mesh.
    g.pointSize(10.0); // Set the size of the points.
    g.lineWidth(10.0); // Set the width of the lines.
    g.draw(oldMesh); // Draw the original mesh.
    g.draw(oldVertices); // Draw the original vertices.
    g.draw(newMesh); // Draw the subdivided mesh.
    g.draw(newVertices); // Draw the subdivided vertices.
    g.draw(curveControl); // Draw the curve control points.
    g.draw(curve1); // Draw the initial curve.
    g.polygonMode(true ? GL_FILL : GL_FILL); // Make the mesh solid.      
    g.draw(ribbon); // Draw the final curve.
  }
};

// Function which runs the app:
int main() {
  RayApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1200, 800);
  app.start();
}
// void subdivide(Mesh &m, unsigned iterations, bool normalize) {
//   typedef std::map<uint64_t, unsigned> PointToIndex;

//   if (m.primitive() != Mesh::TRIANGLES)
//     return;

//   for (unsigned k = 0; k < iterations; ++k) {
//     PointToIndex middlePointIndexCache;

//     Mesh::Index newIndex = m.vertices().size();
//     Mesh::Indices oldIndices(m.indices());
//     m.indices().clear();

//     // Iterate through triangles
//     for (unsigned j = 0; j < (unsigned)oldIndices.size(); j += 3) {
//       // printf("%u %u\n", k, j);

//       Mesh::Index *corner = &oldIndices[j];
//       Mesh::Index mid[3];

//       for (unsigned i = 0; i < 3; ++i) {
//         uint64_t i1 = corner[i];
//         uint64_t i2 = corner[(i + 1) % 3];
//         uint64_t key = i1 < i2 ? (i1 << 32) | i2 : (i2 << 32) | i1;

//         PointToIndex::iterator it = middlePointIndexCache.find(key);
//         if (it != middlePointIndexCache.end()) {
//           mid[i] = it->second;
//         } else {
//           middlePointIndexCache.insert(std::make_pair(key, newIndex));
//           Mesh::Vertex v1 = m.vertices()[i1];
//           Mesh::Vertex v2 = m.vertices()[i2];
//           Mesh::Vertex vm;
//           if (normalize) {
//             vm = v1 + v2;
//             // vm.normalize();
//             // use average magnitude to keep smooth
//             vm.normalize((v1.mag() + v2.mag()) * 0.5);
//           } else {
//             vm = (v1 + v2) * 0.5;
//           }
//           m.vertex(vm);
//           // TODO: other attributes (colors, normals, etc.)
//           mid[i] = newIndex;
//           ++newIndex;
//         }
//       }

//       Mesh::Index newIndices[] = {corner[0], mid[0], mid[2],    corner[1],
//                                   mid[1],    mid[0], corner[2], mid[2],
//                                   mid[1],    mid[0], mid[1],    mid[2]};

//       m.index(newIndices, 12);
//     }
//   }
// }

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