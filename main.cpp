#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

struct Ball {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float size;
    sf::Color color;
    std::vector<sf::Vector2f> trail;
};

struct AABB {
    float x, y, width, height;
    
    bool contains(const Ball& ball) const {
        return ball.pos.x - ball.size >= x && 
               ball.pos.x + ball.size <= x + width &&
               ball.pos.y - ball.size >= y && 
               ball.pos.y + ball.size <= y + height;
    }
    
    bool intersects(const AABB& other) const {
        return x < other.x + other.width && 
               x + width > other.x &&
               y < other.y + other.height && 
               y + height > other.y;
    }
};

class Quadtree {
private:
    static const int MAX_OBJECTS = 4;
    static const int MAX_LEVELS = 5;
    
    int level;
    AABB bounds;
    std::vector<Ball*> objects;
    Quadtree* nodes[4];
    bool divided;
    
    int getIndex(const Ball& ball) const {
        float verticalMidpoint = bounds.x + bounds.width / 2;
        float horizontalMidpoint = bounds.y + bounds.height / 2;
        
        bool topQuadrant = ball.pos.y - ball.size < horizontalMidpoint;
        bool bottomQuadrant = ball.pos.y + ball.size > horizontalMidpoint;
        bool leftQuadrant = ball.pos.x - ball.size < verticalMidpoint;
        bool rightQuadrant = ball.pos.x + ball.size > verticalMidpoint;
        
        if (leftQuadrant && topQuadrant) return 0;
        if (rightQuadrant && topQuadrant) return 1;
        if (leftQuadrant && bottomQuadrant) return 2;
        if (rightQuadrant && bottomQuadrant) return 3;
        return -1;
    }
    
    void subdivide() {
        float subWidth = bounds.width / 2;
        float subHeight = bounds.height / 2;
        float x = bounds.x;
        float y = bounds.y;
        
        nodes[0] = new Quadtree(level + 1, {x, y, subWidth, subHeight});
        nodes[1] = new Quadtree(level + 1, {x + subWidth, y, subWidth, subHeight});
        nodes[2] = new Quadtree(level + 1, {x, y + subHeight, subWidth, subHeight});
        nodes[3] = new Quadtree(level + 1, {x + subWidth, y + subHeight, subWidth, subHeight});
        
        divided = true;
    }
    
public:
    Quadtree(int pLevel, const AABB& pBounds) 
        : level(pLevel), bounds(pBounds), divided(false) {
        for (int i = 0; i < 4; i++) {
            nodes[i] = nullptr;
        }
    }
    
    ~Quadtree() {
        clear();
    }
    
    void clear() {
        objects.clear();
        
        if (divided) {
            for (int i = 0; i < 4; i++) {
                if (nodes[i]) {
                    delete nodes[i];
                    nodes[i] = nullptr;
                }
            }
            divided = false;
        }
    }
    
    void insert(Ball* ball) {
        if (!bounds.contains(*ball)) {
            return;
        }
        
        if (objects.size() < MAX_OBJECTS || level >= MAX_LEVELS) {
            objects.push_back(ball);
            return;
        }
        
        if (!divided) {
            subdivide();
        }
        
        int index = getIndex(*ball);
        if (index != -1) {
            nodes[index]->insert(ball);
        } else {
            objects.push_back(ball);
        }
    }
    
    void retrieve(std::vector<Ball*>& returnObjects, const Ball& ball) {
        int index = getIndex(ball);
        if (index != -1 && divided) {
            nodes[index]->retrieve(returnObjects, ball);
        }
        
        returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
    }
    
    void getBounds(std::vector<AABB>& boundsList) const {
        boundsList.push_back(bounds);
        if (divided) {
            for (int i = 0; i < 4; i++) {
                if (nodes[i]) {
                    nodes[i]->getBounds(boundsList);
                }
            }
        }
    }
};

void checkCollisionBruteForce(std::vector<Ball>& balls) {
    for (size_t i = 0; i < balls.size(); i++) {
        for (size_t j = i + 1; j < balls.size(); j++) {
            sf::Vector2f diff = balls[j].pos - balls[i].pos;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            float minDist = balls[i].size + balls[j].size;

            if (dist < minDist && dist > 0.1f) {
                sf::Vector2f normal = {diff.x / dist, diff.y / dist};
                
                float overlap = minDist - dist;
                balls[i].pos -= normal * (overlap * 0.5f);
                balls[j].pos += normal * (overlap * 0.5f);

                float dot1 = balls[i].vel.x * normal.x + balls[i].vel.y * normal.y;
                float dot2 = balls[j].vel.x * normal.x + balls[j].vel.y * normal.y;

                balls[i].vel.x -= 2 * dot1 * normal.x;
                balls[i].vel.y -= 2 * dot1 * normal.y;
                balls[j].vel.x -= 2 * dot2 * normal.x;
                balls[j].vel.y -= 2 * dot2 * normal.y;
            }
        }
    }
}

