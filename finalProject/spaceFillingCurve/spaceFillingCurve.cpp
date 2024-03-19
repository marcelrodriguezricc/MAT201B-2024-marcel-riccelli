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
// 4. Need to assign mesh to state.

// AlloLib Libraries:
#include "al/app/al_DistributedApp.hpp" // For distributing tasks across multiple machines in AlloSphere.
#include "al/app/al_App.hpp" // App library.
#include "al/graphics/al_Shapes.hpp" // Shapes library.
#include "al/graphics/al_Isosurface.hpp" // Isosurface library.
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp" // For distributing state across multiple machines in AlloSphere.
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"
#include <queue> // C++ standard priority queue library.
#include <vector> // C++ standard vector library.

// Namespaces:
using namespace al; // "al" namespace.
using namespace std;// "std" namespace.

// Global Variables:
int numVertices = 500; // Number of vertices to be distributed on the sphere's surface.
float rate = 0.05; // Rate of animation.
float timer; // Timer for animation.
int travel = 0; // Travel index for animation.

#define PI 3.14159265f
#define PHI 1.618033988749895f

float cube_size = 1.0f;

// State structure for the distributed app.
struct State {
  Pose pose; // The pose of the camera.
};

struct vertex{
  Vec3f pos;
  vector<int> edges;
  vector<int> faces;
};

struct edge{
  Vec3f midpoint;
  vector<int> vertices;
  vector<int> faces;
};

struct face{
  Vec3f centroid;
  vector<int> sub_indices;
  vector<int> vertices;
  vector<int> edges;
};

struct quad_mesh{
  vector<vertex> vertices;
  vector<face> faces;
  vector<edge> edges;
};

// Function to create a Cube with quad faces:
quad_mesh create_cube(int size) {

  // Create the quad_mesh structure:
  quad_mesh cube;

  // The vertices of a cube:
  vector<Vec3f> cube_vertices = {
    Vec3f(cube_size * -1.0f, cube_size * -1.0f,  cube_size * 1.0f), // Vertex 0
    Vec3f(cube_size * 1.0f, cube_size * -1.0f, cube_size * 1.0f), // Vertex 1
    Vec3f(cube_size * -1.0f, cube_size * 1.0f, cube_size * 1.0f), // Vertex 2
    Vec3f(cube_size * 1.0f, cube_size * 1.0f, cube_size * 1.0f), // Vertex 3
    Vec3f(cube_size * -1.0f, cube_size * -1.0f, cube_size * -1.0f), // Vertex 4
    Vec3f(cube_size * 1.0f, cube_size * -1.0f, cube_size * -1.0f), // Vertex 5
    Vec3f(cube_size * -1.0f, cube_size *  1.0f, cube_size * -1.0f), // Vertex 6
    Vec3f(cube_size * 1.0f, cube_size * 1.0f, cube_size * -1.0f)  // Vertex 7
  };

  // The indices of the vertices for each face:
  vector<int> cube_indices = {
    0, 1, 3, 2, // Face 0
    3, 1, 5, 7, // Face 1
    3, 7, 6, 2, // Face 2
    6, 4, 0, 2, // Face 3
    6, 7, 5, 4, // Face 4
    5, 1, 0, 4  // Face 5
  };

  // Generate the vertices and store them in the quad_mesh structure:
  for (int i = 0; i < cube_vertices.size(); i++) {
    vertex v;
    v.pos = cube_vertices[i];
    cube.vertices.push_back(v);
  }

  // Generate the faces and store them in the quad_mesh structure:
  for (int i = 0; i < cube_indices.size() / 4; i++) { // For each face of the input mesh...
    face f;
    Vec3f vertex_sum = 0.0f;
    for (int j = 0; j < 4; j++) {
      f.vertices.push_back(cube_indices[i * 4 + j]); // Get the vertices based on the corresponding indices assigned to each face.
      cube.vertices[cube_indices[i * 4 + j]].faces.push_back(i); // Store the face index in the data sub-structure for the vertex.
      vertex_sum += cube.vertices[cube_indices[i * 4 + j]].pos; // Add all of the vertices together.
    }
    f.centroid = vertex_sum / float(4); // Average the vertices to find the centroid.
    cube.faces.push_back(f); // Store the face in the quad_mesh structure.
  }

  // // Generate the edges and store them in the quad_mesh structure:
  int edge_index = 0;
  for (int i = 0; i < cube_indices.size(); i += 2) { // For each pair of vertices in the list of vertices...
    edge e;
    Vec3f test_mp = (cube.vertices[cube_indices[i]].pos + cube.vertices[cube_indices[i + 1]].pos) / 2.0f; // Calculate the midpoint of the edge based on the average of the two associated vertices.
    bool found = false; // Initialize the found flag as set to false.
    for (int j = 0; j < cube.edges.size(); j++){ // For each edge in the list of edges...
      if (cube.edges[j].midpoint == test_mp){ // If the edge midpoint is already in the list of edges...
        found = true; // Set the found flag to true.
      }
    }
    if (!found){ // If the edge midpoint is not already in the list of edges...
      e.midpoint = test_mp; // Store the edge midpoint in the data sub-structure for the edge.
      e.vertices.push_back(cube_indices[i]); // Store the vertex in the data sub-structure for the edge.
      e.vertices.push_back(cube_indices[i + 1]); // Store the second vertex in the data sub-structure for the edge.
      cube.vertices[cube_indices[i]].edges.push_back(edge_index); // Store the edge index in the data sub-structure for the vertex.
      cube.vertices[cube_indices[i + 1]].edges.push_back(edge_index); // Store the edge index in the data sub-structure for the second vertex.
      cube.edges.push_back(e);
      edge_index++;
    }
  }

  // // Find the edges associated with each face:
  for (int i = 0; i < cube.faces.size(); i++) { // For each face in the list of faces...
    for (int j = 0; j < 4; j++) { // For each vertex in the list of vertices associated with the face...
      Vec3f face_mp = (cube.vertices[cube.faces[i].vertices[j]].pos + cube.vertices[cube.faces[i].vertices[(j + 1) % 4]].pos) / 2.0f; // Calculate the midpoint of the first edge of the face.
      for (int k = 0; k < cube.edges.size(); k++) { // For each edge in the list of edges...
        if (cube.edges[k].midpoint == face_mp) { // If the edge midpoint is the same as any previously calculated midpoint...
          cube.faces[i].edges.push_back(k); // Store the edge index in the data sub-structure for the face.
          cube.edges[k].faces.push_back(i); // Store the face index in the data sub-structure for the edge.
        }
      }
    }
  }

  return cube; // Return the fully filled quad_mesh structure.
};

