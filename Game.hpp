#pragma once
#include "Entities.hpp"

enum class Phase { MENU, BUILD, COMBAT, GAME_OVER, WIN, MUTATION_SELECT };
enum class Difficulty { EASY, NORMAL, HARD };

struct Wave { int normal, armored, fast, boss, medic, slime; };
inline const Wave WAVES[] = {
    {8,0,0,0,0,0}, {10,2,0,0,0,0}, {12,2,4,0,1,0}, {10,4,4,0,2,2}, {12,4,6,1,2,3},
    {14,6,6,1,3,4},{16,8,8,2,4,4},{20,10,10,2,5,5},{25,12,12,3,6,6},{30,15,15,4,8,8}
};
inline constexpr int TOTAL_WAVES = 10;

struct Game {
    Phase   phase     = Phase::MENU;
    Phase   prevPhase = Phase::MENU;
    Difficulty difficulty = Difficulty::NORMAL;
    float   diffMult  = 1.0f;
    int     wave      = 0, money = 600, lives = 20, score = 0, kills = 0;
    float   buildTime = 0.f, spawnTimer= 0.f;
    int     toSpawn   = 0, spawnSerial = 0;
    float   damageFlashTimer = 0.f;
    float   screenShake = 0.f;

    float   spellMeteorCD = 0.f;
    float   spellFreezeCD = 0.f;
    bool    targetingMeteor = false;

    TowerKind    selectedKind = TowerKind::BASIC;
    sf::Vector2f mouseWorld   = sf::Vector2f(0.f,0.f);

    static constexpr int MAX_LEVEL = 3;
    int draggedTowerId = -1;
    sf::Vector2f dragStartPos = sf::Vector2f(0.f, 0.f);

    int mutSourceTowerId = -1;
    int mutTargetTowerId = -1;

    std::vector<Enemy>  enemies;
    std::vector<Tower>  towers;
    int nextTowerId = 1;

    void startGame(Difficulty d) {
        difficulty = d;
        phase = Phase::BUILD; wave = 0; score = 0; kills = 0;
        enemies.clear(); towers.clear(); bullets.clear(); particles.clear(); floatTexts.clear(); lightnings.clear();
        damageFlashTimer = 0.f; screenShake = 0.f; spellMeteorCD = 0.f; spellFreezeCD = 0.f; targetingMeteor = false;
        if (d == Difficulty::EASY) { money = 800; lives = 30; diffMult = 0.75f; }
        else if (d == Difficulty::NORMAL) { money = 600; lives = 20; diffMult = 1.0f; }
        else if (d == Difficulty::HARD) { money = 400; lives = 10; diffMult = 1.35f; }
    }

    int buyCost() const { return TOWER_DEFS[int(selectedKind)].cost; }
    bool canAfford() const { return money >= buyCost(); }

    bool onPath(sf::Vector2f p) const {
        for(size_t i=1;i<PATH_WP.size();i++){
            sf::Vector2f a=PATH_WP[i-1], b=PATH_WP[i];
            sf::Vector2f ab=b-a, ap=p-a;
            float t = (ab.x*ap.x+ab.y*ap.y)/(ab.x*ab.x+ab.y*ab.y+0.001f); t = std::clamp(t,0.f,1.f);
            if(dist(sf::Vector2f(a.x+ab.x*t, a.y+ab.y*t), p)<32.f) return true;
        } return false;
    }
    bool towersOverlap(sf::Vector2f p) const { for(auto& t:towers) if(dist(t.pos,p)<38.f) return true; return false; }
    bool canPlace(sf::Vector2f p) const {
        if(p.x<5||p.y<90||p.x>WIN_W-5||p.y>WIN_H-5) return false;
        return !onPath(p) && !towersOverlap(p);
    }

