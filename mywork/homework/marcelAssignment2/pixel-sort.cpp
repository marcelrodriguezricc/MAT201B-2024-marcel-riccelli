// Include functions from the following libraries:
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"

// Use the following namespaces:
using namespace al;
using namespace std;
#include <fstream>
#include <vector>

// Used to call the shader files for the pointShader shader program. What is slurp?
string slurp(string fileName);

// The AlloApp structure, which allows us to run AlloLib applications:
struct AlloApp : App
{
  // Parameter declarations:
  Parameter pointSize{"/pointSize", "", 1.0, 0.1, 3.0}; // Size of vertices, what does each number represent?
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};  // Length of our time step to be used in animation, what does each number represent?

  ShaderProgram pointShader; // Call a premade shader called pointShader.

  // The function which determines what happens on initialization:
  void onInit() override
  {
    // Create a Graphic User Interface (GUI):
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize); // Add the pointSize parameter to be adjustable in the GUI.
    gui.add(timeStep);  // Add the pointSize parameter to be adjustable in the GUI.
  }

  // The meshes to be used to create objects within the scene when called.
  Mesh current, next;
  Mesh imageMesh;
  Mesh rgbMesh;
  Mesh hsvMesh;
  Mesh labMesh;

  // The function which creates the objects within our scene:
  void onCreate() override
  {

    // Compile the vertex, fragment, and geometry shaders for the AlloLib pointShader shader program:
    pointShader.compile(slurp("../point-vertex.glsl"), slurp("../point-fragment.glsl"), slurp("../point-geometry.glsl"));

    // Mesh declarations:
    current.primitive(Mesh::POINTS);   // A placeholder for the mesh currently being reference for particle display.
    next.primitive(Mesh::POINTS);      // A placeholder for the mesh being targeted for particle movement.
    imageMesh.primitive(Mesh::POINTS); // Mesh for the points organized according to the pixels in the original image.
    rgbMesh.primitive(Mesh::POINTS);   // Mesh for the RGB color space.
    hsvMesh.primitive(Mesh::POINTS);   // Mesh for the HSV color space.
    labMesh.primitive(Mesh::POINTS);   // Mesh for the LAB color space.

    // Loading the image:
    auto file = File::currentPath() + "../liquidLightShow.jpg"; // The filepath for the image.
    auto image = Image(file);                            // Store the image.
    if (image.width() == 0)
    {                                       // If the image's width is equal to zero pixels, determine that it has loaded incorrectly.
      cout << "did not load image" << endl; // Print this message to the console.
      exit(1);
    }

    auto aspect_ratio = 1.0f * image.width() / image.height(); // Aspect ratio based on image size.

    // Here's the nested for loop which will create our imageMesh image, and it's transformation:
    for (int j = 0; j < image.height(); j++)
    { // For every row of pixels in the image...
      for (int i = 0; i < image.width(); i++)
      { // And for every pixel within each row...

        auto pixel = image.at(i, j); // Label each pixel with a coordinate?

        // The vertcies of this mesh are created and colored according to the corresponding pixel in the input image:
        imageMesh.vertex(1.0 * i / image.width() * aspect_ratio, 1.0 * j / image.height(), 0);
        imageMesh.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        imageMesh.texCoord(0.05, 0); // s, t

        // The vertcies of this mesh are positioned and colored according to the RGB values of the corresponding pixel:
        RGB rgb(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        rgbMesh.vertex(rgb.r, rgb.g, rgb.b);
        rgbMesh.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        rgbMesh.texCoord(0.5, 0);

        // The vertcies of this mesh are positioned according to the HSV values of the corresponding pixel mapped to a
        // cylindrical shape, with H representing the angle, S representing the radius, and V representing the height:
        HSV hsv = rgb;
        hsvMesh.vertex(sin(M_PI * 2.0 * hsv.h) * hsv.s, hsv.v, cos(M_PI * 2.0 * hsv.h) * hsv.s);
        hsvMesh.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        hsvMesh.texCoord(0.5, 0);

        // The verticies of this mesh are positioned based on the LAB values of the corresponding pixel:
        Lab lab = rgb;
        labMesh.vertex(lab.l / 100, lab.a / 100, lab.b / 100);
        labMesh.color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);
        labMesh.texCoord(0.5, 0);
      }
    }

    // Determine the position of the camera five in the z direction, to give us a view of our objects.
    nav().pos(.45, .45, 7.5);

    // Initialize the current and next placeholder meshes:
    current = imageMesh;
    next = imageMesh;
  }

  // Initialize the time accumulator:
  double t = 1;

  void onAnimate(double dt) override
  {

    if (t < 1)
    { // If the total of frames accumulated is less than one.

      t += dt; // Count the frames.

      // Perform a liner interpolation for each vertex in the current and next meshes based on our frame accumulation
      // to morph between them (A  * (1 - t) + B * t):
      for (int i = 0; i < current.vertices().size(); i++) {
        current.vertices()[i] = current.vertices()[i] * (1 - t) + next.vertices()[i] * t;
      }
    }

    // If the total frames accumulated exeeds one, then stop the accumulation and interpolation:
    else if (t >= 1)
    {
      t = 1;
    }
  }

  // The user key commands, which will allow the user to select which meshes will be interpolated between
  // and reset the frame acculuation.
  bool onKeyDown(const Keyboard &k) override
  {

    // 1 will change to the original image mesh:
    if (k.key() == '1')
    {
      current = next;
      next = imageMesh;
      t = 0;
    }

    // 2 will change to the RGB mesh:
    if (k.key() == '2')
    {
      current = next;
      next = rgbMesh;
      t = 0;
    }

    // 1 will change to the HSV mesh:
    if (k.key() == '3')
    {
      current = next;
      next = hsvMesh;
      t = 0;
    }

    // 1 will change to the LAB mesh:
    if (k.key() == '4')
    {
      current = next;
      next = labMesh;
      t = 0;
    }
    return true;
  }

  // This loop will continue to draw the shader to the screen each frame:
  void onDraw(Graphics &g) override
  {
    g.clear(0.3);                                     // Clear the screen with a greyish color.
    g.shader(pointShader);                            // Call the shader previously named pointShader.
    g.shader().uniform("pointSize", pointSize / 100); // Create a uniform named pointSize and make it equal to pointSize / 100.

    // What do these parameters mean again?
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);

    // Draw the mesh.
    g.draw(current);
  }
};

// Running the AlloApp:
int main()
{
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

// Not sure what this is:
string slurp(string fileName)
{
  fstream file(fileName);
  string returnValue = "";
  while (file.good())
  {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