void checkCollisionQuadtree(std::vector<Ball>& balls, float w, float h) {
    Quadtree qt(0, {0, 0, w, h});
    
    for (auto& ball : balls) {
        qt.insert(&ball);
    }
    
    for (size_t i = 0; i < balls.size(); i++) {
        std::vector<Ball*> candidates;
        qt.retrieve(candidates, balls[i]);
        
        for (Ball* other : candidates) {
            if (other == &balls[i]) continue;
            
            size_t j = other - &balls[0];
            if (j <= i) continue;
            
            sf::Vector2f diff = balls[j].pos - balls[i].pos;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            float minDist = balls[i].size + balls[j].size;

            if (dist < minDist && dist > 0.1f) {
                sf::Vector2f normal = {diff.x / dist, diff.y / dist};
                
                float overlap = minDist - dist;
                balls[i].pos -= normal * (overlap * 0.5f);
                balls[j].pos += normal * (overlap * 0.5f);

                float dot1 = balls[i].vel.x * normal.x + balls[i].vel.y * normal.y;
                float dot2 = balls[j].vel.x * normal.x + balls[j].vel.y * normal.y;

                balls[i].vel.x -= 2 * dot1 * normal.x;
                balls[i].vel.y -= 2 * dot1 * normal.y;
                balls[j].vel.x -= 2 * dot2 * normal.x;
                balls[j].vel.y -= 2 * dot2 * normal.y;
            }
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Collision Detection - Brute Force / Quadtree");
    window.setFramerateLimit(60);

    std::vector<Ball> balls;
    
    balls.push_back({{150, 200}, {180, 140}, 18, sf::Color(100, 200, 255)});
    balls.push_back({{300, 100}, {-200, 180}, 22, sf::Color(255, 120, 100)});
    balls.push_back({{500, 300}, {160, -150}, 20, sf::Color(150, 255, 150)});
    balls.push_back({{200, 400}, {-140, -120}, 16, sf::Color(255, 200, 100)});
    balls.push_back({{600, 250}, {-180, 200}, 24, sf::Color(200, 150, 255)});
    balls.push_back({{100, 150}, {120, -100}, 14, sf::Color(255, 100, 150)});
    balls.push_back({{400, 200}, {-160, 120}, 19, sf::Color(100, 255, 200)});
    balls.push_back({{250, 350}, {140, -180}, 17, sf::Color(255, 150, 50)});
    balls.push_back({{550, 100}, {-120, 160}, 21, sf::Color(150, 100, 255)});
    balls.push_back({{700, 400}, {-200, -140}, 23, sf::Color(255, 255, 100)});
    balls.push_back({{80, 300}, {100, 180}, 15, sf::Color(200, 255, 100)});
    balls.push_back({{450, 450}, {-180, -200}, 18, sf::Color(255, 100, 255)});
    balls.push_back({{350, 250}, {200, -120}, 16, sf::Color(100, 150, 255)});
    balls.push_back({{650, 350}, {-140, 200}, 20, sf::Color(255, 200, 200)});
    balls.push_back({{150, 500}, {160, -160}, 17, sf::Color(200, 255, 255)});
    balls.push_back({{750, 200}, {-220, 140}, 22, sf::Color(255, 180, 100)});
    balls.push_back({{50, 450}, {180, -200}, 15, sf::Color(150, 200, 100)});
    balls.push_back({{500, 50}, {-100, 220}, 19, sf::Color(255, 120, 200)});
    balls.push_back({{300, 500}, {-160, -180}, 16, sf::Color(100, 255, 255)});
    balls.push_back({{700, 500}, {-200, -160}, 21, sf::Color(255, 100, 100)});

    bool useQuadtree = false;
    bool showQuadtree = false;
    
    sf::Clock clock;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (!event) continue;
            if (auto* closeEvent = event->getIf<sf::Event::Closed>()) {
                window.close();
            }
            
            if (auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    useQuadtree = !useQuadtree;
                }
                if (keyEvent->code == sf::Keyboard::Key::D) {
                    showQuadtree = !showQuadtree;
                }
            }
        }

        float dt = clock.restart().asSeconds();
        float w = static_cast<float>(window.getSize().x);
        float h = static_cast<float>(window.getSize().y);

        for (auto& ball : balls) {
            ball.pos += ball.vel * dt;

            if (ball.pos.x - ball.size < 0) {
                ball.pos.x = ball.size;
                ball.vel.x = -ball.vel.x;
            } else if (ball.pos.x + ball.size > w) {
                ball.pos.x = w - ball.size;
                ball.vel.x = -ball.vel.x;
            }

            if (ball.pos.y - ball.size < 0) {
                ball.pos.y = ball.size;
                ball.vel.y = -ball.vel.y;
            } else if (ball.pos.y + ball.size > h) {
                ball.pos.y = h - ball.size;
                ball.vel.y = -ball.vel.y;
            }

            float speed = std::sqrt(ball.vel.x * ball.vel.x + ball.vel.y * ball.vel.y);
            if (speed > 5.0f) {
                ball.trail.push_back(ball.pos);
                if (ball.trail.size() > 35) {
                    ball.trail.erase(ball.trail.begin());
                }
            }
        }

        if (useQuadtree) {
            checkCollisionQuadtree(balls, w, h);
        } else {
            checkCollisionBruteForce(balls);
        }

        window.clear(sf::Color(20, 20, 40));
        
        if (showQuadtree && useQuadtree) {
            Quadtree qt(0, {0, 0, w, h});
            for (auto& ball : balls) {
                qt.insert(&ball);
            }
            std::vector<AABB> boundsList;
            qt.getBounds(boundsList);
            
            for (const auto& bound : boundsList) {
                sf::RectangleShape rect(sf::Vector2f(bound.width, bound.height));
                rect.setPosition(sf::Vector2f(bound.x, bound.y));
                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color(100, 100, 100));
                rect.setOutlineThickness(1);
                window.draw(rect);
            }
        }
        
        for (const auto& ball : balls) {
            if (ball.trail.size() > 1) {
                for (size_t i = 0; i < ball.trail.size(); i++) {
                    float t = static_cast<float>(i) / ball.trail.size();
                    float smoothT = t * t * (3.0f - 2.0f * t);
                    
                    for (int layer = 0; layer < 3; layer++) {
                        float layerAlpha = (1.0f - smoothT) * (150.0f - layer * 30.0f);
                        float layerSize = ball.size * (0.15f + smoothT * 0.85f + layer * 0.15f);
                        
                        sf::Color trailColor = ball.color;
                        trailColor.a = static_cast<unsigned char>(layerAlpha);
                        
                        float darken = 0.5f + smoothT * 0.3f;
                        trailColor.r = static_cast<unsigned char>(trailColor.r * darken);
                        trailColor.g = static_cast<unsigned char>(trailColor.g * darken);
                        trailColor.b = static_cast<unsigned char>(trailColor.b * darken);
                        
                        sf::CircleShape trailCircle(layerSize);
                        trailCircle.setOrigin({layerSize, layerSize});
                        trailCircle.setPosition(ball.trail[i]);
                        trailCircle.setFillColor(trailColor);
                        window.draw(trailCircle);
                    }
                }
            }
            
            sf::Color outerGlow = ball.color;
            outerGlow.a = 60;
            sf::CircleShape outerGlowCircle(ball.size * 1.8f);
            outerGlowCircle.setOrigin({ball.size * 1.8f, ball.size * 1.8f});
            outerGlowCircle.setPosition(ball.pos);
            outerGlowCircle.setFillColor(outerGlow);
            window.draw(outerGlowCircle);
            
            sf::Color glowColor = ball.color;
            glowColor.a = 120;
            sf::CircleShape glow(ball.size * 1.4f);
            glow.setOrigin({ball.size * 1.4f, ball.size * 1.4f});
            glow.setPosition(ball.pos);
            glow.setFillColor(glowColor);
            window.draw(glow);
            
            sf::CircleShape circle(ball.size);
            circle.setOrigin({ball.size, ball.size});
            circle.setPosition(ball.pos);
            circle.setFillColor(ball.color);
            window.draw(circle);
        }
        
        window.display();
    }

    return 0;
}
