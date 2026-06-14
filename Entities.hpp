#pragma once
#include "Utils.hpp"
#include "Map.hpp"
#include "Visuals.hpp"
#include <vector>
#include <algorithm>
#include <iostream>

struct Bullet {
    sf::Vector2f pos, vel;
    int   targetId, damage;
    float radius;
    sf::Color color;
    float life = 3.f;
    bool  explosive;
    float splashRadius;
    int   level = 1;
    bool  instakill = false;
    int   pierce = 0;
    std::vector<int> hitTargets;
    bool  stun = false;
};
inline std::vector<Bullet> bullets;

enum class EnemyKind { NORMAL, ARMORED, FAST, BOSS, MEDIC, SLIME, SLIME_SMALL };

struct Enemy {
    int        id;
    EnemyKind  kind;
    float      pathDist, speed;
    int        health, maxHealth, reward, damage, armor = 0;
    sf::Color  color;
    float      radius;
    bool       alive = true;
    float      flashTimer = 0.f;
    float      healTimer = 0.f;
    float      stunTimer = 0.f;
    float      bossFreezeTimer = 4.0f;
    float      attackCooldown = 0.f;
    float      animTimer = 0.f;
    int        currentFrame = 0;
    float      attackAnimTimer = 0.f;

    sf::Vector2f pos() const { return posOnPath(pathDist); }
    bool reachedEnd() const { return pathDist >= pathLength(); }

    static Enemy make(EnemyKind k, int wave, int serial, float diffMult){
        Enemy e;
        e.id = wave*1000+serial; e.kind = k; e.pathDist = -randFloat(0.f,60.f);
        float waveScale = 1.f + wave*0.15f;
        float finalScale = waveScale * diffMult;

        switch(k){
        case EnemyKind::NORMAL: e.speed=110.f; e.health=e.maxHealth=int(80*finalScale); e.reward=20; e.damage=5; e.color=sf::Color(220,180,180); e.radius=14.f; break;
        case EnemyKind::ARMORED: e.speed=65.f; e.health=e.maxHealth=int(220*finalScale); e.reward=50; e.damage=12; e.color=sf::Color(160,160,160); e.radius=18.f; e.armor=15; break;
        case EnemyKind::FAST: e.speed=210.f; e.health=e.maxHealth=int(45*finalScale); e.reward=30; e.damage=3; e.color=sf::Color(150,220,255); e.radius=10.f; break;
        case EnemyKind::BOSS: e.speed=45.f; e.health=e.maxHealth=int(1200*finalScale); e.reward=200; e.damage=35; e.color=sf::Color(255,150,255); e.radius=28.f; e.armor=30; break;
        case EnemyKind::MEDIC: e.speed=70.f; e.health=e.maxHealth=int(180*finalScale); e.reward=60; e.damage=8; e.color=sf::Color(255,255,200); e.radius=14.f; e.armor=5; break;
        case EnemyKind::SLIME: e.speed=50.f; e.health=e.maxHealth=int(260*finalScale); e.reward=40; e.damage=10; e.color=sf::Color(100,255,100); e.radius=20.f; break;
        case EnemyKind::SLIME_SMALL: e.speed=230.f; e.health=e.maxHealth=int(25*finalScale); e.reward=10; e.damage=2; e.color=sf::Color(100,255,100); e.radius=8.f; break;
        }
        return e;
    }

    void takeDamage(int dmg){
        dmg = std::max(1, dmg - armor);
        health -= dmg; flashTimer = 0.12f;
        if(health<=0){ health=0; alive=false; }
    }

    void updateTimers(float dt){
        if(flashTimer>0.f) flashTimer-=dt;
        if(stunTimer>0.f) stunTimer-=dt;
        if(attackAnimTimer > 0.f) attackAnimTimer-=dt;

        if (stunTimer <= 0.f) {
            animTimer += dt;
            float frameSpeed = 0.15f * (100.f / std::max(50.f, speed));
            if (animTimer > frameSpeed) {
                animTimer = 0.f; currentFrame++;
            }
        }
    }

