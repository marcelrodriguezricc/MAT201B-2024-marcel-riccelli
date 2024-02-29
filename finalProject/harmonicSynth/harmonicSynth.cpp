// Tasks:
//
// RAYMARCHING:
// 1. Create the spherical raymarched scene, and display a single metaball cluster inside of it. Visualize in cube.
// 2. Animate the metaball cluster to oscillate.
// 3. Orbit the metaball cluster around the viewer.
// 4. Introduce more clusters, have them repel each other.
//
// AUDIO:
// 1. Build a sine wave synthesizer, which can scan through the overtone series of a set fundamental.
// 2. Allow each sine wave to be frequency modulated.
// 4. Audio should be spatialized based on the position of the Metaball.
// 3. Add reverberation.
//
// LINKING:
// 1. Metaball cluster should oscillate in a way dictated by the frequencies present in the audio signal of the respective sine-wave synthesizer.
// This should be accomplished through an FFT analysis of the audio signal controlling the metaball cluster.
// Each bin will contribute to the rate of the cluster oscillation.
// 2. Metaballs approaching each other should cause the sine wave synthesizer to frequency modulate, based on the frequency of the synthesizer related to the cluster it's approaching.
//
// CONTROL:
// 1. A neural network model will receive a two dimensional integer on it's input, and output interpolations between multiple states of the synthesizer's parameters.
// 2. The neural network will send the output to the synthesizer via OSC.
//
// TO DO NEXT:
// 1. Create a class which contains a nav for the position of the cluster.
// 2. Make it orbit around the viewer.
// 3. Create a class for each sphere within the cluster.
// 4. Make the spheres oscillate.

// Libraries:
#include "al/app/al_DistributedApp.hpp" // For distributing tasks across multiple machines in AlloSphere.
#include "al/app/al_GUIDomain.hpp" // GUI.
#include "al/ui/al_ParameterGUI.hpp" // Parameters to GUI.
#include "al_ext/statedistribution/al_CuttleboneDomain.hpp" // For distributing state across multiple machines in AlloSphere.

using namespace al;

// State structure for the distributed app.
struct State {
  Pose pose; // The pose of the camera.
};


struct RayApp : public DistributedAppWithState<State> {
  // Class Declarations:
  VAOMesh quad; // A fullscreen quad mesh for which to color with our shader.
  ShaderProgram clusters; // The raymarched shader program.
    
  // Watch for changes in the shader file and reload.
  SearchPaths searchPaths; // A search path for shader files.
  struct WatchedFile { 
    File file; // The file to watch.
    al_sec modified; // The last time the file was modified.
  };
  std::map<std::string, WatchedFile> watchedFiles; // A map of watched files.
  al_sec watchCheckTime; // The last time the files were checked for modification.

  // GUI Parameters:
  ControlGUI *gui; // GUI for controlling uniform parameters.
  Parameter clusterPosX{"Cluster Position X", "Clusters", 0.0, -50.0, 50.0};
  Parameter clusterPosY{"Cluster Position Y", "Clusters", 0.0, -50.0, 50.0};
  Parameter clusterPosZ{"Cluster Position Z", "Clusters", 0.0, -50.0, 50.0};
  // Parameter orbitSpeed{"orbitSpeed", "Clusters", 0.1, 0.0, 10.0}; // The position of the cluster.
  // Parameter fundamental{"Fundamental", "Oscillators", 220.0, 20.0, 20000.0}; // The fundamental frequency of the sine wave synthesizer.
  // Parameter eyeSep{"Eye Separation", "Raymarching", 0.02, 0., 0.5};
  // Parameter focalLength{"Focal Length", "Raymarching", 0.02, 0., 0.5};
  // Parameter lightPos{"Light Position", "Raymarching", 0.02, 0., 0.5};
  // Parameter specular{"Specular", "Raymarching", 0.5, 0., 2.0};
  // Parameter diffusion{"Diffusion", "Raymarching", 0.05, 0., 0.5};

  std::shared_ptr<CuttleboneDomain<State>> cuttleboneDomain; // The domain for distributing and synchronizing the app state.

  // When initializing the app:
  void onInit() override {
    searchPaths.addSearchPath(".", false); // Add the current directory to the search path.
		searchPaths.addAppPaths(); // Add the app's paths to the search path.
    searchPaths.addRelativePath("../shaders", true); // Add the shaders directory to the search path.
    searchPaths.print(); // Print the search paths.
  }

  // When creating the app:
  void onCreate() override {
  cuttleboneDomain = CuttleboneDomain<State>::enableCuttlebone(this); // Enable Cuttlebone.
  if (!cuttleboneDomain) { // If Cuttlebone could not be enabled...
    std::cerr << "ERROR: Could not start Cuttlebone" << std::endl; // Print an error message.
    quit(); // Quit the app.
  }

  // Initialize the quad mesh which will display the raymarched scene.
  quad.primitive(Mesh::TRIANGLE_STRIP); // Set the primitive type to triangle strip.
  quad.vertex(-1, -1, 0); // The bottom left vertex.
  quad.vertex(1, -1, 0); // The bottom right vertex.
  quad.vertex(-1, 1, 0); // The top left vertex.
  quad.vertex(1, 1, 0); // The top right vertex.
  quad.update(); // Update the mesh.

  // Initialize GUI and parameter callbacks:
  if (isPrimary()) {
    auto guiDomain = GUIDomain::enableGUI(defaultWindowDomain()); // Enable the GUI.
    gui = &guiDomain->newGUI(); // Create the GUI
    *gui << clusterPosX << clusterPosY << clusterPosZ; // Assign our parameters to the GUI.
  }

  parameterServer() << clusterPosX << clusterPosY << clusterPosZ; // Make parameters accessible via OSC.
  nav().pos(0,0,5); // Set the camera position at the center of the 3D space.
  reloadShaders(); // Load the shader files.
  }  

