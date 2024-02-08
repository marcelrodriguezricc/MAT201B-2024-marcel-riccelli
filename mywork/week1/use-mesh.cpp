#include "al/app/al_App.hpp"

struct MyApp : public al::App {

    al::Mesh mesh;

    void onCreate() {
        mesh.primitive(al::Mesh::TRIANGLES);
        mesh.vertex(0, 0, 0); // First vertex.
        mesh.vertex(0, 1, 0); // Second vertex.
        mesh.vertex(1, 0, 0); // Third vertex.
        nav().pos(0, 0, 4);
    }

    void onDraw(al::Graphics& g){
        g.clear(1); // Background color.
        g.color(0); // Shape color.
        g.draw(mesh);
    }
};

int main(){
    MyApp app;
    app.configureAudio(48000, 512, 2, 0);
    app.start();
}

 // int main() { MyApp().start(); }