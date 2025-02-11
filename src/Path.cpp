#include <algorithm>
#include "Path.h"

void Path::moveTo(Point p) {
    // points.push_back(p);
    first = p;
    last = p;
    if (edges.empty()) {
        leftBound = rightBound = p.x;
        bottomBound = topBound = p.y;
    }
}

void Path::lineTo(Point p) {
    if (p.y > last.y) {
        edges.emplace_back(last, p);
    } else if (p.y != last.y) {
        edges.emplace_back(p, last);
    }
    leftBound = std::min(leftBound, p.x);
    rightBound = std::max(rightBound, p.x);
    topBound = std::min(topBound, p.y);
    bottomBound = std::max(bottomBound, p.y);
    last = p;
}

void Path::curveTo(Point b, Point c, Point d, int div) {
    Point a1 = last * 0.5 + b * 0.5;
    Point b1 = b * 0.5 + c * 0.5;
    Point c1 = c * 0.5 + d * 0.5;
    Point a2 = a1 * 0.5 + b1 * 0.5;
    Point b2 = b1 * 0.5 + c1 * 0.5;
    Point a3 = a2 * 0.5 + b2 * 0.5;

    if (div >= 3) {
        lineTo(a3);
        lineTo(d);
    } else {
        curveTo(a1, a2, a3, div + 1);
        curveTo(b2, c1, d, div + 1);
    }
}

void Path::curveTo(Point b, Point c, Point d) {
    double distBetweenEnds = (d - last).mod();
    double distAroundControlPoints = (b - last).mod() + (c - b).mod() + (d - c).mod();

    if (distAroundControlPoints > 1.0001 * distBetweenEnds) {
        // last    b     c     d
        //     \  /  \  /  \  /
        //      a1    b1    c1
        //        \  /  \  /
        //         a2    b2
        //           \  /
        //            a3

        Point a1 = last * 0.5 + b * 0.5;
        Point b1 = b * 0.5 + c * 0.5;
        Point c1 = c * 0.5 + d * 0.5;
        Point a2 = a1 * 0.5 + b1 * 0.5;
        Point b2 = b1 * 0.5 + c1 * 0.5;
        Point a3 = a2 * 0.5 + b2 * 0.5;

        curveTo(a1, a2, a3);
        curveTo(b2, c1, d);
    } else {
        lineTo(d);
    }
}

void Path::end() {
    std::sort(edges.begin(), edges.end(), [](const Edge& e1, const Edge& e2) { return e1.start.y < e2.start.y; });
    nextEdgeIndex = 0;
    firstActive = nullptr;
}

void Path::scanLine(double y, double xFirst, double spacing, int length, const int subSamples, uint8_t* results) {
    std::vector<double> intersections;

#if 1
    // remove edges that we've skipped pass
    Edge** next = &firstActive;
    while (*next != nullptr) {
        if ((*next)->end.y < y) {
            *next = (*next)->nextActive;
        }
        else {
            next = &(*next)->nextActive;
        }
    }

    // then add new candidates
    while (nextEdgeIndex < edges.size()) {
        Edge& edge = edges[nextEdgeIndex];
        if (edge.start.y > y)
            break;
        if (y < edge.end.y) {
            edge.nextActive = firstActive;
            firstActive = &edge;
        }
        nextEdgeIndex++;
    }

    // compute intersections
    Edge* e = firstActive;
    while (e != nullptr) {
        double intersectionX = e->start.x + (e->end.x - e->start.x) * (y - e->start.y) / (e->end.y - e->start.y);
        intersections.push_back(intersectionX - xFirst);
        e = e->nextActive;
    }


    // then fill

    if (intersections.size() > 1) {
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int firstPixel = std::max(0, static_cast<int>(subSamples * intersections[i] / spacing));
            int lastPixel = std::min(length * subSamples, static_cast<int>(subSamples * intersections[i + 1] / spacing));
            for (int pixel = firstPixel; pixel < lastPixel; ++pixel) results[pixel / subSamples]++;
        }
    }
#else
    for (const Edge& edge : edges) {
        const Point& p1 = edge.start;
        const Point& p2 = edge.end;
        if (p1.y > y)
            break;
        if (y >= p1.y && y < p2.y) {
            double intersectionX = p1.x + (p2.x - p1.x) * (y - p1.y) / (p2.y - p1.y);
            intersections.push_back(intersectionX - xFirst);
        }
    }
    if (intersections.size() > 1) {
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int firstPixel = std::max(0, static_cast<int>(subSamples * intersections[i] / spacing));
            int lastPixel = std::min(length * subSamples, static_cast<int>(subSamples * intersections[i + 1] / spacing));
            for (int pixel = firstPixel; pixel < lastPixel; ++pixel) results[pixel / subSamples]++;
        }
    }
#endif
}
