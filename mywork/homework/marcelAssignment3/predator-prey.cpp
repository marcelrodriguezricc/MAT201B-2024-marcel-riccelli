#include "al/app/al_App.hpp"

// how to include things ~ how do we know what to include?
// fewer includes == faster compile == only include what you need
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp" // addCone

// Determine namespaces:
using namespace al;
using namespace std;
#include <fstream>
#include <vector>

const int numPrey = 20;
const int numPred = 5;
const int numFood = 2;
float smallestDist, findDist;
int closestPrey, foodOn[numFood];
float foodDist[numFood], preyDist[numPrey], predDist[numPred];

struct MyApp : public al::App {
    al::Mesh mesh;
    al::Nav predator[numPred], prey[numPrey]; // Nav is a frame of refence in  space that can be moved / animated.
    al::Vec3d food[numFood], preyTarget;

    void onCreate() {
        addCone(mesh);
        mesh.generateNormals();

        // Predator:
        for (int i = 0; i < numPred; i++){
            predator[i].pos() = al::rnd::ball<al::Vec3d>() * 2.0;
        }

        // Prey:
        for (int i = 0; i < numPrey; i++){
            prey[i].pos() = al::rnd::ball<al::Vec3d>() * 2.0;
        }

        // Camera:
        nav().pos(0, 0, 10);
        nav().faceToward(0,0,0);
    }

    double phase = 0;
    void onAnimate(double dt) {
        // Counter:
        phase += dt;

        // Global Variables:
        double turn_rate = 0.05;
        double move_rate = 0.02;

        // Food:
        if (phase >= 5) { // Every 10 seconds...
            for (int i = 0; i < numFood; i++){
                food[i] = al::rnd::ball<al::Vec3d>(); // Generate a random point for the Food to spawn.
                foodOn[i] = 1;
            }
            phase -= 5; // Reset counter.
        }

        //    foodDist[i] = dist(prey[i].pos(), food[j]); // Find the distance from each Food to each Prey and store in an array.
        //    smallestDist = foodDist[0]; // Initialize smallest distance...
        //    for (int j = 0; j < numFood; j++){
        //        if (foodDist[i] < smallestDist){ // and find the food which is the smallest distance from the Prey, then store.
        //            smallestDist = foodDist[i];   
        //            preyTarget[i] = food[j];
        //    }

        // Prey:

        // 1. Find each neighborhood using nested for loop.
        // 2. For neighborhood, find average position of all neighbors.
        // 3. Turn toward average position, unless too close.
        for (int i = 0; i < numPrey; i++){
            if (abs(prey[i].pos()) < 2.0){
                for (int j = 0; j < numPred; j++){
                    float magnitude = dist(prey[i].pos(), predator[j].pos());
                    Vec3f repel = prey[i].pos() - predator[j].pos();
                    repel *= (1.0 / pow(magnitude, 2));
                    preyTarget += repel;
                }
            }
            else {
                preyTarget = 0;
            }

            for (int j = 0; j < numFood; j++){
                    foodDist[j] = dist(prey[i].pos(), food[j]);
                    if (foodDist[j] < 0.1){
                        foodOn[j] = 0;
                    }
            }

            smallestDist = 100.0;
            for (int j = 0; j < numFood; j++) {
                if  (foodOn[j] == 1){
                    if (foodDist[j] < smallestDist){
                        smallestDist = foodDist[j];
                        preyTarget = food[j];
                    }
                }
            }


            prey[i].faceToward(preyTarget, turn_rate); 
            prey[i].moveF(move_rate);
            prey[i].step();
        }

        // Predator:
        for (int i = 0; i < numPred; i++){
            for (int j = 0; j < numPrey; j++){
                preyDist[j] = dist(predator[i].pos(), prey[j].pos());
            }
            smallestDist = preyDist[0];
            closestPrey = 0;
            for (int j = 0; j < numPrey; j++){
                if (preyDist[j] < smallestDist){
                    smallestDist = preyDist[j];   
                    closestPrey = j;
                }
            }
            predator[i].faceToward(prey[closestPrey].pos(), turn_rate);
            predator[i].moveF(move_rate / 4);
            predator[i].step();
        }
    }

    void onDraw(al::Graphics& g) {
        // Background:
        g.depthTesting(true);
        g.lighting(true);
        g.clear(0.5);

        // Prey:
        g.color(0, 1, 0);
        for (int i = 0; i < numPrey; i++){
            g.pushMatrix();
            g.translate(prey[i].pos());
            g.rotate(prey[i].quat());
            g.scale(0.05);
            g.draw(mesh);
            g.popMatrix();
        }

        // Predators:
        g.color(1, 0, 0);
        for (int i = 0; i < numPred; i++){
            g.pushMatrix();
            g.translate(predator[i].pos());
            g.rotate(predator[i].quat());
            g.scale(0.1);
            g.draw(mesh);
            g.popMatrix();
        }

        // Food:
        for (int i = 0; i < numFood; i++){
            if (foodOn[i] == 1){
                g.color(1, 1, 1);
                g.pushMatrix();
                g.translate(food[i]);
                g.scale(0.25);
                g.scale(0.1);
                g.draw(mesh);
                g.popMatrix();
            }
        }
    }
};

int main() {
    MyApp app;
    app.configureAudio(48000, 512, 2, 0);
    app.start();
}

// int main() {  MyApp().start(); }
