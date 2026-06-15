#include "Game.hpp"

void drawDamageVignette(sf::RenderWindow& w, float timer) {
    if (timer <= 0.f) return;
    float alpha = std::min(timer, 1.f) * 130.f;
    sf::VertexArray vignette(sf::TriangleFan, 6);
    sf::Color edgeCol(255, 0, 0, static_cast<std::uint8_t>(alpha));
    sf::Color centerCol(255, 0, 0, 0);
    vignette[0].position = sf::Vector2f(WIN_W/2.f, WIN_H/2.f); vignette[0].color = centerCol;
    vignette[1].position = sf::Vector2f(0.f, 0.f);             vignette[1].color = edgeCol;
    vignette[2].position = sf::Vector2f(WIN_W, 0.f);           vignette[2].color = edgeCol;
    vignette[3].position = sf::Vector2f(WIN_W, WIN_H);         vignette[3].color = edgeCol;
    vignette[4].position = sf::Vector2f(0.f, WIN_H);           vignette[4].color = edgeCol;
    vignette[5].position = sf::Vector2f(0.f, 0.f);             vignette[5].color = edgeCol;
    w.draw(vignette);
}

void drawPath(sf::RenderWindow& w){
    for(size_t i=1;i<PATH_WP.size();i++){
        sf::Vector2f a=PATH_WP[i-1], b=PATH_WP[i];
        sf::Vector2f dir = normalize(b-a), perp(-dir.y, dir.x);
        sf::VertexArray quad(sf::TriangleStrip, 4);
        quad[0].position = a+perp*28.f; quad[0].color = sf::Color(100,80,50);
        quad[1].position = a-perp*28.f; quad[1].color = sf::Color(90,70,45);
        quad[2].position = b+perp*28.f; quad[2].color = sf::Color(100,80,50);
        quad[3].position = b-perp*28.f; quad[3].color = sf::Color(90,70,45);
        w.draw(quad);
    }
    for(size_t i=1;i<PATH_WP.size();i++){
        sf::Vector2f a=PATH_WP[i-1], b=PATH_WP[i];
        float segLen = dist(a,b);
        int dashes = int(segLen/20.f);
        for(int d=0;d<dashes;d++){
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = lerp(a,b,d/float(dashes)); line[0].color = sf::Color(200,180,120,80);
            line[1].position = lerp(a,b,(d+0.5f)/float(dashes)); line[1].color = sf::Color(200,180,120,80);
            w.draw(line);
        }
    }
}

void drawLasers(sf::RenderWindow& w, const Game& g){
    for(auto& tower : g.towers){
        if(!tower.firing || tower.kind != TowerKind::LASER) continue;
        Enemy* target = nullptr;
        for(auto& e : g.enemies){ if(e.id == tower.targetId){ target = const_cast<Enemy*>(&e); break; } }
        if(!target) continue;

        sf::Vector2f dir = normalize(target->pos() - tower.pos);
        sf::Vector2f perp(-dir.y, dir.x);

        for(int l = 0; l < tower.level; l++) {
            float offset = (l - (tower.level-1)/2.f) * 2.5f;
            sf::Vector2f tP = tower.pos + perp*offset;
            sf::Vector2f eP = target->pos() + perp*offset;

            sf::VertexArray line(sf::Lines, 2);
            line[0].position = tP;
            line[0].color = (tower.mutation == Mutation::PATH_A) ? sf::Color(255,50,50,200) : sf::Color(255,100,255,200);
            line[1].position = eP;
            line[1].color = (tower.mutation == Mutation::PATH_A) ? sf::Color(255,50,50,140) : sf::Color(255,100,255,140);
            w.draw(line);
        }

        for(int i=0;i<3 + tower.level*2;i++){
            sf::VertexArray glow(sf::Lines, 2);
            glow[0].position = tower.pos;
            glow[0].color = (tower.mutation == Mutation::PATH_A) ? sf::Color(255,50,50,static_cast<std::uint8_t>(80-i*(15/tower.level))) : sf::Color(255,100,255,static_cast<std::uint8_t>(80-i*(15/tower.level)));
            glow[1].position = target->pos();
            glow[1].color = (tower.mutation == Mutation::PATH_A) ? sf::Color(255,50,50,10) : sf::Color(255,100,255,10);
            w.draw(glow);
        }
    }
}