quad_mesh catmull_clark(quad_mesh input) {

  // Create a quad_mesh structure to hold the calculations for our subdivision:
  quad_mesh sub, output;
  vector<int> sub_indices;

  // Set a new edge point which is the average of two neighboring face centroids:
  for (int i = 0; i < input.edges.size(); i++) { // For each edge in the input mesh...
    edge e; // Declare an edge to hold the calculations for the new edge.
    Vec3f face_sum = 0.0f; // Initialize the sum of the face centroids to be zero.
    Vec3f vertex_sum = 0.0f; // Initialize the sum of the vertices to be zero.
    for (int j = 0; j < input.edges[i].faces.size(); j++) { // For each face of the edge...
      face_sum += input.faces[input.edges[i].faces[j]].centroid; // Get the edge to be calculated.
    }
    for (int j = 0; j < input.edges[i].vertices.size(); j++) { // For each vertex of the edge...
      vertex_sum += input.vertices[input.edges[i].vertices[j]].pos; // Get the edge to be calculated.
    }
    e.vertices = input.edges[i].vertices; // Copy the adjoining vertex indices of the input edge to the new edge.
    e.faces = input.edges[i].faces; // Copy the adjoining face indices of the input edge to the new edge.
    e.midpoint = (face_sum + vertex_sum) / (float(input.edges[i].faces.size()) + float(input.edges[i].vertices.size())); // Calculate the midpoint of the edge based on the average of the two associated vertices and the two associated face centroids.
    sub.edges.push_back(e); // Store the new edge in the sub quad_mesh structure.
  }

  // Set new vertices to be the barycenter of each original vertex, the average of the connected edge midpoints, and the average of the connected face centroids with respective weights (n-3), 2, and 1,
  for (int i = 0; i < input.vertices.size(); i++) {
    vertex v; // Declare a vertex to hold the calculations for the new vertex.
    Vec3f edge_sum = 0.0f; // Initialize the sum of the edge midpoints to be zero.
    Vec3f face_sum = 0.0f; // Initialize the sum of the face centroids to be zero.
    for (int j = 0; j < input.vertices[i].edges.size(); j++) { // For each edge of the vertex...
      edge_sum += input.edges[input.vertices[i].edges[j]].midpoint; // Add their midpoints together.
    }
    Vec3f edge_avg = edge_sum / float(input.vertices[i].edges.size()); // Find the average of the connected edge midpoints.
    v.edges = input.vertices[i].edges; // Copy the adjoining edge indices of the input vertex to the new vertex.
    for (int j = 0; j < input.vertices[i].faces.size(); j++) { // For each face of the vertex...
      face_sum += input.faces[input.vertices[i].faces[j]].centroid; // Add their centroids together.
    }
    Vec3f face_avg = face_sum / float(input.vertices[i].faces.size()); // Find the average of the connected face centroids.
    v.faces = input.vertices[i].faces; // Copy the adjoining face indices of the input vertex to the new vertex.
    v.pos = (face_avg + (2.0f * edge_avg) + ((float(input.vertices[i].edges.size()) - 3.0f) * input.vertices[i].pos)) / float(input.vertices[i].edges.size()); // Calculate the barycenter of the original vertex, the average of the connected edge midpoints, and the average of the connected face centroids.
    sub.vertices.push_back(v); // Store the new vertex in the sub quad_mesh structure.
  }

  // For each face, look at the edges.
  // If the edges share a vertex...
  // Create a new face for the output mesh with vertices at the two edge midpoints, the original face centroid, and the shared vertex.
  // If a vertex has already been created by another face, don't create a new vertex.

  // Get faces of new mesh:
  int counter = 0;
  for (int i = 0; i < input.faces.size(); i++) { // For each face in the input mesh...
    for (int j = 0; j < 4; j++) { // For each edge of the face...
      for (int k = 0; k < 2; k++) { // For each vertex of the edge...
        int vert1 = sub.edges[input.faces[i].edges[j]].vertices[k]; // Fetch the vertices one at a time.
        for (int l = 0; l < 4; l++) { // For every other edge of the face...
          if (j != l) { // If the edge is not itself...
            vertex v1, v2, v3, v4; // Declare four vertex structures to hold the calculations for the new vertices.
            int index1, index2, index3, index4; // Declare four indices to hold the calculated indices for the new vertices.
            for (int m = 0; m < 2; m++) { // For each vertex of the other edge...
              int vert2 = sub.edges[input.faces[i].edges[l]].vertices[m]; // Fetch the vertex...
              if (vert1 == vert2) { // If the selected vertex of the first edge is the same as the selected vertex of the other edge...
                face f; // Declare a face to hold the calculations for the new face.
                v1.pos = sub.vertices[vert1].pos; // Fetch the position of the vertex.
                bool match = false;
                for (int n = 0; n < output.vertices.size(); n++) {
                  if (output.vertices[n].pos == v1.pos) {
                    match = true;
                    index1 = n;
                  }
                }
                if (!match) {
                  index1 = output.vertices.size();
                  output.vertices.push_back(v1); // Store the new vertex in the output quad_mesh structure.
                }
                v2.pos = sub.edges[input.faces[i].edges[j]].midpoint; // Fetch the midpoint of the edge.
                match = false;
                for (int n = 0; n < output.vertices.size(); n++) {
                  if (output.vertices[n].pos == v2.pos) {
                    match = true;
                    index2 = n;
                  }
                }
                if (!match) {
                  index2 = output.vertices.size();
                  output.vertices.push_back(v2); // Store the new vertex in the output quad_mesh structure.
                }
                v3.pos = input.faces[i].centroid; // Fetch the centroid of the face.
                match = false;
                for (int n = 0; n < output.vertices.size(); n++) {
                  if (output.vertices[n].pos == v3.pos) {
                    match = true;
                    index3 = n;
                  }
                }
                if (!match) {
                  index3 = output.vertices.size();
                  output.vertices.push_back(v3); // Store the new vertex in the output quad_mesh structure.
                }
                v4.pos = sub.edges[input.faces[i].edges[l]].midpoint; // Fetch the midpoint of the other edge.
                match = false;
                for (int n = 0; n < output.vertices.size(); n++) {
                  if (output.vertices[n].pos == v4.pos) {
                    match = true;
                    index4 = n;
                  }
                }
                if (!match) {
                  index4 = output.vertices.size();
                  output.vertices.push_back(v4); // Store the new vertex in the output quad_mesh structure.
                }
                
                // Store the calculated indices in the data sub-structure for the new face.
                f.vertices.push_back(index1);
                f.vertices.push_back(index2);
                f.vertices.push_back(index3);
                f.vertices.push_back(index4);

                // Store the calculated indices in the indices vector:
                sub_indices.push_back(index1);
                sub_indices.push_back(index2);
                sub_indices.push_back(index3);
                sub_indices.push_back(index4);


                // Calculate the centroid of the new face based on the average of the four vertices:
                f.centroid = (v1.pos + v2.pos + v3.pos + v4.pos) / 4.0f; 

                output.faces.push_back(f); // Store the new face in the output quad_mesh structure.
                f.sub_indices.push_back(i); // Copy the index of the input face to the new face.
              }
            }
          }
        }
      }
    }
  }

  int edge_index = 0;
  for (int i = 0; i < sub_indices.size(); i += 2) { // For each pair of vertices in the list of vertices...
    edge e;
    Vec3f test_mp = (output.vertices[sub_indices[i]].pos + output.vertices[sub_indices[i + 1]].pos) / 2.0f; // Calculate the midpoint of the edge based on the average of the two associated vertices.
    bool found = false; // Initialize the found flag as set to false.
    for (int j = 0; j < sub.edges.size(); j++){ // For each edge in the list of edges...
      if (sub.edges[j].midpoint == test_mp){ // If the edge midpoint is already in the list of edges...
        found = true; // Set the found flag to true.
      }
    }
    if (!found){ // If the edge midpoint is not already in the list of edges...
      e.midpoint = test_mp; // Store the edge midpoint in the data sub-structure for the edge.
      e.vertices.push_back(sub_indices[i]); // Store the vertex in the data sub-structure for the edge.
      e.vertices.push_back(sub_indices[i + 1]); // Store the second vertex in the data sub-structure for the edge.
      output.vertices[sub_indices[i]].edges.push_back(edge_index); // Store the edge index in the data sub-structure for the vertex.
      output.vertices[sub_indices[i + 1]].edges.push_back(edge_index); // Store the edge index in the data sub-structure for the second vertex.
      output.edges.push_back(e);
      edge_index++;
    }
  }

  // Find the edges associated with each face:
  for (int i = 0; i < output.faces.size(); i++) { // For each face in the list of faces...
    for (int j = 0; j < 4; j++) { // For each vertex in the list of vertices associated with the face...
      Vec3f face_mp = (output.vertices[output.faces[i].vertices[j]].pos + output.vertices[output.faces[i].vertices[(j + 1) % 4]].pos) / 2.0f; // Calculate the midpoint of the first edge of the face.
      for (int k = 0; k < output.edges.size(); k++) { // For each edge in the list of edges...
        if (output.edges[k].midpoint == face_mp) { // If the edge midpoint is the same as any previously calculated midpoint...
          output.faces[i].edges.push_back(k); // Store the edge index in the data sub-structure for the face.
          output.edges[k].faces.push_back(i); // Store the face index in the data sub-structure for the edge.
        }
      }
    }
  }
  return output; // Return the subdivided quad_mesh structure.
};

