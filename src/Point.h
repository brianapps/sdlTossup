#ifndef POINT_H_
#define POINT_H_

#include <cmath>

class Point {
public:
    double x;
    double y;

    Point() : x(0), y(0) {}

    Point(double x, double y) : x(x), y(y) {}
    Point(int x, int y) : x(x), y(y) {}

    Point operator-(const Point& p) {
        return Point(x - p.x, y - p.y);
    }

    Point operator+(const Point& p) {
        return Point(x + p.x, y + p.y);
    }

    Point operator*(const double scalar) {
        return Point(scalar * x, scalar * y);
    }

    double modSquared() {
        return x * x + y * y;
    }

    double mod() {
        return std::sqrt(modSquared());
    }
};

#endif