constexpr float PANEL_X = 1360.f;
constexpr float PANEL_W = WIN_W - PANEL_X;

void drawMenu(sf::RenderWindow& w) {
    w.draw(makeRect(sf::Vector2f(0.f,0.f), sf::Vector2f(float(WIN_W),float(WIN_H)), sf::Color(20,20,30)));
    drawText(w, L"TOWER DEFENSE", sf::Vector2f(WIN_W/2.f, 150.f), 60, sf::Color(150,180,255), true);
    drawText(w, L"Wybierz poziom trudno\u015bci", sf::Vector2f(WIN_W/2.f, 230.f), 24, sf::Color(200,200,200), true);
    w.draw(makeRect(sf::Vector2f(WIN_W/2.f - 200.f, 320.f), sf::Vector2f(400.f, 90.f), sf::Color(40,80,40), 3.f, sf::Color(100,255,100)));
    drawText(w, L"\u0141ATWY", sf::Vector2f(WIN_W/2.f, 350.f), 28, sf::Color(255,255,255), true);
    drawText(w, L"800$, 30 \u017cy\u0107, S\u0142abi wrogowie", sf::Vector2f(WIN_W/2.f, 385.f), 16, sf::Color(200,255,200), true);

    w.draw(makeRect(sf::Vector2f(WIN_W/2.f - 200.f, 450.f), sf::Vector2f(400.f, 90.f), sf::Color(80,80,40), 3.f, sf::Color(255,255,100)));
    drawText(w, L"NORMALNY", sf::Vector2f(WIN_W/2.f, 480.f), 28, sf::Color(255,255,255), true);
    drawText(w, L"600$, 20 \u017cy\u0107, Standardowi wrogowie", sf::Vector2f(WIN_W/2.f, 515.f), 16, sf::Color(255,255,200), true);
    w.draw(makeRect(sf::Vector2f(WIN_W/2.f - 200.f, 580.f), sf::Vector2f(400.f, 90.f), sf::Color(80,40,40), 3.f, sf::Color(255,100,100)));
    drawText(w, L"TRUDNY", sf::Vector2f(WIN_W/2.f, 610.f), 28, sf::Color(255,255,255), true);
    drawText(w, L"400$, 10 \u017cy\u0107, Bardzo silni wrogowie", sf::Vector2f(WIN_W/2.f, 645.f), 16, sf::Color(255,200,200), true);
}

