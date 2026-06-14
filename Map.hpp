#pragma once
#include "Utils.hpp"
#include <vector>
#include <algorithm>

inline const std::vector<sf::Vector2f> PATH_WP = {
    sf::Vector2f(-30, 200), sf::Vector2f(200, 200), sf::Vector2f(200, 400), sf::Vector2f(520, 400),
    sf::Vector2f(520, 160), sf::Vector2f(840, 160), sf::Vector2f(840, 480), sf::Vector2f(1120, 480),
    sf::Vector2f(1120, 240), sf::Vector2f(1400, 240), sf::Vector2f(1400, 680), sf::Vector2f(920, 680),
    sf::Vector2f(920, 820), sf::Vector2f(440, 820), sf::Vector2f(440, 580), sf::Vector2f(120, 580),
    sf::Vector2f(120, 820), sf::Vector2f(-30, 820)
};

inline float pathLength(){
    static float cached = -1.f;
    if(cached>0.f) return cached;
    cached=0.f;
    for(size_t i=1;i<PATH_WP.size();i++) cached+=dist(PATH_WP[i-1],PATH_WP[i]);
    return cached;
}

inline sf::Vector2f posOnPath(float d){
    float rem = d;
    for(size_t i=1;i<PATH_WP.size();i++){
        float seg = dist(PATH_WP[i-1],PATH_WP[i]);
        if(rem<=seg){ float t = rem/seg; return lerp(PATH_WP[i-1],PATH_WP[i],t); }
        rem -= seg;
    }
    return PATH_WP.back();
}

inline sf::Vector2f getClosestPathPoint(sf::Vector2f p) {
    sf::Vector2f bestP = p; float bestD = 1e9f;
    for(size_t i=1; i<PATH_WP.size(); i++){
        sf::Vector2f a = PATH_WP[i-1], b = PATH_WP[i];
        sf::Vector2f ab = b - a, ap = p - a;
        float t = (ab.x*ap.x + ab.y*ap.y)/(ab.x*ab.x + ab.y*ab.y + 0.001f);
        t = std::clamp(t, 0.f, 1.f);
        sf::Vector2f proj = a + ab * t;
        float d = dist(p, proj);
        if(d < bestD) { bestD = d; bestP = proj; }
    }
    return bestP;
}