#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <vector>

#include "header.hpp"

typedef std::vector<Point> vP;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Squared distance: no sqrt, good enough when we only need to compare distances.
static float distSq(const Point& a, const Point& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Every algorithm returns a closed loop: the first point is repeated at the end.
static vP closeLoop(vP route) {
    if (!route.empty()) { route.push_back(route.front()); }
    return route;
}

// ---------------------------------------------------------------------------
// Nearest neighbour (best result over every possible starting point)
// ---------------------------------------------------------------------------

vP nearestNeighbour(vP& points) {
    int n = points.size();
    if (n < 3) { return closeLoop(points); }

    float bestLength = FLT_MAX;
    std::vector<int> bestOrder;
    std::vector<int> order;
    std::vector<char> visited(n);

    for (int start = 0; start < n; start++) {
        std::fill(visited.begin(), visited.end(), 0);
        order.clear();
        order.push_back(start);
        visited[start] = 1;

        int current = start;
        float length = 0;

        while (static_cast<int>(order.size()) < n) {
            // find the closest unvisited point
            int next = -1;
            float nextDist = FLT_MAX;
            for (int i = 0; i < n; i++) {
                if (visited[i]) { continue; }
                float d = distSq(points[current], points[i]);
                if (d < nextDist) {
                    nextDist = d;
                    next = i;
                }
            }
            length += std::sqrt(nextDist);
            if (length >= bestLength) { break; } // already worse than the best route, give up on this start

            visited[next] = 1;
            order.push_back(next);
            current = next;
        }

        if (static_cast<int>(order.size()) < n) { continue; } // abandoned above

        length += findDistance(points[current], points[start]); // back to the start
        if (length < bestLength) {
            bestLength = length;
            bestOrder = order;
        }
    }

    vP route;
    route.reserve(n + 1);
    for (int idx : bestOrder) { route.push_back(points[idx]); }
    return closeLoop(route);
}

// ---------------------------------------------------------------------------
// Brute force (only sensible for small n)
// ---------------------------------------------------------------------------

vP bruteForce(vP& points) {
    int n = points.size();
    if (n < 3) { return closeLoop(points); }

    // distance lookup table so we never recompute a distance
    std::vector<float> dist(n * n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) { dist[i * n + j] = findDistance(points[i], points[j]); }
    }

    // point 0 is always the start, so only the other n-1 points are permuted
    std::vector<int> order(n - 1);
    std::iota(order.begin(), order.end(), 1);

    float bestLength = FLT_MAX;
    std::vector<int> bestOrder = order;

    do {
        // a route and its mirror image have the same length, so only check one of each pair
        if (order.front() > order.back()) { continue; }

        float length = dist[order.front()]; // 0 -> first
        for (int i = 1; i < n - 1 && length < bestLength; i++) { length += dist[order[i - 1] * n + order[i]]; }
        length += dist[order.back() * n]; // last -> 0

        if (length < bestLength) {
            bestLength = length;
            bestOrder = order;
        }
    }
    while (std::next_permutation(order.begin(), order.end()));

    vP route;
    route.reserve(n + 1);
    route.push_back(points[0]);
    for (int idx : bestOrder) { route.push_back(points[idx]); }
    return closeLoop(route);
}

// ---------------------------------------------------------------------------
// Insertion heuristics (cheapest insertion and convex hull insertion share this)
// ---------------------------------------------------------------------------