void drawPanel(sf::RenderWindow& w, const Game& g){
    w.draw(makeRect(sf::Vector2f(PANEL_X, 80.f), sf::Vector2f(PANEL_W, float(WIN_H)-80.f), sf::Color(15,15,30,240), 2.f, sf::Color(60,60,100)));
    drawText(w,L"SKLEP",sf::Vector2f(PANEL_X+PANEL_W/2.f,90.f),20,sf::Color(200,200,255),true);
    for(int i=0;i<8;i++){
        float y = 115.f + i*76.f;
        bool sel = (int(g.selectedKind)==i);
        w.draw(makeRect(sf::Vector2f(PANEL_X+10.f, y),sf::Vector2f(PANEL_W-20.f,68.f), sel?sf::Color(40,40,80):sf::Color(20,20,40), sel?2.f:1.f, sel?sf::Color(120,120,255):sf::Color(50,50,80)));
        sf::CircleShape ic(12.f); ic.setOrigin(12.f, 12.f); ic.setPosition(PANEL_X+30.f, y+18.f);
        ic.setFillColor(TOWER_DEFS[i].color); ic.setOutlineColor(sf::Color(255,255,255,100)); ic.setOutlineThickness(2.f); w.draw(ic);

        drawText(w, std::wstring(TOWER_DEFS[i].name), sf::Vector2f(PANEL_X+55.f, y+2.f), 15, sf::Color(220,220,255));
        drawText(w, std::to_wstring(TOWER_DEFS[i].cost)+L"$", sf::Vector2f(PANEL_X+55.f, y+20.f), 14, g.money>=TOWER_DEFS[i].cost?sf::Color(100,255,100):sf::Color(200,80,80));
        drawText(w,L"["+std::to_wstring(i+1)+L"]",sf::Vector2f(PANEL_X+PANEL_W-30.f,y+5.f),12,sf::Color(120,120,160));

        if (i != 1 && i != 3) {
            drawText(w,L"ZAS", sf::Vector2f(PANEL_X+55.f, y+37.f), 9, sf::Color(150,180,255));
            drawText(w,L"DMG", sf::Vector2f(PANEL_X+55.f, y+48.f), 9, sf::Color(255,150,150));

            int rDots = std::min(5, std::max(1, (int)std::ceil(TOWER_DEFS[i].range / 180.f)));
            int dDots = std::min(5, std::max(1, (int)std::ceil(TOWER_DEFS[i].damage / 30.f)));
            if(TowerKind(i) == TowerKind::LASER) dDots = 5;
            for(int d=0; d<5; d++) {
                sf::CircleShape rDot(3.f);
                rDot.setPosition(PANEL_X + 85.f + d*12.f, y + 40.f);
                rDot.setFillColor(d < rDots ? sf::Color(80,160,255) : sf::Color(40,40,60));
                w.draw(rDot);

                sf::CircleShape dDot(3.f);
                dDot.setPosition(PANEL_X + 85.f + d*12.f, y + 51.f);
                dDot.setFillColor(d < dDots ? sf::Color(255,100,100) : sf::Color(40,40,60));
                w.draw(dDot);
            }
        } else {
            drawText(w, std::wstring(TOWER_DEFS[i].desc), sf::Vector2f(PANEL_X+55.f, y+45.f), 10, sf::Color(150,150,200));
        }
    }

    float sy = WIN_H - 120.f;
    w.draw(makeRect(sf::Vector2f(PANEL_X+10.f, sy), sf::Vector2f(PANEL_W-20.f, 110.f), sf::Color(40,20,20), 2.f, sf::Color(150,50,50)));
    drawText(w,L"ZAKL\u0118CIA GRACZA", sf::Vector2f(PANEL_X+PANEL_W/2.f, sy+5.f), 14, sf::Color(255,150,150), true);
    w.draw(makeRect(sf::Vector2f(PANEL_X+20.f, sy+25.f), sf::Vector2f(85.f, 65.f), sf::Color(60,30,30), 2.f, g.spellMeteorCD<=0?sf::Color(255,100,50):sf::Color(80,50,50)));
    drawText(w,L"[Q] Meteoryt", sf::Vector2f(PANEL_X+62.f, sy+35.f), 11, sf::Color(255,200,150), true);
    if(g.spellMeteorCD>0) drawText(w,std::to_wstring(int(g.spellMeteorCD))+L"s", sf::Vector2f(PANEL_X+62.f, sy+65.f), 16, sf::Color(255,100,100), true);
    else if(g.targetingMeteor) drawText(w,L"CELUJE!", sf::Vector2f(PANEL_X+62.f, sy+65.f), 14, sf::Color(255,255,0), true);
    w.draw(makeRect(sf::Vector2f(PANEL_X+135.f, sy+25.f), sf::Vector2f(85.f, 65.f), sf::Color(30,40,60), 2.f, g.spellFreezeCD<=0?sf::Color(100,200,255):sf::Color(50,60,80)));
    drawText(w,L"[W] EMP L\u00f3d", sf::Vector2f(PANEL_X+177.f, sy+35.f), 11, sf::Color(200,240,255), true);
    if(g.spellFreezeCD>0) drawText(w,std::to_wstring(int(g.spellFreezeCD))+L"s", sf::Vector2f(PANEL_X+177.f, sy+65.f), 16, sf::Color(100,200,255), true);
    else drawText(w,L"GOTOWE", sf::Vector2f(PANEL_X+177.f, sy+65.f), 14, sf::Color(100,255,100), true);
}