// Comparator for priority queue:
struct CompareDist {
    bool operator()(pair<Vec3f, float> const& p1, pair<Vec3f, float> const& p2) { 
        return p1.second < p2.second; // Returns a boolean (true / false) based on distance from p1 being greater than than distance from p2.
    }
};

// Main App Class:
struct RayApp : public DistributedAppWithState<State> {
  // App Declarations:
  Mesh cube_vertices, catmull_vertices, catmull2_vertices, catmull3_vertices;
  priority_queue<pair<Vec3f, float>, vector<pair<Vec3f, float>>, CompareDist> pq; // Create a priority queue that holds pairs of Vec3f vertices and the float distance between them, ordered by the custom comparator.

  // Cuttlebone for AlloSphere distribution:
  void onCreate() override {
    auto cuttleboneDomain = CuttleboneStateSimulationDomain<State>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    // Camera:
    nav().pos(0, 0, 0); // Set camera at center of scene.

    // Create a mesh:
    quad_mesh cube_mesh = create_cube(cube_size); // Create a cube with quad faces.

    cube_vertices.primitive(Mesh::POINTS); // Set the primitive type of the mesh to points.
    for (int i = 0; i < cube_mesh.vertices.size(); i++){ // For each vertex in the input mesh...
      cube_vertices.vertex(cube_mesh.vertices[i].pos); // Add the vertex to the mesh.
      cube_vertices.color(HSV(1.0, 0.0, 1.0)); // Set the color of the vertex to be white.
    }

    quad_mesh catmull_mesh = catmull_clark(cube_mesh); // Apply a Catmull-Clark subdivision to the input mesh.

    catmull_vertices.primitive(Mesh::POINTS); // Set the primitive type of the mesh to points.
    for (int i = 0; i < catmull_mesh.vertices.size(); i++){ // For each vertex in the input mesh...
      catmull_vertices.vertex(catmull_mesh.vertices[i].pos); // Add the vertex to the mesh.
      catmull_vertices.color(HSV(0.0, 1.0, 1.0)); // Set the color of the vertex to be red.
    }

    quad_mesh catmull2_mesh = catmull_clark(catmull_mesh); // Apply a second Catmull-Clark subdivision.

    catmull2_vertices.primitive(Mesh::POINTS); // Set the primitive type of the mesh to points.
    for (int i = 0; i < catmull2_mesh.vertices.size(); i++){ // For each vertex in the input mesh...
      catmull2_vertices.vertex(catmull2_mesh.vertices[i].pos); // Add the vertex to the mesh.
      catmull2_vertices.color(HSV(0.33, 1.0, 1.0)); // Set the color of the vertex to be green.
    }

    quad_mesh catmull3_mesh = catmull_clark(catmull2_mesh); // Apply a second Catmull-Clark subdivision.

    catmull3_vertices.primitive(Mesh::POINTS); // Set the primitive type of the mesh to points.
    for (int i = 0; i < catmull3_mesh.vertices.size(); i++) { // For each vertex in the input mesh...
      catmull3_vertices.vertex(catmull3_mesh.vertices[i].pos); // Add the vertex to the mesh.
      catmull3_vertices.color(HSV(0.66, 1.0, 1.0)); // Set the color of the vertex to be blue.
    }
  };

