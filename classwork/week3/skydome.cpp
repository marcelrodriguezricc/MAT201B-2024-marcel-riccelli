// Make a Spherical Mesh with Texture Coordinates.
// Open an image.
// Make a texture.

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"

using namespace std;
using namespace al;

struct MyApp : public App {
    Mesh skydome;
    Texture texture;

    void onCreate(){
        // Spherical mesh with texture coordinates:
        addSphereWithTexcoords(skydome, 1.0, 160, true);
        // skydome.primitive(Mesh::LINES);
        // Load image:
        //auto file = File::currentPath() + "../../../MAT201B-2024/church.jpg"; // Locate file "church.jpg".
        auto file = File::currentPath() + "../../../MAT201B-2024/map.jpg"; // Locate file "map.jpg".
        //auto file = File::currentPath() + "../../../MAT201B-2024/outside.jpg"; // Locate file "outside.jpg".
        //auto file = File::currentPath() + "../../../MAT201B-2024/horizon.jpg"; // Locate file "horizon.jpg".
        auto image = Image(file); // Open file.
        // Assign texture:
        texture.create2D(image.width(), image.height()); // Send message to GPU to allocate 2D array for image pixels.
        texture.submit(image.array().data(), GL_RGBA, GL_UNSIGNED_BYTE); // Take data from image we loaded and put into allocated 2D array.
        texture.filter(Texture::LINEAR); // Linear interpolation between colors.
    }

    void onAnimate(double dt){        
    }

    void onDraw(al::Graphics& g){
        g.clear(1);
        g.depthTesting(true);
        texture.bind();
        g.texture(); // Tell AlloLib to use the texture shader pipeline.
        g.draw(skydome);
    }

    bool onKeyDown(Keyboard const& k){
        return true;
    }
};

int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}
