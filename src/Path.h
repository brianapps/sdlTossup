#ifndef PATH_H_
#define PATH_H_
#include <vector>
#include <cstdint>
#include "Point.h"

class Path {
public:
    std::vector<Point> points;
    Point first;
    Point last;

    double leftBound;
    double topBound;
    double bottomBound;
    double rightBound;

    Path() = default;

    void moveTo(Point p);

    void lineTo(Point p);

    void curveTo(Point b, Point c, Point d, int div);

    void curveTo(Point b, Point c, Point d);

    void close() {
        lineTo(first);
    }

    void scanLine(double y, double xFirst, double spacing, int length, int subSamples, uint8_t* results);
};

#endif  // PATH_H_
