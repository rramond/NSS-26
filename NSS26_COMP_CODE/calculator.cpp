#include <iostream>
#include <cmath>
using namespace std;

const int rpm = 0.7; //Wanted rotation speed in revolutions per minute
const double pi = 3.14159265358979323846; 
const double g = 9.81; //Standard gravity in m/s^2
const double omega = (rpm * 2 * pi) / 60.0; // Angular velocity in radians per second

//radius calculation from desired artificial gravity
void radius(double artificialGravity) {
    cout << "Enter the desired artificial gravity (measured in g): ";
    cin >> artificialGravity;
    double r = (artificialGravity * g) / (omega * omega);
    cout << "The required radius is: " << r << " meters" << endl;
}

void artificialGravityCalc(double radius) {
    double r;
    cout << "Enter the radius (in meters): ";
    cin  >> r;
    double artificialGravity;
    artificialGravity = (r * (omega * omega)) / g;
    cout << "The artificial gravity at this radius is: " << artificialGravity << " g" << endl;
}

// Torus volume with elliptical cross-section
// V = 2 * π² * R * a * b
// R = major radius, a & b = semi-axes of ellipse
void torusVolume() {
    double R, a, b;
    cout << "Enter major radius of the torus (meters): ";
    cin >> R;
    cout << "Enter semi-major axis of ellipse (meters): ";
    cin >> a;
    cout << "Enter semi-minor axis of ellipse (meters): ";
    cin >> b;

    double V = 2 * pi * pi * R * a * b;
    cout << "The torus volume is: " << V << " cubic meters" << endl;
}

// Coriolis acceleration magnitude
// a = 2 * ω * v
void coriolisEffect() {
    double v;
    cout << "Enter velocity of the object relative to the station (m/s): ";
    cin >> v;

    double a = 2 * omega * v;
    cout << "Coriolis acceleration magnitude is: " << a << " m/s^2" << endl;
}

int main() {
    int choice;
    cout << "Select an option:\n";
    cout << "1. Calculate required radius for desired artificial gravity\n";
    cout << "2. Calculate artificial gravity at a given radius\n";
    cout << "3. Calculate torus volume (elliptical cross-section)\n";
    cout << "4. Calculate Coriolis effect acceleration\n";
    cin >> choice;