  // Reload the shader files when they are modified:
  void reloadShaders() {
    loadShader(clusters, "clusters.vert", "clusters.frag"); // Call the loadShader function to reload the shader program.
  }

  // Animate loop, here we'll watch for changes in the shader files and the camera pose:
  void onAnimate(double dt) override {
    // Watch for changes in the shader files, and update accordingly:
    if (watchCheck()) { // If the shader files have been modified...
      printf("shader files changed, reloading..\n"); // Print a message stating that the files have been changed.
      reloadShaders(); // Reload the shaders.
    }
    // **Ask Karl what's going on here:**
    if(isPrimary()){ // If the app is the primary instance...
      state().pose.set(nav()); // Set the state's pose to the camera's pose.
    } else { // If the app is not the primary instance...
      nav().set(state().pose); // Set the camera's pose to the state's pose.
    }
  }

  void onDraw(Graphics &g) override {
    Vec3f clusterPos = Vec3f(clusterPosX, clusterPosY, clusterPosZ); // Set the cluster position to the GUI parameters.
    g.clear(0); // Clear the graphics buffer.
    clusters.use(); // Use the raymarched shader program.
    clusters.uniform("clusterPos", clusterPos) // Pass the position of the cluster to the shader.
    .uniform("cam_pos", nav().pos()) // Position of the camera.
    .uniform("foc_len", g.lens().focalLength()) // Focal length of the lens.
    .uniform("eye_sep", g.lens().eyeSep() * g.eye() / 2.0f) // Eye separation.
    .uniform("al_ProjMatrixInv", Matrix4f::inverse(g.projMatrix())) // Pass the inverse projection matrix to the shader.
    .uniform("al_ViewMatrixInv", Matrix4f::inverse(g.viewMatrix())) // Pass the inverse view matrix to the shader.
    .uniform("al_ModelMatrixInv", Matrix4f::inverse(g.modelMatrix())); // Pass the inverse model matrix to the shader.
    quad.draw(); // Draw the quad mesh displaying the raymarched scene.
  }

  // Respond to keystrokes:
  bool onKeyDown(const Keyboard &k) override {}

  // Choose file to watch for changes.
  void watchFile(std::string path) {
    File file(searchPaths.find(path).filepath()); // Find the file in the search paths.
    watchedFiles[path] = WatchedFile{ file, file.modified() };
  }

  // Watch for changes in files:
  bool watchCheck() {
    bool changed = false; // A flag to indicate if the files have changed, false by default.
    if (floor(al_system_time()) > watchCheckTime) { // If the time has passed since the last check...
      watchCheckTime = floor(al_system_time()); // Set the time of the last check to the current time.
      for (std::map <std::string, WatchedFile>::iterator i = watchedFiles.begin(); i != watchedFiles.end(); i++) { // Iterate through the watched files.
        WatchedFile& watchedFile = (*i).second; // Retrieve the files.
        if (watchedFile.modified != watchedFile.file.modified()) { // If the file has been modified...
          watchedFile.modified = watchedFile.file.modified(); // Set the modified time to the current time.
          changed = true; // Set the changed flag to true.
        }
      }
    }
    return changed; // Return the changed flag boolean.
  }

  // Change file if modified:   
  std::string loadGlsl(std::string filename) {
    watchFile(filename); // Watch the input file for changes.
		std::string code = File::read(searchPaths.find(filename).filepath()); // Read the file.
		size_t from = code.find("#include \""); // Find the include directive.
		if (from != std::string::npos) { // If the include directive is found...
			size_t capture = from + strlen("#include \""); // Begin the catpuring the code.
			size_t to = code.find("\"", capture); // End code capture.
			std::string include_filename = code.substr(capture, to-capture); // Get the filename.
			std::string replacement = File::read(searchPaths.find(include_filename).filepath());  // Read the file.
			code = code.replace(from, to-from+2, replacement); // Replace the include directive with the code inside of the file.
			//printf("code: %s\n", code.data());
		}
		return code; // Return the code.
	}

  // Load a shader program from file:
  void loadShader(ShaderProgram& program, std::string vp_filename, std::string fp_filename) {
	  std::string vp = loadGlsl(vp_filename); // Load the vertex shader.
	  std::string fp = loadGlsl(fp_filename); // Load the fragment shader.
	  program.compile(vp, fp); // Compile the shader program.
  }
};

// Main Function:
int main() {
  RayApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1200, 800);
  app.start();
}