// Takes a small starting loop and the points still to be placed, and inserts the
// remaining points one at a time.
//
// For every remaining point we remember the cheapest edge to insert it into
// (bestPos / bestCost). After an insertion only two edges are new, so most points
// just compare against those two instead of rescanning the whole route.
// That makes this O(n^2) instead of O(n^3).
//
// useRatio = false: insert the point with the smallest extra distance (cheapest insertion)
// useRatio = true:  insert the point with the smallest (d(a,p) + d(p,b)) / d(a,b) (convex hull insertion)
static vP insertRemaining(vP route, vP remaining, bool useRatio) {
    int m = remaining.size();
    std::vector<float> bestCost(m);
    std::vector<int> bestPos(m); // the point goes between route[bestPos] and the next one

    // extra distance if point p is inserted into the edge starting at route[j]
    auto extraCost = [&](const Point& p, int j) {
        const Point& a = route[j];
        const Point& b = route[(j + 1) % route.size()];
        return findDistance(p, a) + findDistance(p, b) - findDistance(a, b);
    };

    // check every edge for remaining[i]
    auto scanAllEdges = [&](int i) {
        bestCost[i] = FLT_MAX;
        for (int j = 0; j < static_cast<int>(route.size()); j++) {
            float cost = extraCost(remaining[i], j);
            if (cost < bestCost[i]) {
                bestCost[i] = cost;
                bestPos[i] = j;
            }
        }
    };

    for (int i = 0; i < m; i++) { scanAllEdges(i); }

    while (!remaining.empty()) {
        // choose which point to insert next
        int pick = 0;
        float pickScore = FLT_MAX;
        for (int i = 0; i < static_cast<int>(remaining.size()); i++) {
            float score = bestCost[i];
            if (useRatio) {
                const Point& a = route[bestPos[i]];
                const Point& b = route[(bestPos[i] + 1) % route.size()];
                float edge = findDistance(a, b);
                score = (bestCost[i] + edge) / std::max(edge, 1e-6f);
            }
            if (score < pickScore) {
                pickScore = score;
                pick = i;
            }
        }

        int pos = bestPos[pick];
        route.insert(route.begin() + pos + 1, remaining[pick]);

        // remove the picked point (swap with the last one, then pop)
        int last = remaining.size() - 1;
        remaining[pick] = remaining[last];
        bestCost[pick] = bestCost[last];
        bestPos[pick] = bestPos[last];
        remaining.pop_back();
        bestCost.pop_back();
        bestPos.pop_back();

        // edge `pos` was split into two new edges: `pos` and `pos + 1`.
        // every edge after it has moved along by one.
        for (int i = 0; i < static_cast<int>(remaining.size()); i++) {
            if (bestPos[i] == pos) {
                scanAllEdges(i); // its best edge no longer exists
                continue;
            }
            if (bestPos[i] > pos) { bestPos[i]++; }

            float c1 = extraCost(remaining[i], pos);
            if (c1 < bestCost[i]) {
                bestCost[i] = c1;
                bestPos[i] = pos;
            }
            float c2 = extraCost(remaining[i], pos + 1);
            if (c2 < bestCost[i]) {
                bestCost[i] = c2;
                bestPos[i] = pos + 1;
            }
        }
    }

    return closeLoop(route);
}

// ---------------------------------------------------------------------------
// Cheapest insertion
// ---------------------------------------------------------------------------

vP cheapestInsertion(vP& points) {
    int n = points.size();
    if (n < 3) { return closeLoop(points); }

    // start with the two closest points
    int bestI = 0;
    int bestJ = 1;
    float best = FLT_MAX;
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            float d = distSq(points[i], points[j]);
            if (d < best) {
                best = d;
                bestI = i;
                bestJ = j;
            }
        }
    }

    vP route = {points[bestI], points[bestJ]};
    vP remaining;
    for (int i = 0; i < n; i++) {
        if (i != bestI && i != bestJ) { remaining.push_back(points[i]); }
    }
    return insertRemaining(route, remaining, false);
}

// ---------------------------------------------------------------------------
// Convex hull insertion
// ---------------------------------------------------------------------------

// > 0 if o -> a -> b turns left, < 0 if it turns right, 0 if they are in a line
static float cross(const Point& o, const Point& a, const Point& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// Andrew's monotone chain. Returns the indices of the hull points in order.
static std::vector<int> convexHull(const vP& points) {
    int n = points.size();
    std::vector<int> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        if (points[a].x != points[b].x) { return points[a].x < points[b].x; }
        return points[a].y < points[b].y;
    });

    std::vector<int> hull(2 * n);
    int k = 0;
    for (int i = 0; i < n; i++) { // lower hull
        while (k >= 2 && cross(points[hull[k - 2]], points[hull[k - 1]], points[idx[i]]) <= 0) { k--; }
        hull[k++] = idx[i];
    }
    for (int i = n - 2, lowerSize = k + 1; i >= 0; i--) { // upper hull
        while (k >= lowerSize && cross(points[hull[k - 2]], points[hull[k - 1]], points[idx[i]]) <= 0) { k--; }
        hull[k++] = idx[i];
    }
    hull.resize(k - 1); // last point is the same as the first
    return hull;
}

