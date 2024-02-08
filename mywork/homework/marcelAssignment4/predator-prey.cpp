#include "al/app/al_App.hpp"

// how to include things ~ how do we know what to include?
// fewer includes == faster compile == only include what you need
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp" // addCone
#include "al/math/al_Complex.hpp"
#include "al/app/al_GUIDomain.hpp"

// Determine namespaces:
using namespace al;
using namespace std;
#include <fstream>
#include <vector>

const int numBoids = 1500;
const int numPred = 2;
const int numFood = 3;

bool followBoid, followPred, camReturn, camBoid, camPred;

int foodOn[numFood];
float predDist[numBoids];


struct MyApp : public al::App {
    al::Mesh mesh;
    al::Nav predator[numPred], boid[numBoids]; // Nav is a frame of refence in  space that can be moved / animated.
    Vec3f heading[numBoids], food[numFood], cohesion, separation, evasion;

    Parameter fov{"Field of View", "", 0.015, 0.01, 0.1};
    Parameter personalSpace{"Personal Space", "", 3.0, 0.01, 4.0};
    Parameter turnRate{"Turn Rate", "", 0.05, 0.01, 0.1};
    Parameter moveRate{"Move Rate", "", 0.02, 0.01, 0.1};
    Parameter evasionWeight{"Evasion Weight", "", 2.0, 0.01, 2.0};
    Parameter separationWeight{"Separation Weight", "", 1.0, 0.01, 2.0};
    Parameter cohesionWeight{"Cohesion Weight", "", 1.0, 0.01, 2.0};

    Material material;     // Necessary for specular highlights
    Light light;

    // GUI:
    void onInit() override {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(fov); 
        gui.add(personalSpace);
        gui.add(turnRate); 
        gui.add(moveRate);
        gui.add(evasionWeight);
        gui.add(separationWeight);
        gui.add(cohesionWeight);
    }

    void onCreate() {
        // Meshes for our Boids & Pedators:
        addPrism(mesh, 0.5, 0.01, 5, 16, 16);
        mesh.generateNormals();

        // Boids:
        for (int i = 0; i < numBoids; i++){
            boid[i].pos() = rnd::ball<al::Vec3d>() * 3.0;
            boid[i].smooth(0.15);
        }

        // Predators:
        for (int i = 0; i < numPred; i++){
            predator[i].pos() = rnd::ball<al::Vec3d>() * 3.0;
            boid[i].smooth(0.15);
        }

        // Camera:
        nav().pos(0, 0, 10);
        nav().setHome();
        nav().faceToward(0,0,0);
    }

    double phase = 0;
    