  // 7. Animate the curve being drawn over time:
  void onAnimate(double dt) override {
    if(isPrimary()){ // If the app is the primary instance...
      state().pose.set(nav()); // Set the state's pose to the camera's pose.
    } else { // If the app is not the primary instance...
      nav().set(state().pose); // Set the camera's pose to the state's pose.
    }
  };

  // Draw Graphics to Screen:
  void onDraw(Graphics& g) override {
    static Light light; // Create a light named light.
    g.clear(0); // Clear the graphics buffer.
   
    // Establish lighting for scene:
    // light.pos(0, 0, 0); // Set the position of the light.
    // gl::depthTesting(true); // Enable depth testing.
    // g.lighting(true); // Enable lighting.
    // g.light(light); // Assign light to the scene.
    // light.ambient(HSV(1.0, 0.0, 1.0)); // Set the ambient light color.

    // Draw mesh to scene:
    g.polygonMode(true ? GL_LINE : GL_FILL); // Make the mesh wireframe.
    g.meshColor(); // Set the color of the mesh.
    g.pointSize(10.0); // Set the cube_size of the points.
    g.lineWidth(10.0); // Set the width of the lines.
    g.draw(cube_vertices);
    g.draw(catmull_vertices);
    g.draw(catmull2_vertices);
    g.draw(catmull3_vertices);
  }
};

// Function which runs the app:
int main() {
  RayApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1200, 800);
  app.start();
};