vP convexHullInsertion(vP& points) {
    int n = points.size();
    if (n < 3) { return closeLoop(points); }

    // start with the convex hull as the route
    std::vector<char> onHull(n, 0);
    vP route;
    for (int idx : convexHull(points)) {
        onHull[idx] = 1;
        route.push_back(points[idx]);
    }

    // everything else gets inserted afterwards
    vP remaining;
    for (int i = 0; i < n; i++) {
        if (!onHull[i]) { remaining.push_back(points[i]); }
    }
    return insertRemaining(route, remaining, true);
}

// ---------------------------------------------------------------------------
// Nearest merge
// ---------------------------------------------------------------------------

// Joins two loops (lists of point indices) into one, using the cheapest way:
// cut one edge in each loop and reconnect the four ends (either way round).
static std::vector<int> mergeLoops(const vP& points, const std::vector<int>& A, const std::vector<int>& B) {
    int p = A.size();
    int q = B.size();

    float bestCost = FLT_MAX;
    int bestI = 0;
    int bestJ = 0;
    bool reversed = false;

    for (int i = 0; i < p; i++) {
        const Point& a1 = points[A[i]];
        const Point& a2 = points[A[(i + 1) % p]];
        float aEdge = findDistance(a1, a2);
        for (int j = 0; j < q; j++) {
            const Point& b1 = points[B[j]];
            const Point& b2 = points[B[(j + 1) % q]];
            float removed = aEdge + findDistance(b1, b2);

            float forward = findDistance(a1, b2) + findDistance(b1, a2) - removed; // a1 -> b2 ... b1 -> a2
            float backward = findDistance(a1, b1) + findDistance(b2, a2) - removed; // a1 -> b1 ... b2 -> a2

            if (forward < bestCost) {
                bestCost = forward;
                bestI = i;
                bestJ = j;
                reversed = false;
            }
            if (backward < bestCost) {
                bestCost = backward;
                bestI = i;
                bestJ = j;
                reversed = true;
            }
        }
    }

    std::vector<int> merged;
    merged.reserve(p + q);
    for (int k = 0; k < p; k++) { merged.push_back(A[(bestI + 1 + k) % p]); } // ends at A[bestI]
    if (!reversed) {
        for (int k = 0; k < q; k++) { merged.push_back(B[(bestJ + 1 + k) % q]); } // B[bestJ + 1] ... B[bestJ]
    }
    else {
        for (int k = 0; k < q; k++) { merged.push_back(B[(bestJ - k + q) % q]); } // B[bestJ] ... B[bestJ + 1]
    }
    return merged;
}

// Every point starts as its own tiny loop. Repeatedly take the two closest loops
// (smallest distance between any point in one and any point in the other) and merge
// them the cheapest way, until only one loop is left.
//
// Looking at every pair of points from closest to furthest and skipping pairs that
// are already in the same loop finds "the two closest loops" without rescanning.
vP nearestMerge(vP& points) {
    int n = points.size();
    if (n < 3) { return closeLoop(points); }

    struct Pair {
        int a;
        int b;
        float dist;
    };
    std::vector<Pair> pairs;
    pairs.reserve(n * (n - 1) / 2);
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) { pairs.push_back({i, j, distSq(points[i], points[j])}); }
    }
    std::sort(pairs.begin(), pairs.end(), [](const Pair& x, const Pair& y) { return x.dist < y.dist; });

    std::vector<std::vector<int>> loops(n); // loops[t] = point indices of loop t in order
    std::vector<int> loopOf(n);             // which loop each point is in
    for (int i = 0; i < n; i++) {
        loops[i] = {i};
        loopOf[i] = i;
    }

    int loopCount = n;
    for (const Pair& pair : pairs) {
        if (loopCount == 1) { break; }
        int la = loopOf[pair.a];
        int lb = loopOf[pair.b];
        if (la == lb) { continue; }

        loops[la] = mergeLoops(points, loops[la], loops[lb]);
        for (int idx : loops[lb]) { loopOf[idx] = la; }
        loops[lb].clear();
        loopCount--;
    }

    vP route;
    route.reserve(n + 1);
    for (int idx : loops[loopOf[0]]) { route.push_back(points[idx]); }
    return closeLoop(route);
}
