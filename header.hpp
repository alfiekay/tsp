#ifndef HEADER
#define HEADER

#include <vector>

struct Point {
    float x;
    float y;
};

// fns.cpp
float findDistance(Point p1, Point p2);
std::vector<Point> generatePoints(int num);

// algorithms.cpp
std::vector<Point> nearestNeighbour(std::vector<Point>& points);
std::vector<Point> bruteForce(std::vector<Point>& points);
std::vector<Point> cheapestInsertion(std::vector<Point>& points);

#endif