void drawTopBar(sf::RenderWindow& w, const Game& g){
    w.draw(makeRect(sf::Vector2f(0.f,0.f), sf::Vector2f(float(WIN_W),78.f), sf::Color(10,10,25)));
    w.draw(makeRect(sf::Vector2f(0.f,76.f), sf::Vector2f(float(WIN_W),2.f), sf::Color(60,60,120)));

    drawText(w, L"TOWER DEFENSE", sf::Vector2f(20.f,12.f), 28, sf::Color(150,180,255));
    drawText(w, L"\u017bYCIA: " + std::to_wstring(g.lives), sf::Vector2f(350.f,14.f), 26, sf::Color(255,100,100));
    drawText(w, L"$ " + std::to_wstring(g.money), sf::Vector2f(550.f,14.f), 26, sf::Color(100,240,100));
    drawText(w, L"ZABICI: " + std::to_wstring(g.kills), sf::Vector2f(750.f,14.f), 26, sf::Color(220,100,100));
    drawText(w, L"WYNIK: " + std::to_wstring(g.score), sf::Vector2f(1000.f,14.f), 26, sf::Color(255,220,80));
    std::wstring waveStr = (g.phase == Phase::BUILD ? std::wstring(L"FAZA BUDOWY - ") : std::wstring(L""))
                           + L"FALA " + std::to_wstring(g.wave+1) + L"/" + std::to_wstring(TOTAL_WAVES);
    drawText(w, waveStr, sf::Vector2f(PANEL_X+PANEL_W/2.f,14.f), 18, g.phase==Phase::BUILD?sf::Color(100,200,255):sf::Color(255,160,80), true);
}

void drawMutationOverlay(sf::RenderWindow& w, const Game& g) {
    w.draw(makeRect(sf::Vector2f(0.f,0.f), sf::Vector2f(float(WIN_W),float(WIN_H)), sf::Color(0,0,0,200)));
    drawText(w, L"WYBIERZ \u015aCIE\u017bK\u0118 MUTACJI!", sf::Vector2f(WIN_W/2.f, 150.f), 46, sf::Color(255,220,80), true);
    drawText(w, L"Osi\u0105gni\u0119to Poziom 3. Wybierz unikalne ulepszenie dla tej wie\u017cy:", sf::Vector2f(WIN_W/2.f, 220.f), 24, sf::Color(200,200,255), true);
    auto it = std::find_if(g.towers.begin(), g.towers.end(), [&](const Tower& t){return t.id == g.mutTargetTowerId;});
    if(it != g.towers.end()) {
        auto info = getMutationInfo(it->kind);
        w.draw(makeRect(sf::Vector2f(WIN_W/2.f - 400.f, 350.f), sf::Vector2f(350.f, 300.f), sf::Color(40,40,60), 4.f, sf::Color(255,100,100)));
        drawText(w, info.first, sf::Vector2f(WIN_W/2.f - 225.f, 450.f), 24, sf::Color(255,255,255), true);
        w.draw(makeRect(sf::Vector2f(WIN_W/2.f + 50.f, 350.f), sf::Vector2f(350.f, 300.f), sf::Color(40,40,60), 4.f, sf::Color(100,255,100)));
        drawText(w, info.second, sf::Vector2f(WIN_W/2.f + 225.f, 450.f), 24, sf::Color(255,255,255), true);
    }
}