    void draw(sf::RenderWindow& w) const {
        if(!alive) return;
        sf::Vector2f p = pos();
        sf::Vector2f nextP = posOnPath(pathDist + 5.f);

        sf::CircleShape shadow(radius*1.1f); shadow.setOrigin(radius*1.1f, radius*1.1f);
        shadow.setPosition(p.x+3, p.y+10); shadow.setFillColor(sf::Color(0,0,0,80));
        shadow.setScale(1.f, 0.4f); w.draw(shadow);

        bool drawnSprite = false;
        static sf::Texture goblinWalkTex, goblinAttackTex;
        static sf::Texture mushroomWalkTex, mushroomAttackTex;
        static sf::Texture magmaWalkTex, magmaAttackTex;
        static bool texturesLoaded = false;

        if (!texturesLoaded) {
            sf::Image wImg, aImg;

            // ŚCIEŻKA WZGLĘDNA - zadziała na każdym PC, jeśli folder "InGamePhotos" będzie obok pliku .exe
            std::string path = "InGamePhotos/";

            if (!wImg.loadFromFile(path + "goblin_walk.jpg")) std::cout << "BLAD: Nie znaleziono goblin_walk.jpg!\n";
            if (!aImg.loadFromFile(path + "goblin_attack.jpg")) std::cout << "BLAD: Nie znaleziono goblin_attack.jpg!\n";
            if (!mushroomWalkTex.loadFromFile(path + "Mushroom-Run.png")) std::cout << "BLAD: Nie znaleziono Mushroom-Run.png!\n";
            if (!mushroomAttackTex.loadFromFile(path + "Mushroom-Hit.png")) std::cout << "BLAD: Nie znaleziono Mushroom-Hit.png!\n";
            if (!magmaWalkTex.loadFromFile(path + "Slime3_Idle_body.png")) std::cout << "BLAD: Nie znaleziono Slime3_Idle_body.png!\n";
            if (!magmaAttackTex.loadFromFile(path + "Slime3_Attack_body.png")) std::cout << "BLAD: Nie znaleziono Slime3_Attack_body.png!\n";

            if (wImg.getSize().x > 0 && aImg.getSize().x > 0) {
                applyTolerantMask(wImg); applyTolerantMask(aImg);
                goblinWalkTex.loadFromImage(wImg); goblinAttackTex.loadFromImage(aImg);
            }
            texturesLoaded = true;
        }

        sf::Texture* texToUse = nullptr;
        int cols = 1, rows = 1, rowToUse = 0;
        float scaleMult = 4.5f;
        bool isAttacking = (attackAnimTimer > 0.f);

        if (kind == EnemyKind::NORMAL || kind == EnemyKind::FAST) {
            texToUse = isAttacking ? &goblinAttackTex : &goblinWalkTex;
            cols = isAttacking ? 8 : 6; rows = 4; rowToUse = 2; scaleMult = 4.5f;
        } else if (kind == EnemyKind::MEDIC || kind == EnemyKind::SLIME_SMALL) {
            texToUse = isAttacking ? &mushroomAttackTex : &mushroomWalkTex;
            cols = isAttacking ? 5 : 8; rows = 1; rowToUse = 0; scaleMult = 3.5f;
        } else {
            texToUse = isAttacking ? &magmaAttackTex : &magmaWalkTex;
            cols = isAttacking ? 9 : 6; rows = 4; rowToUse = 2; scaleMult = 4.5f;
        }

        if (texToUse && texToUse->getSize().x > 0) {
            int frameW = texToUse->getSize().x / cols;
            int frameH = texToUse->getSize().y / rows;

            int f = 0;
            if (isAttacking) {
                float progress = 1.0f - (attackAnimTimer / 0.4f);
                f = std::clamp((int)(progress * cols), 0, cols - 1);
            } else { f = currentFrame % cols; }

            sf::Sprite sprite(*texToUse);
            sprite.setTextureRect(sf::IntRect(f * frameW, rowToUse * frameH, frameW, frameH));
            sprite.setOrigin(frameW / 2.f, frameH / 2.f);
            sprite.setPosition(p.x, p.y - radius * 0.8f);

            sf::Color finalColor = flashTimer > 0.f ? sf::Color(255,100,100) : sf::Color::White;
            if (stunTimer > 0.f) finalColor = sf::Color(200, 200, 100);
            sprite.setColor(finalColor);

            float scaleBase = (radius * scaleMult) / frameH;
            if (nextP.x > p.x) sprite.setScale(scaleBase, scaleBase);
            else sprite.setScale(-scaleBase, scaleBase);

            w.draw(sprite); drawnSprite = true;
        }

        if (!drawnSprite) {
            sf::CircleShape body(radius);
            body.setOrigin(radius, radius); body.setPosition(p);
            sf::Color finalColor = flashTimer>0.f ? sf::Color::White : color;
            if (stunTimer > 0.f) finalColor = sf::Color(200, 200, 100);
            body.setFillColor(finalColor);
            if(kind==EnemyKind::ARMORED || kind==EnemyKind::BOSS){ body.setOutlineThickness(3.f); body.setOutlineColor(kind==EnemyKind::BOSS?sf::Color(255,150,255):sf::Color::Yellow); }
            else if(kind==EnemyKind::MEDIC) { body.setOutlineThickness(2.f); body.setOutlineColor(sf::Color(255,255,255)); }
            w.draw(body);
        }

        float pct = float(health)/float(maxHealth);
        float bw = radius*2.5f;
        sf::RectangleShape hbg(sf::Vector2f(bw,6.f)), hfg(sf::Vector2f(bw*pct,6.f));
        hbg.setFillColor(sf::Color(60,0,0)); hbg.setPosition(p.x-bw/2, p.y-radius-25);
        hfg.setFillColor(pct>0.5f?sf::Color::Green:pct>0.2f?sf::Color::Yellow:sf::Color::Red);
        hfg.setPosition(p.x-bw/2, p.y-radius-25);
        w.draw(hbg); w.draw(hfg);
    }
};

