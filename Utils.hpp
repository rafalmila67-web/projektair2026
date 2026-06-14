#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <random>

inline constexpr unsigned int WIN_W  = 1600;
inline constexpr unsigned int WIN_H  = 900;
inline constexpr float        TILE   = 40.f;

inline std::mt19937 rng{ std::random_device{}() };
inline int   randInt  (int   a, int   b){ return std::uniform_int_distribution<int>(a,b)(rng); }
inline float randFloat(float a, float b){ return std::uniform_real_distribution<float>(a,b)(rng); }

inline float dist(sf::Vector2f a, sf::Vector2f b){ return std::hypot(b.x-a.x, b.y-a.y); }

inline sf::Vector2f normalize(sf::Vector2f v){
    float len = std::hypot(v.x, v.y);
    return len>0.f ? sf::Vector2f(v.x/len, v.y/len) : sf::Vector2f(0.f, 0.f);
}

inline sf::Vector2f lerp(sf::Vector2f a, sf::Vector2f b, float t){
    return sf::Vector2f(a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t);
}