    void onAnimate(double dt) {
        // Counter:
        phase += dt;

        // Food:
        if (phase >= 10) { // Every 10 seconds...
            for (int i = 0; i < numFood; i++){
                food[i] = al::rnd::ball<al::Vec3d>(); // Generate a random point for the Food to spawn.
                foodOn[i] = 1;
            }
            phase -= 10; // Reset counter.
        }

        // Prey:
        for (int i = 0; i < numBoids; i++) {

            // Cohesion (turn toward flock average position):
            Vec3f sumPos = 0; // Initialize cumulative variables to accumulate the sum of all flock member positions...
            int flockSize = 0; // and the number of members of the flock.
            for (int j = 0; j < numBoids; j++){ // For each Boid...
                float boidDist = dist(boid[i].pos(), boid[j].pos()); // Find the distance between our current Boid and all others.
                if (i != j && boidDist <= fov){ // If another Boid is within the field of view...
                    flockSize++; // Count the number of members in the flock.
                    sumPos += boid[j].pos(); // Add it's position to an accumulating sum of all flock member positions.
                }
            }
            if (flockSize > 0) {
                cohesion = (sumPos / flockSize) * cohesionWeight; // Find the average position of all neighbors.
                heading[i] = cohesion; // Make a new heading for the average position.
            }
            else {
                heading[i] = 0.;
            }

            // Separation (turn away if too close to flock):
            Vec3f sumClose = 0;
            int numClose = 0;
            for (int j = 0; j < numBoids; j++) { // For each Boid...
                float boidDist = dist(boid[i].pos(), boid[j].pos()); // Find the distance between our current Boid and all others.
                if (boidDist <= personalSpace){ // If a Boid is within the personal space of another...
                    numClose++; // Count the number of Boids which are too close...
                    sumClose += boid[i].pos() + (-1.0 * (boid[j].pos() - boid[i].pos())); // Add the opposite of it's position from A to an accumulating sum of all repeling positions.
                }
            }
            if (numClose > 0){ // If there is at least one Boid which is too close...
                separation = (sumClose / numClose) * separationWeight; // Find the average position of the opposite of all "too close" Boids in relation to the current Boid.
                heading[i] = (cohesion + separation) / 2.0; // Make a new heading which is the average of the cohesion and separation calculations.
            }

            // Evasion (avoid predators):
            Vec3f sumPredClose = 0;
            int numPredClose = 0;
            for (int j = 0; j < numPred; j++) {
                float predBoidDist = dist(boid[i].pos(), predator[j].pos());
                if (predBoidDist <= fov){
                    numPredClose++;
                    sumPredClose += boid[i].pos() + (-1.0 * (predator[j].pos() - boid[i].pos()));
                }
            }
            if (numPredClose > 0){ // If there is at least one Boid which is too close...
                evasion = (sumClose / numClose) * evasionWeight; // Find the average position of the opposite of all "too close" Boids in relation to the current Boid.
                heading[i] = (cohesion + separation + evasion) / 3.0; // Make a new heading which is the average of the cohesion and separation calculations.
            }

            // Consumption (go toward food when available and in sight):
            for (int j = 0; j < numFood; j++) {
                float foodDist = dist(boid[i].pos(), food[j]);
                if (foodOn[j] == 1 && foodDist <= (fov * 100.0)){
                    heading[i] = food[j];
                    if (foodDist <= 0.1){
                        foodOn[j] = 0;
                    }
                }

            }


            // Turn towards center if out of bounds:
            if (abs(boid[i].pos()) > 3.0){ // If out of bounds...
               heading[i] = 0;
            }
            
            boid[i].faceToward(heading[i], turnRate);
            boid[i].moveF(moveRate);
            boid[i].step();
        }

        // Predator:
        for (int i = 0; i < numPred; i++){
            float smallestDist = predDist[0];
            int closestBoid = 0;
            for (int j = 0; j < numBoids; j++){
                predDist[j] = dist(predator[i].pos(), boid[j].pos());
            }
            for (int j = 0; j < numBoids; j++){
                 if (predDist[j] < smallestDist){
                    smallestDist = predDist[j]; 
                    closestBoid = j;
                 }
            }
            predator[i].faceToward(boid[closestBoid].pos(), turnRate / 2);
            predator[i].moveF(moveRate / 4);
            predator[i].step();
        }

        // Camera:
        float camDist = 0;

        // Follow Boid:
        if (followBoid == true){
            camDist = dist(nav().pos(), boid[camBoid].pos());
            nav().faceToward(boid[camBoid].pos());
            if (camDist >= 2){
                nav().moveF(moveRate * 4.0);
            }
            else{
                nav().moveF(0.0);
            }
        }

        // Follow Predator:
        if (followPred == true) {
            camDist = dist(nav().pos(), predator[camPred].pos());
            nav().faceToward(predator[camPred].pos());
            if (camDist >= 2){
                nav().moveF(moveRate * 4.0);
            }
            else{
                nav().moveF(0.0);
            }
        }
    }

    // Keyboard commands for Camera control:
    bool onKeyDown(const Keyboard &k) override {
        // Return Home:
        if (k.key() == '1') {
            followBoid = false;
            followPred = false;
            camReturn = true;
            nav().home();
        }

        // Follow Boid:
        if (k.key() == '2') {
            camReturn = false;
            followPred = false;
            followBoid = true;
            nav().home();
            camBoid = rnd::uniformi(0, numBoids);
        }

        // Follow Pred:
        if (k.key() == '3') {
            camReturn = false;
            followBoid = false;
            followPred = true;
            nav().home();
            camPred = rnd::uniformi(0, numPred);
        }
        return true;
    }

    // Presentation:
    void onDraw(al::Graphics& g) {
        g.clear(HSV(0.66, 1, 0.2));
        g.depthTesting(true);
        g.lighting(true);
        g.light(light);
        light.pos(0, 0, 10);
        light.dir(0, 0, -10);
        light.ambient(HSV(0.66, 1, 1)); 
        light.diffuse(HSV(.33, 1, 1));
        material.specular(light.diffuse() * 0.2);  // Specular highlight, "shine"
        material.shininess(30);  // Concentration of specular component [0,128]
        g.material(material);

        // Prey:
        g.color(HSV(0.33, 0.25, 1));
        for (int i = 0; i < numBoids; i++){
            g.pushMatrix();
            g.translate(boid[i].pos());
            //g.rotate(180);
            g.rotate(boid[i].quat());
            g.scale(0.05);
            g.draw(mesh);
            g.popMatrix();
        }

        // Predator:
        g.color(HSV(1, 0.75, 1));
        for (int i = 0; i < numPred; i++){
            g.pushMatrix();
            g.translate(predator[i].pos());
            //g.rotate(180);
            g.rotate(predator[i].quat());
            g.scale(0.1);
            g.draw(mesh);
            g.popMatrix();
        }

        // Food:
        g.color(HSV(.66, 0.2, 1));
        for (int i = 0; i < numFood; i++){
            if (foodOn[i] == 1){
                g.pushMatrix();
                g.translate(food[i]);
                g.scale(0.075);
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