enum class TowerKind { BASIC, BARRACKS, FROST, MINE, SNIPER, TESLA, ARTILLERY, LASER };
enum class Mutation  { NONE, PATH_A, PATH_B };

struct TowerDef {
    const wchar_t* name; const wchar_t* desc;
    int cost, upgradeCost;
    float range, fireRate, projSpeed; int damage;
    sf::Color color, projColor;
    bool explosive; float splashR; bool slows;
};

inline const TowerDef TOWER_DEFS[] = {
    {L"Strażnik",   L"Szybki, średni zasięg",    100, 80,  160, 0.7f, 350, 30,  sf::Color(70,110,210), sf::Color(200,230,255), false, 0,  false},
    {L"Koszary",    L"Wojownicy blokują wrogów", 180, 110, 150, 1.2f, 0,   15,  sf::Color(200,100,50), sf::Color(0,0,0),       false, 0,  false},
    {L"Mroźnik",    L"Spowalnia wrogów",         200, 120, 180, 1.2f, 220, 20,  sf::Color(80,180,240), sf::Color(160,220,255), false, 0,  true},
    {L"Kopalnia",   L"Generuje kasę (bez atak)", 220, 140, 0,   4.0f, 0,   0,   sf::Color(255,215,0),  sf::Color(0,0,0),       false, 0,  false},
    {L"Snajper",    L"Duży zasięg, duże dmg",    300, 160, 340, 2.2f, 700, 100, sf::Color(50,180,120), sf::Color(150,255,180), false, 0,  false},
    {L"Tesla",      L"Skaczące błyskawice",      350, 180, 180, 1.8f, 0,   60,  sf::Color(100,200,255),sf::Color(200,255,255), false, 0,  false},
    {L"Artyleria",  L"Odłamki, wolna",           450, 220, 230, 3.5f, 200, 140, sf::Color(200,120,40), sf::Color(255,200,80),  true,  80, false},
    {L"Laser",      L"Ciągły ogień",             550, 260, 200, 0.15f,900, 18,  sf::Color(200,50,200), sf::Color(255,100,255), false, 0,  false},
    };

inline std::pair<std::wstring, std::wstring> getMutationInfo(TowerKind kind) {
    switch(kind) {
    case TowerKind::BASIC: return {L"[A] Szybkostrzelny\n2x wyższa szybkostrzelność", L"[B] Niszczyciel\n2.5x więcej obrażeń"};
    case TowerKind::SNIPER: return {L"[A] Zabójca (Zasięg -30%)\n10% szans na natychmiastowe\nzabicie (nie bossów)", L"[B] Penetrator\nPocisk przebija do 3 wrogów\nw jednej linii"};
    case TowerKind::ARTILLERY: return {L"[A] Napalm\n+50% większe pole rażenia", L"[B] Ogłuszenie\nEksplozje zatrzymują\nwrogów na 1 sekundę"};
    case TowerKind::FROST: return {L"[A] Zamieć\nDużo większy zasięg wieży", L"[B] Zamarznięcie\nPociski całkowicie zatrzymują\nwroga na 0.5s"};
    case TowerKind::LASER: return {L"[A] Przegrzanie\n+50% Obrażeń Lasera", L"[B] Długi Promień\nOgromny zasięg lasera"};
    case TowerKind::MINE: return {L"[A] Bogactwo\nGeneruje 30$ zamiast 15$", L"[B] Gorączka Złota\nGeneruje złoto 2x szybciej"};
    case TowerKind::TESLA: return {L"[A] Przeciążenie\nBłyskawica przeskakuje\nna +2 dodatkowe cele", L"[B] Wysokie Napięcie\nBłyskawica ogłusza wrogów\nna 0.5 sekundy"};
    case TowerKind::BARRACKS: return {L"[A] Paladyni\nWojownicy mają +50% HP\ni znacznie szybciej wracają", L"[B] Berserkerzy\nWojownicy zadają w walce\n3x więcej obrażeń"};
    }
    return {L"",L""};
}

