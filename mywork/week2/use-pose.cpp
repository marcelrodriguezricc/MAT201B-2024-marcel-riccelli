#include "al/app/al_App.hpp"

using namespace al;
using namespace std;

// Function for Printing a Quatd:
void print(Quatd const& q){
    std::cout
        << '{'
        << q.w << ' '
        << q.x << ' '
        << q.y << ' '
        << q.z << ' '
        << '}';
}


int main() {
    Pose a, b;
    b.pos(0, 1, 5);
    Quatd(b.pos()).print();
    b.quat(Quatd(b.pos()).normalize());
    a.print();
    b.print();
}