#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

// --------------------------------------------------
// GLOBAL SPEED FACTOR
// --------------------------------------------------
float speedFactor = 5.0f;

// --------------------------------------------------
// AESTHETIC COLOR PALETTE
// --------------------------------------------------
vector<sf::Color> aestheticPalette = {
    sf::Color(255, 0, 128),    // Hot Magenta
    sf::Color(255, 20, 147),   // Deep Pink
    sf::Color(186, 85, 211),   // Orchid Purple
    sf::Color(142, 45, 226),   // Electric Purple
    sf::Color(0, 206, 209),    // Turquoise
    sf::Color(64, 224, 208),   // Bright Cyan
    sf::Color(0, 245, 255),    // Neon Cyan
    sf::Color(178, 69, 146)    // Rose Magenta
};

sf::Color getRandomAestheticColor() {
    return aestheticPalette[rand() % aestheticPalette.size()];
}

// --------------------------------------------------
// BASIC PARTICLE
// --------------------------------------------------
struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius;
    float mass;
    int id;
    sf::Color color; // Warna unik tiap partikel

    Particle(float x, float y, int particleId) {
        pos = sf::Vector2f(x, y);
        vel = sf::Vector2f((rand() % 100 - 50) / 5.f, (rand() % 100 - 50) / 5.f);
        radius = 6.f;
        mass = radius * 10.f;
        id = particleId;
        color = getRandomAestheticColor(); // Random aesthetic color
    }

    void update(float dt, sf::Vector2u win) {
        pos += vel * dt * speedFactor;

        if (pos.x < radius) { pos.x = radius; vel.x *= -1; }
        if (pos.x > win.x - radius) { pos.x = win.x - radius; vel.x *= -1; }
        if (pos.y < radius) { pos.y = radius; vel.y *= -1; }
        if (pos.y > win.y - radius) { pos.y = win.y - radius; vel.y *= -1; }
    }

    void draw(sf::RenderWindow& win) {
        sf::CircleShape s(radius);
        s.setOrigin(sf::Vector2f(radius, radius));
        s.setPosition(pos);
        s.setFillColor(color); // Pakai warna unik partikel
        win.draw(s);
    }
};

// --------------------------------------------------
// BASIC RECT (untuk Quadtree)
// --------------------------------------------------
struct Rect {
    float x, y, w, h;

    bool contains(const sf::Vector2f& p) const {
        return p.x >= x && p.x <= x + w &&
               p.y >= y && p.y <= y + h;
    }

    bool intersects(const Rect& r) const {
        return !(x + w < r.x || r.x + r.w < x ||
                 y + h < r.y || r.y + r.h < y);
    }
};

// --------------------------------------------------
// QUADTREE - ]untuk menyimpan particle index
// --------------------------------------------------
struct Quadtree {
    Rect boundary;
    int capacity;
    vector<int> particleIndices; // simpan index particle, bukan posisi

    bool divided = false;
    Quadtree *NE = nullptr, *NW = nullptr, *SE = nullptr, *SW = nullptr;

    Quadtree(Rect b, int cap) : boundary(b), capacity(cap) {}

    bool insert(const sf::Vector2f& p, int particleIdx) {
        if (!boundary.contains(p)) return false;

        if (particleIndices.size() < capacity) {
            particleIndices.push_back(particleIdx);
            return true;
        }

        if (!divided) subdivide();

        return NE->insert(p, particleIdx) || NW->insert(p, particleIdx) ||
               SE->insert(p, particleIdx) || SW->insert(p, particleIdx);
    }

    void subdivide() {
        float hw = boundary.w / 2;
        float hh = boundary.h / 2;

        NE = new Quadtree({boundary.x + hw, boundary.y, hw, hh}, capacity);
        NW = new Quadtree({boundary.x, boundary.y, hw, hh}, capacity);
        SE = new Quadtree({boundary.x + hw, boundary.y + hh, hw, hh}, capacity);
        SW = new Quadtree({boundary.x, boundary.y + hh, hw, hh}, capacity);

        divided = true;
    }