    void placeTower(sf::Vector2f p){
        if(!canAfford()||!canPlace(p)) return;
        Tower t; t.id=nextTowerId++; t.kind=selectedKind; t.pos=p;
        if (t.kind == TowerKind::BARRACKS) {
            t.rallyPoint = getClosestPathPoint(p);
            for(int i=0; i<3; i++) {
                Warrior w; w.pos = p; w.maxHp = w.hp = 60;
                float angle = i * 2.094f;
                w.rallyPoint = t.rallyPoint + sf::Vector2f(std::cos(angle)*12.f, std::sin(angle)*12.f);
                t.warriors.push_back(w);
            }
        }
        towers.push_back(t); money -= buyCost();
        spawnParticles(p,TOWER_DEFS[int(selectedKind)].color,8,60.f);
    }

    void startWave(){
        if(wave >= TOTAL_WAVES){ phase=Phase::WIN; return; }
        phase = Phase::COMBAT;
        const Wave& w = WAVES[wave];
        toSpawn = w.normal + w.armored + w.fast + w.boss + w.medic + w.slime;
        spawnTimer = 0.f; spawnSerial = 0;
    }

    void spawnNext(){
        const Wave& w = WAVES[wave];
        int spawned = (w.normal + w.armored + w.fast + w.boss + w.medic + w.slime) - toSpawn;
        EnemyKind kind;
        if(spawned < w.normal) kind = EnemyKind::NORMAL;
        else if(spawned < w.normal+w.armored) kind = EnemyKind::ARMORED;
        else if(spawned < w.normal+w.armored+w.fast) kind = EnemyKind::FAST;
        else if(spawned < w.normal+w.armored+w.fast+w.boss) kind = EnemyKind::BOSS;
        else if(spawned < w.normal+w.armored+w.fast+w.boss+w.medic) kind = EnemyKind::MEDIC;
        else kind = EnemyKind::SLIME;
        enemies.push_back(Enemy::make(kind, wave, spawnSerial++, diffMult));
        toSpawn--;
    }

    void applyMutation(Mutation mut) {
        auto targetIt = std::find_if(towers.begin(), towers.end(), [&](const Tower& t){return t.id == mutTargetTowerId;});
        auto sourceIt = std::find_if(towers.begin(), towers.end(), [&](const Tower& t){return t.id == mutSourceTowerId;});
        if (targetIt != towers.end() && sourceIt != towers.end()) {
            targetIt->level = 3; targetIt->mutation = mut;
            if(targetIt->kind == TowerKind::BARRACKS) {
                for(auto& w : targetIt->warriors) {
                    w.maxHp = 60 + (targetIt->level-1)*40;
                    if(targetIt->mutation == Mutation::PATH_A) w.maxHp = int(w.maxHp * 1.5f);
                    w.hp = w.maxHp;
                }
            }
            addFloat(targetIt->pos, L"MUTACJA! \u2605\u2605\u2605", sf::Color(255,100,255), 1.5f);
            spawnParticles(targetIt->pos, sf::Color(255,100,255), 30, 120.f);
            towers.erase(sourceIt);
        }
        phase = prevPhase;
    }

    void castMeteor(sf::Vector2f pos) {
        spellMeteorCD = 30.f; screenShake += 25.f; targetingMeteor = false;
        spawnParticles(pos, sf::Color(255, 100, 50), 80, 250.f, 2.5f);
        addFloat(sf::Vector2f(pos.x, pos.y-50.f), L"BOOM!", sf::Color(255,50,0), 2.f);
        for(auto& e : enemies) {
            if (e.alive && dist(pos, e.pos()) < 220.f) {
                e.takeDamage(1500);
                if (!e.alive) { score+=e.reward; kills++; money+=e.reward; }
            }
        }
    }

    void castGlobalFreeze() {
        spellFreezeCD = 40.f; screenShake += 10.f;
        addFloat(sf::Vector2f(WIN_W/2.f, WIN_H/2.f), L"GLOBALNE ZAMRO\u017bENIE!", sf::Color(160,220,255), 2.5f);
        for(auto& e : enemies) { if (e.alive) e.stunTimer += 4.0f; }
    }