struct Warrior {
    sf::Vector2f pos, rallyPoint;
    int hp = 60, maxHp = 60;
    float attackCooldown = 0.f;
    float respawnTimer = 0.f;
    bool alive = true;
    float animTimer = 0.f;
    int currentFrame = 0;
    int facingDir = 1;
    float attackAnimTimer = 0.f;
};

struct Tower {
    int          id;
    TowerKind    kind;
    sf::Vector2f pos;
    int          level = 1;
    Mutation     mutation = Mutation::NONE;
    float        cooldown = 0.f;
    bool         selected = false;
    bool         firing = false;
    int          targetId = -1;
    float        aimAngle = 0.f;
    int          currentBarrel = 0;
    float        recoil = 0.f;
    bool         frozen = false;
    int          unfreezeClicks = 0;
    sf::Vector2f rallyPoint;
    std::vector<Warrior> warriors;

    const TowerDef& def() const { return TOWER_DEFS[int(kind)]; }
    float range() const {
        float r = def().range * (1.f + (level-1)*0.15f);
        if (mutation == Mutation::PATH_A && kind == TowerKind::SNIPER) r *= 0.7f;
        if (mutation == Mutation::PATH_A && kind == TowerKind::FROST)  r *= 1.5f;
        if (mutation == Mutation::PATH_B && kind == TowerKind::LASER)  r *= 1.6f;
        return r;
    }
    float fireRate() const {
        float fr = def().fireRate;
        if (kind == TowerKind::BASIC) fr /= level; else fr *= (1.f - (level-1)*0.15f);
        if (mutation == Mutation::PATH_A && kind == TowerKind::BASIC) fr *= 0.5f;
        if (mutation == Mutation::PATH_B && kind == TowerKind::MINE)  fr *= 0.5f;
        return fr;
    }
    int damage() const {
        int dmg = def().damage;
        if (kind == TowerKind::SNIPER) dmg = int(dmg * std::pow(1.8f, level-1)); else dmg = int(dmg * (1.f + (level-1)*0.3f));
        if (mutation == Mutation::PATH_B && kind == TowerKind::BASIC) dmg = int(dmg * 2.5f);
        if (mutation == Mutation::PATH_A && kind == TowerKind::LASER) dmg = int(dmg * 1.5f);
        return dmg;
    }

