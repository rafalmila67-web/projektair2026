#pragma once
#include "Utils.hpp"
#include <vector>
#include <string>

inline sf::RectangleShape makeRect(sf::Vector2f pos, sf::Vector2f size, sf::Color fill, float outline=0, sf::Color outCol=sf::Color::White){
    sf::RectangleShape r(size);
    r.setPosition(pos); r.setFillColor(fill);
    if(outline>0){ r.setOutlineThickness(outline); r.setOutlineColor(outCol); }
    return r;
}

inline sf::Font* loadFont(){
    static sf::Font f;
    static bool loaded=false;
    if(loaded) return &f;
    const char* paths[]={"C:/Windows/Fonts/arial.ttf", "arial.ttf", nullptr};
    for(int i=0;paths[i];i++) {
        if(f.loadFromFile(paths[i])){ loaded=true; return &f; }
    }
    return nullptr;
}

inline void drawText(sf::RenderWindow& w, const std::wstring& s, sf::Vector2f p, unsigned int sz=18, sf::Color c=sf::Color::White, bool center=false){
    sf::Font* f = loadFont();
    if(!f) return;
    sf::Text t(s, *f, sz); t.setFillColor(c);
    if(center){
        auto b = t.getLocalBounds();
        t.setOrigin(b.left + b.width/2.f, b.top + b.height/2.f);
    }
    t.setPosition(p); w.draw(t);
}

inline void applyTolerantMask(sf::Image& image) {
    sf::Vector2u size = image.getSize();
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            sf::Color c = image.getPixel(x, y);
            if (c.r < 40 && c.g < 40 && c.b < 40) {
                image.setPixel(x, y, sf::Color::Transparent);
            }
        }
    }
}

struct Particle { sf::Vector2f pos, vel; sf::Color color; float life, maxLife, size; };
inline std::vector<Particle> particles;

struct Lightning { sf::Vector2f a, b; float life; };
inline std::vector<Lightning> lightnings;

inline void spawnParticles(sf::Vector2f p, sf::Color c, int n=12, float speed=80.f, float sizeMult=1.f){
    for(int i=0;i<n;i++){
        float angle = randFloat(0.f, 6.2832f);
        float spd   = randFloat(speed*0.4f, speed);
        particles.push_back({
            p, sf::Vector2f(std::cos(angle)*spd, std::sin(angle)*spd), c,
            randFloat(0.4f,0.9f), randFloat(0.4f,0.9f), randFloat(3.f,7.f) * sizeMult
        });
    }
}

inline void updateParticles(float dt){
    for(auto& p : particles){ p.pos += p.vel*dt; p.vel *= 0.9f; p.life -= dt; }
    particles.erase(std::remove_if(particles.begin(),particles.end(), [](const Particle& p){ return p.life<=0; }), particles.end());
    for(auto& l : lightnings) l.life -= dt;
    lightnings.erase(std::remove_if(lightnings.begin(),lightnings.end(), [](const Lightning& l){ return l.life<=0; }), lightnings.end());
}

inline void drawParticles(sf::RenderWindow& w){
    sf::CircleShape cs;
    for(auto& p : particles){
        float a = p.life/p.maxLife;
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(255*a);
        cs.setRadius(p.size*a); cs.setOrigin(p.size*a, p.size*a);
        cs.setPosition(p.pos); cs.setFillColor(c); w.draw(cs);
    }
    for(auto& l : lightnings){
        float a = l.life / 0.2f;
        sf::VertexArray line(sf::Lines, 2);
        line[0].position = l.a; line[0].color = sf::Color(100,200,255,static_cast<std::uint8_t>(255*a));
        line[1].position = l.b; line[1].color = sf::Color(200,255,255,static_cast<std::uint8_t>(255*a));
        w.draw(line);
        line[0].color.a /= 3; line[1].color.a /= 3;
        sf::Vector2f dir = normalize(l.b - l.a); sf::Vector2f perp(-dir.y, dir.x);
        line[0].position = l.a + perp*2.f; line[1].position = l.b + perp*2.f; w.draw(line);
        line[0].position = l.a - perp*2.f; line[1].position = l.b - perp*2.f; w.draw(line);
    }
}

struct FloatText { sf::Vector2f pos; std::wstring txt; sf::Color col; float life; };
inline std::vector<FloatText> floatTexts;
inline void addFloat(sf::Vector2f p, const std::wstring& t, sf::Color c=sf::Color(255,255,100), float l=1.2f){ floatTexts.push_back({p,t,c,l}); }