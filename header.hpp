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

// algorithms.cpp (all return a closed loop: the first point is repeated at the end)
std::vector<Point> nearestNeighbour(std::vector<Point>& points);
std::vector<Point> bruteForce(std::vector<Point>& points);
std::vector<Point> cheapestInsertion(std::vector<Point>& points);
std::vector<Point> convexHullInsertion(std::vector<Point>& points);
std::vector<Point> nearestMerge(std::vector<Point>& points);

#endif
