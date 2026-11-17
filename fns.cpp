#include <cmath>
#include <cstdlib>
#include <vector>

#include "header.hpp"

typedef std::vector<Point> vP;

// sqrt is much faster than hypot, and our coordinates are small so overflow isn't a concern
float findDistance(const Point p1, const Point p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

vP generatePoints(int num) {
    vP points;
    points.reserve(num);
    for (int i = 0; i < num; i++) {
        Point point;
        point.x = (rand() % 1000) / 100.0f;
        point.y = (rand() % 1000) / 100.0f;
        points.push_back(point);
    }
    return points;
}