    void queryIndices(const Rect& area, vector<int>& found) {
        if (!boundary.intersects(area)) return;

        for (auto idx : particleIndices)
            found.push_back(idx);

        if (!divided) return;

        NE->queryIndices(area, found);
        NW->queryIndices(area, found);
        SE->queryIndices(area, found);
        SW->queryIndices(area, found);
    }

    void draw(sf::RenderWindow& win) {
        sf::RectangleShape box(sf::Vector2f(boundary.w, boundary.h));
        box.setPosition(sf::Vector2f(boundary.x, boundary.y));
        box.setFillColor(sf::Color::Transparent);
        box.setOutlineColor(sf::Color(50, 50, 50));
        box.setOutlineThickness(1);
        win.draw(box);

        if (divided) {
            NE->draw(win);
            NW->draw(win);
            SE->draw(win);
            SW->draw(win);
        }
    }

    ~Quadtree() {
        if (divided) {
            delete NE;
            delete NW;
            delete SE;
            delete SW;
        }
    }
};

// --------------------------------------------------
// SIMPLE COLLISION RESOLUTION
// --------------------------------------------------
void resolveCollision(Particle& a, Particle& b) {
    float dx = b.pos.x - a.pos.x;
    float dy = b.pos.y - a.pos.y;
    float dist = sqrt(dx*dx + dy*dy);

    float minDist = a.radius + b.radius;
    if (dist == 0 || dist >= minDist) return;

    float nx = dx / dist;
    float ny = dy / dist;

    float rvx = b.vel.x - a.vel.x;
    float rvy = b.vel.y - a.vel.y;

    float velAlongNormal = rvx*nx + rvy*ny;
    if (velAlongNormal > 0) return;

    float restitution = 1.0f;

    float j = -(1 + restitution) * velAlongNormal;
    j /= (1 / a.mass + 1 / b.mass);

    sf::Vector2f impulse = {j * nx, j * ny};

    a.vel.x -= impulse.x / a.mass;
    a.vel.y -= impulse.y / a.mass;
    b.vel.x += impulse.x / b.mass;
    b.vel.y += impulse.y / b.mass;

    float overlap = minDist - dist;
    a.pos.x -= nx * overlap * 0.5f;
    a.pos.y -= ny * overlap * 0.5f;
    b.pos.x += nx * overlap * 0.5f;
    b.pos.y += ny * overlap * 0.5f;
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main() {
    sf::RenderWindow window(sf::VideoMode({1200, 600}), "Particles + Quadtree + Cursor Pull + UI");
    window.setFramerateLimit(60);

    vector<Particle> particles;
    int particleIdCounter = 0;
    for (int i = 0; i < 300; i++)
        particles.emplace_back((float)(rand() % 1200), (float)(rand() % 600), particleIdCounter++);

    bool useQuadtree = true;
    int collisionChecks = 0; // counter untuk debug

    // Load font
    sf::Font font;
    if (!font.openFromFile("Montserrat-BlackItalic.otf")) {
        cerr << "Error loading arial.ttf\n";
        return -1;
    }

    sf::Text uiText(font);
    uiText.setCharacterSize(18);
    uiText.setFillColor(sf::Color::White);
    uiText.setPosition(sf::Vector2f(10.f, 10.f));

    sf::Clock clock;
    float fps = 0.f;

    while (window.isOpen()) {
        sf::Time elapsed = clock.restart();
        float dt = elapsed.asSeconds();
        fps = 1.f / dt;

        while (auto event = window.pollEvent()) {
            if (auto* closeEvent = event->getIf<sf::Event::Closed>())
                window.close();

            if (auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                // Toggle quadtree
                if (keyEvent->code == sf::Keyboard::Key::Num1) useQuadtree = false;
                if (keyEvent->code == sf::Keyboard::Key::Num2) useQuadtree = true;

                // Speed control
                if (keyEvent->code == sf::Keyboard::Key::Up) {
                    speedFactor += 0.2f;
                    if (speedFactor > 10.f) speedFactor = 10.f;
                }
                if (keyEvent->code == sf::Keyboard::Key::Down) {
                    speedFactor -= 0.2f;
                    if (speedFactor < 0.f) speedFactor = 0.f;
                }

                // Add/remove particles
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    for (int i = 0; i < 100; i++)
                        particles.emplace_back((float)(rand() % window.getSize().x), 
                                             (float)(rand() % window.getSize().y), 
                                             particleIdCounter++);
                }
                if (keyEvent->code == sf::Keyboard::Key::W) {
                    for (int i = 0; i < 100 && !particles.empty(); i++)
                        particles.pop_back();
                }
            }
        }

        // Update particles
        for (auto& p : particles)
            p.update(dt, window.getSize());

        // ============================================
        // COLLISION DETECTION 
        // ============================================
        collisionChecks = 0;
        
        if (useQuadtree) {
            // MODE QUADTREE - Optimized O(n log n)
            Rect world{0.f, 0.f, (float)window.getSize().x, (float)window.getSize().y};
            Quadtree qt(world, 4); // capacity 4 untuk hasil optimal
            
            // Build quadtree dengan index particle
            for (size_t i = 0; i < particles.size(); i++)
                qt.insert(particles[i].pos, i);
            
            // Check collision hanya dengan neighbor terdekat
            for (size_t i = 0; i < particles.size(); i++) {
                Particle& p = particles[i];
                float searchRadius = p.radius * 4; // area pencarian
                
                Rect searchArea{
                    p.pos.x - searchRadius, 
                    p.pos.y - searchRadius,
                    searchRadius * 2, 
                    searchRadius * 2
                };
                
                vector<int> nearbyIndices;
                qt.queryIndices(searchArea, nearbyIndices);
                
                // Check collision hanya dengan nearby particles
                for (int j : nearbyIndices) {
                    if (j > (int)i) { // hindari duplikasi check
                        resolveCollision(particles[i], particles[j]);
                        collisionChecks++;
                    }
                }
            }
            
        } else {
            // MODE BRUTE FORCE - O(n²)
            for (size_t i = 0; i < particles.size(); i++) {
                for (size_t j = i + 1; j < particles.size(); j++) {
                    resolveCollision(particles[i], particles[j]);
                    collisionChecks++;
                }
            }
        }

        // Build quadtree untuk cursor (tetap seperti sebelumnya)
        Rect world{0.f, 0.f, (float)window.getSize().x, (float)window.getSize().y};
        Quadtree cursorQt(world, 6);
        for (size_t i = 0; i < particles.size(); i++)
            cursorQt.insert(particles[i].pos, i);

        // Cursor logic (tidak berubah)
        sf::Vector2i mouseI = sf::Mouse::getPosition(window);
        sf::Vector2f mouse((float)mouseI.x, (float)mouseI.y);
        float cursorRadius = 50.f;

        // Pull particles toward cursor
        for (auto& p : particles) {
            float dx = mouse.x - p.pos.x;
            float dy = mouse.y - p.pos.y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist > 0 && dist < cursorRadius) {
                float force = (cursorRadius - dist) * 1.0f;
                p.vel.x += dx / dist * force;
                p.vel.y += dy / dist * force;
            }
        }

        // Draw
        window.clear(sf::Color(20, 20, 30));
        if (useQuadtree) cursorQt.draw(window);

        for (auto& p : particles)
            p.draw(window);

        // Draw cursor
        sf::CircleShape cursor(5.f);
        cursor.setOrigin(sf::Vector2f(5.f, 5.f));
        cursor.setFillColor(sf::Color::Yellow);
        cursor.setPosition(mouse);
        window.draw(cursor);

        // Draw UI dengan info collision checks
        std::ostringstream speedStream;
        speedStream << std::fixed << std::setprecision(1) << speedFactor;

        uiText.setString(
            "FPS: " + to_string(int(fps)) +
            " | Particles: " + to_string(particles.size()) +
            " | Mode: " + (useQuadtree ? "Quadtree" : "Brute Force") +
            " | Speed: " + speedStream.str() +
            " | Collision Checks: " + to_string(collisionChecks)
        );
        window.draw(uiText);

        window.display();
    }

    return 0;
}