void drawOverlay(sf::RenderWindow& w, const Game& g){
    w.draw(makeRect(sf::Vector2f(0.f,0.f), sf::Vector2f(float(WIN_W),float(WIN_H)), sf::Color(0,0,0,160)));
    drawText(w,g.phase==Phase::WIN?L"ZWYCI\u0118STWO!":L"KONIEC GRY",sf::Vector2f(WIN_W/2.f, WIN_H/2.f-60.f),60,g.phase==Phase::WIN?sf::Color(100,255,100):sf::Color(255,60,60),true);
    drawText(w,L"Wynik: "+std::to_wstring(g.score),sf::Vector2f(WIN_W/2.f, WIN_H/2.f+20.f),30,sf::Color(255,220,80),true);
    drawText(w,L"Zabici: "+std::to_wstring(g.kills),sf::Vector2f(WIN_W/2.f, WIN_H/2.f+65.f),24,sf::Color(200,200,255),true);
    drawText(w,L"[R] Powr\u00f3t do Menu",sf::Vector2f(WIN_W/2.f, WIN_H/2.f+120.f),22,sf::Color(150,255,150),true);
}

int main(){
    sf::RenderWindow window(sf::VideoMode(WIN_W,WIN_H), "Tower Defense - ANIMATION UPDATE (SFML 2.5)", sf::Style::Default);
    window.setFramerateLimit(60); sf::Clock clock; Game game; bool spaceHeld = false;

    sf::View logicalView(sf::Vector2f(WIN_W / 2.f, WIN_H / 2.f), sf::Vector2f(float(WIN_W), float(WIN_H)));
    while(window.isOpen()){
        sf::Event ev;
        while(window.pollEvent(ev)){
            if(ev.type == sf::Event::Closed) window.close();

            if(ev.type == sf::Event::KeyPressed){
                auto code = ev.key.code;
                if(code == sf::Keyboard::Escape) window.close();
                if(code == sf::Keyboard::Space) spaceHeld = true;

                if(code == sf::Keyboard::Num1) game.selectedKind=TowerKind::BASIC;
                if(code == sf::Keyboard::Num2) game.selectedKind=TowerKind::BARRACKS;
                if(code == sf::Keyboard::Num3) game.selectedKind=TowerKind::FROST;
                if(code == sf::Keyboard::Num4) game.selectedKind=TowerKind::MINE;
                if(code == sf::Keyboard::Num5) game.selectedKind=TowerKind::SNIPER;
                if(code == sf::Keyboard::Num6) game.selectedKind=TowerKind::TESLA;
                if(code == sf::Keyboard::Num7) game.selectedKind=TowerKind::ARTILLERY;
                if(code == sf::Keyboard::Num8) game.selectedKind=TowerKind::LASER;
                if(code == sf::Keyboard::Enter && game.phase==Phase::BUILD) game.startWave();

                if(code == sf::Keyboard::Q && game.spellMeteorCD <= 0.f && game.phase != Phase::MENU) {
                    game.targetingMeteor = !game.targetingMeteor;
                }
                if(code == sf::Keyboard::W && game.spellFreezeCD <= 0.f && game.phase != Phase::MENU) {
                    game.castGlobalFreeze();
                }

                if(code == sf::Keyboard::Delete){
                    auto it = std::find_if(game.towers.begin(),game.towers.end(),[](const Tower& t){return t.selected;});
                    if(it!=game.towers.end()){
                        int refund = it->def().cost/2;
                        game.money += refund;
                        addFloat(it->pos,L"Sprzedano +"+std::to_wstring(refund)+L"$",sf::Color(100,255,180));
                        spawnParticles(it->pos,sf::Color(100,255,100),8,50.f); game.towers.erase(it);
                    }
                }
                if(code == sf::Keyboard::R && (game.phase==Phase::GAME_OVER||game.phase==Phase::WIN)) game = Game{};
            }

            if(ev.type == sf::Event::KeyReleased){
                if(ev.key.code == sf::Keyboard::Space) spaceHeld = false;
            }

            if(ev.type == sf::Event::MouseButtonPressed){
                sf::Vector2f mp = window.mapPixelToCoords(sf::Vector2i(ev.mouseButton.x, ev.mouseButton.y), logicalView);
                game.mouseWorld = mp;

                if (game.phase == Phase::MENU && ev.mouseButton.button == sf::Mouse::Left) {
                    if(mp.x >= WIN_W/2.f - 200.f && mp.x <= WIN_W/2.f + 200.f) {
                        if(mp.y >= 320.f && mp.y <= 410.f) game.startGame(Difficulty::EASY);
                        if(mp.y >= 450.f && mp.y <= 540.f) game.startGame(Difficulty::NORMAL);
                        if(mp.y >= 580.f && mp.y <= 670.f) game.startGame(Difficulty::HARD);
                    }
                    continue;
                }

                if (game.phase == Phase::MUTATION_SELECT && ev.mouseButton.button == sf::Mouse::Left) {
                    if (mp.x >= WIN_W/2.f - 400.f && mp.x <= WIN_W/2.f - 50.f && mp.y >= 350.f && mp.y <= 650.f) game.applyMutation(Mutation::PATH_A);
                    else if (mp.x >= WIN_W/2.f + 50.f && mp.x <= WIN_W/2.f + 400.f && mp.y >= 350.f && mp.y <= 650.f) game.applyMutation(Mutation::PATH_B);
                    continue;
                }

                if(ev.mouseButton.button == sf::Mouse::Left && game.phase != Phase::MUTATION_SELECT && game.phase != Phase::MENU){
                    if(game.targetingMeteor && mp.x < PANEL_X) {
                        game.castMeteor(mp);
                        continue;
                    }

                    bool hit=false;
                    auto hitIt = std::find_if(game.towers.begin(), game.towers.end(), [&](const Tower& t){ return dist(t.pos, mp) < 22.f; });
                    if(hitIt != game.towers.end()){
                        hit = true;
                        if (hitIt->frozen) {
                            hitIt->unfreezeClicks--;
                            spawnParticles(hitIt->pos, sf::Color(255, 255, 255), 8, 40.f);
                            if (hitIt->unfreezeClicks <= 0) { hitIt->frozen = false; addFloat(hitIt->pos, L"ODBLOKOWANA!", sf::Color(100, 255, 100), 1.5f);
                            }
                            continue;
                        }
                        game.draggedTowerId = hitIt->id;
                        game.dragStartPos = hitIt->pos; hitIt->selected = true;
                        Tower t = *hitIt; game.towers.erase(hitIt); game.towers.push_back(t);
                    }

                    for(auto& t : game.towers) if(!hit || t.id != game.draggedTowerId) t.selected = false;
                    if(!hit && game.phase==Phase::BUILD && mp.x<PANEL_X && !game.targetingMeteor) game.placeTower(mp);
                    if(mp.x>=PANEL_X){
                        for(int i=0;i<8;i++){ if(mp.y>=(115.f+i*76.f) && mp.y<=(115.f+i*76.f+68.f)) game.selectedKind = TowerKind(i);
                        }

                        float sy = WIN_H - 120.f;
                        if(mp.x >= PANEL_X+20.f && mp.x <= PANEL_X+105.f && mp.y >= sy+25.f && mp.y <= sy+90.f && game.spellMeteorCD<=0) game.targetingMeteor = !game.targetingMeteor;
                        if(mp.x >= PANEL_X+135.f && mp.x <= PANEL_X+220.f && mp.y >= sy+25.f && mp.y <= sy+90.f && game.spellFreezeCD<=0) game.castGlobalFreeze();
                    }
                }
                if(ev.mouseButton.button == sf::Mouse::Right && game.phase != Phase::MUTATION_SELECT && game.phase != Phase::MENU){
                    game.targetingMeteor = false;
                    for(auto& t:game.towers) t.selected=false;
                    if(game.draggedTowerId != -1){
                        auto itD = std::find_if(game.towers.begin(), game.towers.end(), [&](const Tower& t){return t.id == game.draggedTowerId;});
                        if(itD != game.towers.end()) {
                            itD->pos = game.dragStartPos;
                            if (itD->kind == TowerKind::BARRACKS) {
                                itD->rallyPoint = getClosestPathPoint(itD->pos);
                                for(int i=0; i<3; i++) {
                                    float angle = i * 2.094f;
                                    itD->warriors[i].rallyPoint = itD->rallyPoint + sf::Vector2f(std::cos(angle)*12.f, std::sin(angle)*12.f);
                                }
                            }
                        }
                        game.draggedTowerId = -1;
                    }
                }
            }

            if(ev.type == sf::Event::MouseButtonReleased){
                if(ev.mouseButton.button == sf::Mouse::Left && game.draggedTowerId != -1){
                    auto itD = std::find_if(game.towers.begin(), game.towers.end(), [&](const Tower& t){return t.id == game.draggedTowerId;});
                    if(itD != game.towers.end()){
                        bool merged = false;
                        for(auto& target : game.towers){
                            if(target.id != itD->id && dist(target.pos, game.mouseWorld) < 30.f){
                                if(target.frozen) continue;
                                if(target.kind == itD->kind && target.level == itD->level && target.level < Game::MAX_LEVEL){
                                    if (target.level == 2) {
                                        game.prevPhase = game.phase;
                                        game.phase = Phase::MUTATION_SELECT;
                                        game.mutSourceTowerId = itD->id; game.mutTargetTowerId = target.id;
                                        merged = true; itD->pos = game.dragStartPos; break;
                                    } else {
                                        target.level++;
                                        addFloat(target.pos, L"MERGE! \u2605", sf::Color(255,100,255), 1.5f);
                                        spawnParticles(target.pos, sf::Color(255,100,255), 20, 100.f);
                                        if(target.kind == TowerKind::BARRACKS) {
                                            for(auto& w : target.warriors) { w.maxHp = 60 + (target.level-1)*40;
                                                w.hp = w.maxHp; }
                                        }
                                        merged = true;
                                        game.towers.erase(itD); break;
                                    }
                                }
                            }
                        }
                        if(!merged) {
                            itD->pos = game.dragStartPos;
                        } else if (itD != game.towers.end() && itD->kind == TowerKind::BARRACKS) {
                            itD->rallyPoint = getClosestPathPoint(itD->pos);
                            for(int i=0; i<3; i++) {
                                float angle = i * 2.094f;
                                itD->warriors[i].rallyPoint = itD->rallyPoint + sf::Vector2f(std::cos(angle)*12.f, std::sin(angle)*12.f);
                            }
                        }
                    }
                    game.draggedTowerId = -1;
                }
            }

            if(ev.type == sf::Event::MouseMoved){
                game.mouseWorld = window.mapPixelToCoords(sf::Vector2i(ev.mouseMove.x, ev.mouseMove.y), logicalView);
                if(game.draggedTowerId != -1){
                    for(auto& t : game.towers){
                        if(t.id == game.draggedTowerId){
                            t.pos = game.mouseWorld;
                            if(t.kind == TowerKind::BARRACKS) {
                                t.rallyPoint = getClosestPathPoint(t.pos);
                                for(int i=0; i<3; i++) {
                                    float angle = i * 2.094f;
                                    t.warriors[i].rallyPoint = t.rallyPoint + sf::Vector2f(std::cos(angle)*12.f, std::sin(angle)*12.f);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        float dt = std::min(clock.restart().asSeconds(), 0.05f);
        if (spaceHeld && (game.phase == Phase::COMBAT || game.phase == Phase::BUILD)) dt *= 3.0f;
        if(game.phase==Phase::COMBAT || game.phase==Phase::BUILD) game.update(dt);

        window.clear(sf::Color(15,35,15));
        sf::View shakeView = logicalView;
        if (game.screenShake > 0.f && game.phase != Phase::MENU) {
            float sx = randFloat(-game.screenShake, game.screenShake);
            float sy = randFloat(-game.screenShake, game.screenShake);
            shakeView.move(sf::Vector2f(sx, sy));
        }
        window.setView(shakeView);
        if (game.phase == Phase::MENU) { drawMenu(window); }
        else {
            sf::RectangleShape tile(sf::Vector2f(TILE,TILE));
            for(float x=0;x<PANEL_X;x+=TILE)
                for(float y=80;y<WIN_H;y+=TILE){
                    int shade = ((int(x/TILE)+int((y-80)/TILE))%2) ? 28 : 22;
                    tile.setFillColor(sf::Color(shade, shade+18, shade));
                    tile.setPosition(sf::Vector2f(x,y)); window.draw(tile);
                }

            drawPath(window);
            for(auto& t:game.towers) {
                if (game.phase == Phase::MUTATION_SELECT && t.id == game.mutSourceTowerId) continue;
                t.draw(window, game.phase==Phase::BUILD, t.id == game.draggedTowerId);
            }
            drawParticles(window);
            for(auto& b:bullets){
                sf::CircleShape bc(b.radius); bc.setOrigin(b.radius, b.radius);
                bc.setPosition(b.pos); bc.setFillColor(b.color); window.draw(bc);
            }

            for(auto& e:game.enemies) e.draw(window);
            if (game.targetingMeteor && game.mouseWorld.x < PANEL_X) {
                sf::CircleShape blast(220.f);
                blast.setOrigin(220.f, 220.f);
                blast.setPosition(game.mouseWorld); blast.setFillColor(sf::Color(255,50,0, 40));
                blast.setOutlineColor(sf::Color(255,50,0, 150)); blast.setOutlineThickness(2.f);
                window.draw(blast);
                drawText(window, L"KLIKNIJ BY UDERZY\u0106", sf::Vector2f(game.mouseWorld.x, game.mouseWorld.y - 30.f), 16, sf::Color::Yellow, true);
            }

            drawLasers(window, game);
            if(game.phase==Phase::BUILD && game.mouseWorld.x < PANEL_X && game.draggedTowerId == -1 && !game.targetingMeteor){
                sf::CircleShape prev(20.f);
                prev.setOrigin(20.f, 20.f); prev.setPosition(game.mouseWorld);
                bool ok = game.canPlace(game.mouseWorld) && game.canAfford();
                prev.setFillColor(ok?sf::Color(100,255,100,60):sf::Color(255,80,80,60));
                prev.setOutlineColor(ok?sf::Color(100,255,100,160):sf::Color(255,80,80,160));
                prev.setOutlineThickness(2.f); window.draw(prev);

                float rng = TOWER_DEFS[int(game.selectedKind)].range;
                if(rng > 0) {
                    sf::CircleShape rc(rng);
                    rc.setOrigin(rng, rng); rc.setPosition(game.mouseWorld);
                    rc.setFillColor(sf::Color(255,255,255,8)); rc.setOutlineColor(ok?sf::Color(100,255,100,80):sf::Color(255,80,80,80));
                    rc.setOutlineThickness(1.f); window.draw(rc);
                }
            }

            sf::Font* f = loadFont();
            if(f){
                for(auto& ft:floatTexts){
                    sf::Text t(ft.txt, *f, 16);
                    sf::Color c = ft.col;
                    c.a = static_cast<std::uint8_t>(255*std::min(ft.life,1.f));
                    t.setFillColor(c); auto b = t.getLocalBounds();
                    t.setOrigin(b.left + b.width/2.f, b.top + b.height/2.f);
                    t.setPosition(ft.pos); window.draw(t);
                }
            }

            window.setView(logicalView);
            drawTopBar(window, game); drawPanel(window, game);
            drawDamageVignette(window, game.damageFlashTimer);

            if (game.phase == Phase::MUTATION_SELECT) drawMutationOverlay(window, game);
            if (game.phase == Phase::GAME_OVER || game.phase == Phase::WIN) drawOverlay(window, game);
        }

        window.display();
    }
    return 0;
}