    void draw(sf::RenderWindow& w, bool buildMode, bool isDragged) const {
        if((selected || buildMode) && kind != TowerKind::MINE){
            sf::CircleShape rc(range());
            rc.setOrigin(range(), range()); rc.setPosition(pos);
            rc.setFillColor(sf::Color(255,255,255,10)); rc.setOutlineColor(sf::Color(255,255,255,60)); rc.setOutlineThickness(1.f);
            w.draw(rc);
        }

        sf::RectangleShape base(sf::Vector2f(42,42)); base.setOrigin(21.f, 21.f); base.setPosition(pos);
        base.setFillColor(isDragged ? sf::Color(50,50,70,200) : sf::Color(30,30,50));

        sf::Color outCol = (isDragged ? sf::Color(120,120,255) : sf::Color(80,80,100));
        if (mutation == Mutation::PATH_A) outCol = sf::Color(255, 100, 100);
        if (mutation == Mutation::PATH_B) outCol = sf::Color(100, 255, 100);
        base.setOutlineColor(outCol); base.setOutlineThickness(2.f); w.draw(base);

        sf::Vector2f rOffset(-std::cos(aimAngle * 3.14159f / 180.f) * recoil, -std::sin(aimAngle * 3.14159f / 180.f) * recoil);
        sf::Vector2f bPos = pos + rOffset;

        if (kind == TowerKind::MINE) {
            sf::CircleShape coin(12.f); coin.setOrigin(12.f, 12.f); coin.setPosition(pos);
            coin.setFillColor(sf::Color(255,215,0)); coin.setOutlineColor(sf::Color(200,150,0)); coin.setOutlineThickness(2.f);
            float pulse = 1.f + 0.1f * std::sin(cooldown * 5.f);
            if(frozen) pulse = 1.f;
            coin.setScale(pulse, pulse); w.draw(coin);
        }
        else if (kind == TowerKind::BARRACKS) {
            sf::RectangleShape roof(sf::Vector2f(30, 30)); roof.setOrigin(15.f, 15.f); roof.setPosition(pos);
            roof.setFillColor(def().color); roof.setRotation(45.f); w.draw(roof);

            if (selected) {
                sf::CircleShape flag(4.f); flag.setOrigin(4.f, 4.f); flag.setPosition(rallyPoint);
                flag.setFillColor(sf::Color::Blue); w.draw(flag);
            }

            static sf::Texture warrWalkTex, warrAttackTex;
            static bool warrTexLoaded = false;
            static bool warrAttempted = false;
            if (!warrAttempted) {
                sf::Image wImg, aImg;
                std::string path = "InGamePhotos/";

                if (!wImg.loadFromFile(path + "warrior_walk.jpg")) std::cout << "BLAD: Nie znaleziono warrior_walk.jpg!\n";
                if (!aImg.loadFromFile(path + "warrior_attack.jpg")) std::cout << "BLAD: Nie znaleziono warrior_attack.jpg!\n";

                if(wImg.getSize().x > 0 && aImg.getSize().x > 0) {
                    applyTolerantMask(wImg); applyTolerantMask(aImg);
                    warrWalkTex.loadFromImage(wImg); warrAttackTex.loadFromImage(aImg);
                    warrTexLoaded = true;
                }
                warrAttempted = true;
            }

            for(const auto& warr : warriors) {
                if (!warr.alive) continue;
                if (warrTexLoaded) {
                    bool isAttacking = (warr.attackAnimTimer > 0.f);
                    sf::Sprite sprite(isAttacking ? warrAttackTex : warrWalkTex);
                    int cols = isAttacking ? 4 : 6;
                    int frameW = sprite.getTexture()->getSize().x / cols;
                    int frameH = sprite.getTexture()->getSize().y;
                    int f = isAttacking ? std::clamp((int)((1.0f - (warr.attackAnimTimer / 0.3f)) * cols), 0, cols - 1) : warr.currentFrame % cols;

                    sprite.setTextureRect(sf::IntRect(f * frameW, 0, frameW, frameH));
                    sprite.setOrigin(frameW / 2.f, frameH / 2.f);
                    sprite.setPosition(warr.pos.x, warr.pos.y - 12.f);
                    sf::Color wColor = (mutation == Mutation::PATH_A) ? sf::Color(255, 230, 100) : sf::Color::White;
                    sprite.setColor(wColor);
                    float scaleBase = 52.f / frameH;
                    sprite.setScale(warr.facingDir * scaleBase, scaleBase);
                    w.draw(sprite);
                } else {
                    sf::CircleShape c(5.f); c.setOrigin(5.f, 5.f); c.setPosition(warr.pos);
                    c.setFillColor(mutation == Mutation::PATH_A ? sf::Color(255, 215, 0) : sf::Color(200, 100, 50));
                    c.setOutlineThickness(1.f); c.setOutlineColor(sf::Color::White); w.draw(c);
                }
                float pct = std::max(0.f, (float)warr.hp / warr.maxHp);
                sf::RectangleShape wbg(sf::Vector2f(14.f, 3.f)); wbg.setPosition(warr.pos.x - 7.f, warr.pos.y - 32.f); wbg.setFillColor(sf::Color::Red);
                sf::RectangleShape wfg(sf::Vector2f(14.f * pct, 3.f)); wfg.setPosition(warr.pos.x - 7.f, warr.pos.y - 32.f); wfg.setFillColor(sf::Color::Green);
                w.draw(wbg); w.draw(wfg);
            }
        }
        else if (kind == TowerKind::TESLA) {
            sf::CircleShape dome(12.f + level*2.f); dome.setOrigin(12.f + level*2.f, 12.f + level*2.f);
            dome.setPosition(pos); dome.setFillColor(def().color);
            dome.setOutlineThickness(2.f); dome.setOutlineColor(sf::Color(255,255,255, 150)); w.draw(dome);
            sf::RectangleShape ant(sf::Vector2f(4, 16)); ant.setOrigin(2.f, 16.f); ant.setPosition(pos);
            ant.setFillColor(sf::Color(200,255,255)); w.draw(ant);
        }
        else if (kind == TowerKind::BASIC) {
            for(int i = 0; i < level; i++) {
                sf::RectangleShape barrel(sf::Vector2f(28, 6)); barrel.setOrigin(4.f, 3.f);
                float offset = 0.f;
                if (level == 2) offset = (i == 0) ? -5.f : 5.f;
                if (level == 3) offset = (i == 0) ? -7.f : (i == 1 ? 0.f : 7.f);
                sf::Vector2f bPosOff = bPos;
                float perpAngle = aimAngle + 90.f;
                bPosOff.x += std::cos(perpAngle * 3.14159f / 180.f) * offset; bPosOff.y += std::sin(perpAngle * 3.14159f / 180.f) * offset;
                barrel.setPosition(bPosOff); barrel.setFillColor(def().color); barrel.setRotation(aimAngle); w.draw(barrel);
            }
        }
        else if (kind == TowerKind::SNIPER) {
            float bLen = 28.f + (level-1)*16.f; float bThick = 8.f + (level-1)*4.f;
            sf::RectangleShape barrel(sf::Vector2f(bLen, bThick)); barrel.setOrigin(4.f, bThick/2.f);
            barrel.setPosition(bPos); barrel.setFillColor(def().color); barrel.setRotation(aimAngle); w.draw(barrel);
        }
        else if (kind == TowerKind::ARTILLERY) {
            float bThick = 14.f + (level-1)*10.f;
            sf::RectangleShape barrel(sf::Vector2f(28, bThick)); barrel.setOrigin(4.f, bThick/2.f);
            barrel.setPosition(bPos); barrel.setFillColor(def().color); barrel.setRotation(aimAngle); w.draw(barrel);
        }
        else if (kind == TowerKind::FROST) {
            for(int i = 0; i < level; i++) {
                sf::RectangleShape barrel(sf::Vector2f(26.f + i*3.f, 4.f)); barrel.setOrigin(-4.f - i*2.f, 2.f);
                barrel.setPosition(bPos); barrel.setFillColor(sf::Color(180, 240, 255, 200)); barrel.setRotation(aimAngle + (i - (level-1)/2.f)*20.f); w.draw(barrel);
            }
        }
        else {
            sf::RectangleShape barrel(sf::Vector2f(28, 8)); barrel.setOrigin(4.f, 4.f);
            barrel.setPosition(bPos); barrel.setFillColor(def().color); barrel.setRotation(aimAngle); w.draw(barrel);
        }

        if (kind != TowerKind::MINE && kind != TowerKind::TESLA && kind != TowerKind::BARRACKS) {
            sf::CircleShape head(10.f); head.setOrigin(10.f, 10.f); head.setPosition(pos);
            head.setFillColor(def().color); head.setOutlineColor(sf::Color(255,255,255,120)); head.setOutlineThickness(2.f); w.draw(head);
        }

        if(level>1){
            int stars = level - 1; float startX = pos.x - (stars - 1) * 4.5f;
            for(int i=0; i<stars; i++){
                sf::CircleShape star(3.5f); star.setOrigin(3.5f, 3.5f); star.setPosition(startX + i*9.f, pos.y+18.f);
                sf::Color sCol = sf::Color::Yellow;
                if(i == 1) { if(mutation == Mutation::PATH_A) sCol = sf::Color(255, 100, 100); if(mutation == Mutation::PATH_B) sCol = sf::Color(100, 255, 100); }
                star.setFillColor(sCol); w.draw(star);
            }
        }

        if (frozen) {
            sf::RectangleShape ice(sf::Vector2f(46.f, 46.f)); ice.setOrigin(23.f, 23.f);
            ice.setPosition(pos); ice.setFillColor(sf::Color(160, 220, 255, 180));
            ice.setOutlineThickness(2.f); ice.setOutlineColor(sf::Color(200, 240, 255, 220)); w.draw(ice);
            drawText(w, std::to_wstring(unfreezeClicks), sf::Vector2f(pos.x, pos.y - 8.f), 22, sf::Color::Red, true);
        }
    }
};