    void update(float dt){
        if(phase == Phase::MUTATION_SELECT || phase == Phase::MENU) return;
        buildTime += dt;
        if (damageFlashTimer > 0.f) damageFlashTimer -= dt;
        if (screenShake > 0.f) screenShake = std::max(0.f, screenShake - dt * 45.f);
        if (spellMeteorCD > 0.f) spellMeteorCD -= dt;
        if (spellFreezeCD > 0.f) spellFreezeCD -= dt;

        for(auto& t : towers) if(t.recoil > 0.f) t.recoil = std::max(0.f, t.recoil - dt * 30.f);

        if(phase == Phase::COMBAT){
            spawnTimer += dt;
            float interval = (wave<3)?1.4f:(wave<6)?1.0f:0.7f;
            if(toSpawn>0 && spawnTimer>=interval){ spawnNext(); spawnTimer=0.f; }

            for(auto& e : enemies) {
                if (e.alive && e.kind == EnemyKind::MEDIC) {
                    e.healTimer += dt;
                    if (e.healTimer >= 1.5f) {
                        e.healTimer = 0.f; bool healedAny = false;
                        for (auto& other : enemies) {
                            if (other.alive && other.health < other.maxHealth && dist(e.pos(), other.pos()) < 120.f) {
                                other.health = std::min(other.maxHealth, other.health + int(20 * (1.f + wave*0.1f)));
                                spawnParticles(other.pos(), sf::Color(100, 255, 100), 4, 40.f); healedAny = true;
                            }
                        }
                        if (healedAny) addFloat(sf::Vector2f(e.pos().x, e.pos().y-20.f), L"+Leczenie!", sf::Color(100,255,100), 0.8f);
                    }
                }
                if (e.alive && e.kind == EnemyKind::BOSS && e.stunTimer <= 0.f) {
                    e.bossFreezeTimer -= dt;
                    if (e.bossFreezeTimer <= 0.f) {
                        e.bossFreezeTimer = 4.0f;
                        std::vector<Tower*> validTowers;
                        for (auto& t : towers) if (!t.frozen && t.kind != TowerKind::MINE && t.kind != TowerKind::BARRACKS) validTowers.push_back(&t);
                        if (!validTowers.empty()) {
                            std::sort(validTowers.begin(), validTowers.end(), [](Tower* a, Tower* b){
                                if (a->level != b->level) return a->level > b->level;
                                return a->damage() > b->damage();
                            });
                            Tower* target = validTowers[0]; target->frozen = true; target->unfreezeClicks = 3;
                            addFloat(target->pos, L"ZAMRO\u017bONA!", sf::Color(160, 220, 255), 2.0f);
                            spawnParticles(target->pos, sf::Color(160, 220, 255), 30, 80.f);
                        }
                    }
                }
            }

            for(auto& e : enemies){
                if(!e.alive) continue;
                e.updateTimers(dt);
                if(e.stunTimer > 0.f) continue;

                bool isBlocked = false;
                for(auto& t : towers) {
                    if(t.kind != TowerKind::BARRACKS) continue;
                    for(auto& w : t.warriors) {
                        if(!w.alive) continue;
                        if(dist(e.pos(), w.pos) < e.radius + 8.f) {
                            isBlocked = true;
                            w.facingDir = (e.pos().x >= w.pos.x) ? 1 : -1;

                            if (w.attackAnimTimer > 0.f) {
                                w.attackAnimTimer -= dt; w.currentFrame = (w.attackAnimTimer > 0.15f) ? 2 : 3;
                            } else {
                                w.animTimer += dt * 1.5f;
                                if (w.animTimer > 0.15f) { w.animTimer = 0.f; w.currentFrame = (w.currentFrame + 1) % 6; }
                            }

                            e.attackCooldown -= dt;
                            if(e.attackCooldown <= 0.f) {
                                w.hp -= e.damage; e.attackCooldown = 1.0f; e.attackAnimTimer = 0.4f;
                                spawnParticles(w.pos, sf::Color(255,0,0), 4, 30.f);
                            }

                            w.attackCooldown -= dt;
                            if(w.attackCooldown <= 0.f) {
                                int wDmg = t.damage();
                                if(t.mutation == Mutation::PATH_B) wDmg *= 3;
                                e.takeDamage(wDmg); w.attackCooldown = t.fireRate(); w.attackAnimTimer = 0.3f; w.currentFrame = 2;
                                spawnParticles(e.pos(), sf::Color(200,200,200), 4, 30.f);
                            }

                            if(w.hp <= 0) {
                                w.alive = false; float respTime = 8.0f - t.level;
                                if(t.mutation == Mutation::PATH_A) respTime *= 0.5f;
                                w.respawnTimer = respTime; spawnParticles(w.pos, sf::Color(200,150,50), 15, 60.f);
                            }
                            break;
                        }
                    }
                    if (isBlocked) break;
                }

                if (!isBlocked) e.pathDist += e.speed * dt;
                if(e.reachedEnd()){
                    lives -= 1; e.alive = false; damageFlashTimer = 1.0f; screenShake += 15.f;
                    spawnParticles(e.pos(),sf::Color(255,50,50),30,150.f);
                }
            }

            std::vector<Enemy> newEnemies;
            for(auto& e : enemies) {
                if (!e.alive && e.health <= 0 && e.kind == EnemyKind::SLIME) {
                    for(int i=0; i<3; i++) {
                        Enemy se = Enemy::make(EnemyKind::SLIME_SMALL, wave, spawnSerial++, diffMult);
                        se.pathDist = std::max(0.f, e.pathDist - i*15.f); newEnemies.push_back(se);
                    }
                }
            }
            if(!newEnemies.empty()) enemies.insert(enemies.end(), newEnemies.begin(), newEnemies.end());
            enemies.erase(std::remove_if(enemies.begin(),enemies.end(), [](const Enemy& e){ return !e.alive; }),enemies.end());

            for(auto& tower : towers){
                if (tower.frozen) continue;
                tower.cooldown -= dt; tower.firing = false;

                if (tower.kind == TowerKind::BARRACKS) {
                    for(auto& w : tower.warriors) {
                        if (!w.alive) {
                            w.respawnTimer -= dt;
                            if (w.respawnTimer <= 0.f) {
                                w.alive = true; w.maxHp = 60 + (tower.level-1)*40;
                                if(tower.mutation == Mutation::PATH_A) w.maxHp = int(w.maxHp * 1.5f);
                                w.hp = w.maxHp; w.pos = tower.pos;
                            }
                        } else {
                            if (w.attackAnimTimer > 0.f) w.attackAnimTimer -= dt;
                            if (dist(w.pos, w.rallyPoint) > 2.f) {
                                sf::Vector2f dir = normalize(w.rallyPoint - w.pos);
                                w.pos += dir * 60.f * dt; w.facingDir = (dir.x >= 0) ? 1 : -1;
                                w.animTimer += dt;
                                if (w.animTimer > 0.15f) { w.animTimer = 0.f; w.currentFrame = (w.currentFrame + 1) % 6; }
                            } else if (w.attackAnimTimer <= 0.f) { w.currentFrame = 0; }
                        }
                    }
                    continue;
                }

                if (tower.kind == TowerKind::MINE) {
                    if (tower.cooldown <= 0.f) {
                        tower.cooldown = tower.fireRate();
                        int inc = (tower.mutation == Mutation::PATH_A) ? 30 : 15; if (tower.level == 2) inc = 20;
                        money += inc; addFloat(tower.pos, L"+"+std::to_wstring(inc)+L"$", sf::Color(255,215,0));
                        spawnParticles(tower.pos, sf::Color(255,215,0), 6, 30.f);
                    } continue;
                }

                Enemy* target = nullptr; float bestDist = -1e9f;
                for(auto& e : enemies){
                    if(e.alive && dist(tower.pos, e.pos()) <= tower.range()){
                        if(e.pathDist > bestDist){ bestDist=e.pathDist; target=&e; }
                    }
                }
                tower.targetId = target ? target->id : -1;

                if(target){
                    sf::Vector2f dir = target->pos()-tower.pos;
                    tower.aimAngle = std::atan2(dir.y,dir.x)*180.f/3.14159f;
                    if(tower.cooldown<=0.f){
                        tower.cooldown = tower.fireRate(); tower.recoil = 8.f;

                        if (tower.kind == TowerKind::BASIC) tower.currentBarrel = (tower.currentBarrel + 1) % tower.level;
                        if (tower.kind == TowerKind::TESLA) {
                            tower.firing = true;
                            int jumps = 3; if(tower.level==2) jumps=4; if(tower.mutation == Mutation::PATH_A) jumps += 2;
                            std::vector<int> hitIds; Enemy* currTarget = target;
                            for(int j=0; j<jumps; j++){
                                hitIds.push_back(currTarget->id); currTarget->takeDamage(tower.damage());
                                if(tower.mutation == Mutation::PATH_B && currTarget->kind != EnemyKind::BOSS) currTarget->stunTimer = 0.5f;
                                if(!currTarget->alive){ score+=currTarget->reward; kills++; money+=currTarget->reward; spawnParticles(currTarget->pos(),currTarget->color,10,70.f); }

                                Enemy* next = nullptr; float cDist = 180.f;
                                for(auto& e2 : enemies){
                                    if(e2.alive && std::find(hitIds.begin(),hitIds.end(),e2.id) == hitIds.end() && dist(currTarget->pos(), e2.pos()) < cDist){
                                        cDist = dist(currTarget->pos(), e2.pos()); next = &e2;
                                    }
                                }
                                if(j==0) lightnings.push_back({tower.pos, currTarget->pos(), 0.15f});
                                if(next) { lightnings.push_back({currTarget->pos(), next->pos(), 0.15f}); currTarget = next; }
                                else break;
                            }
                        }
                        else if(tower.kind==TowerKind::LASER){
                            tower.firing = true; target->takeDamage(tower.damage());
                            if(!target->alive){
                                score+=target->reward; kills++; money+=target->reward;
                                spawnParticles(target->pos(),target->color,16,90.f); addFloat(target->pos(),L"+"+std::to_wstring(target->reward)+L"$",sf::Color(100,255,100));
                            } else spawnParticles(target->pos(),sf::Color(255,150,255),4,50.f);
                        } else {
                            Bullet b; sf::Vector2f spawnPos = tower.pos;
                            if (tower.kind == TowerKind::BASIC) {
                                float offset = 0.f;
                                if(tower.level == 2) offset = (tower.currentBarrel == 0) ? -5.f : 5.f;
                                if(tower.level == 3) offset = (tower.currentBarrel == 0) ? -7.f : (tower.currentBarrel == 1 ? 0.f : 7.f);
                                float perp = tower.aimAngle * 3.14159f / 180.f + 1.5708f;
                                spawnPos.x += std::cos(perp) * offset; spawnPos.y += std::sin(perp) * offset;
                            }
                            if (tower.kind == TowerKind::ARTILLERY) screenShake += 3.f;
                            b.pos = spawnPos; b.vel = normalize(target->pos()-tower.pos) * tower.def().projSpeed;
                            b.targetId = target->id; b.damage = tower.damage(); b.radius = tower.def().explosive ? 7.f : 4.f;
                            b.color = tower.def().projColor; b.explosive = tower.def().explosive; b.level = tower.level;
                            b.splashRadius = tower.def().splashR * (1.f + (tower.level-1)*0.6f);
                            if (tower.mutation == Mutation::PATH_A && tower.kind == TowerKind::ARTILLERY) b.splashRadius *= 1.5f;
                            if (tower.mutation == Mutation::PATH_B && tower.kind == TowerKind::ARTILLERY) b.stun = true;
                            if (tower.mutation == Mutation::PATH_A && tower.kind == TowerKind::SNIPER) b.instakill = true;
                            if (tower.mutation == Mutation::PATH_B && tower.kind == TowerKind::SNIPER) b.pierce = 3;
                            if (tower.mutation == Mutation::PATH_B && tower.kind == TowerKind::FROST)  b.stun = true;

                            if(tower.def().slows) b.color = sf::Color(160,220,255);
                            bullets.push_back(b);
                        }
                    }
                }
            }

            for(auto& b : bullets){
                b.pos += b.vel * dt; b.life -= dt;
                for(auto& e : enemies){
                    if(e.alive && dist(b.pos, e.pos()) < b.radius + e.radius){
                        if (b.pierce > 0) { if (std::find(b.hitTargets.begin(), b.hitTargets.end(), e.id) != b.hitTargets.end()) continue;
                            b.hitTargets.push_back(e.id); }

                        if(b.explosive){
                            screenShake += 5.f + b.level*3.f;
                            for(auto& e2 : enemies){
                                if(e2.alive && dist(b.pos,e2.pos())<b.splashRadius){
                                    e2.takeDamage(b.damage);
                                    if(b.stun && e2.kind != EnemyKind::BOSS) e2.stunTimer = 1.0f;
                                    if(!e2.alive){ score+=e2.reward; kills++; money+=e2.reward; spawnParticles(e2.pos(),e2.color,16,90.f); addFloat(e2.pos(),L"+"+std::to_wstring(e2.reward)+L"$",sf::Color(100,255,100)); }
                                }
                            }
                            spawnParticles(b.pos,sf::Color(255,180,50),30 + b.level*10, 120.f + b.level*30.f);
                        } else {
                            if(TOWER_DEFS[int(TowerKind::FROST)].slows && b.color.b>200){
                                float slowMult = 0.65f - (b.level-1)*0.18f;
                                e.speed = std::max(10.f, e.speed * slowMult);
                                if(b.stun && e.kind != EnemyKind::BOSS) e.stunTimer = 0.5f;
                                spawnParticles(b.pos,sf::Color(160,220,255),5 + b.level*2, 40.f);
                            }

                            if (b.instakill && e.kind != EnemyKind::BOSS && randFloat(0.f, 1.f) <= 0.10f) { e.takeDamage(99999);
                                addFloat(e.pos(), L"ZAB\u00d3JSTWO!", sf::Color(255, 0, 0), 1.5f); }
                            else { e.takeDamage(b.damage); }

                            if(!e.alive){ score+=e.reward; kills++; money+=e.reward; spawnParticles(e.pos(),e.color,16,90.f); addFloat(e.pos(),L"+"+std::to_wstring(e.reward)+L"$",sf::Color(100,255,100)); }
                            else spawnParticles(b.pos,b.color,3,40.f);
                        }

                        if (b.pierce > 0) { b.pierce--; if(b.pierce <= 0) b.life = -1.f; } else { b.life = -1.f; break; }
                    }
                }
            }
            bullets.erase(std::remove_if(bullets.begin(),bullets.end(),[](const Bullet& b){ return b.life<=0.f; }),bullets.end());
            if(toSpawn==0 && enemies.empty()){
                wave++;
                if(wave>=TOTAL_WAVES) phase=Phase::WIN;
                else { phase=Phase::BUILD; money+=100; addFloat(sf::Vector2f(WIN_W/2.f,WIN_H/2.f-60.f),L"Fala uko\u0144czona! +100$",sf::Color(100,255,100),2.f); }
            }
            if(lives<=0){ lives=0; phase=Phase::GAME_OVER; }
        }

        updateParticles(dt);
        for(auto& f:floatTexts){ f.pos.y-=30.f*dt; f.life-=dt; }
        floatTexts.erase(std::remove_if(floatTexts.begin(),floatTexts.end(),[](const FloatText& f){return f.life<=0;}),floatTexts.end());
    }
};