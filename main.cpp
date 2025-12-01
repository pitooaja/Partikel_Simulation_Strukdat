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
// BASIC PARTICLE
// --------------------------------------------------
struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius;
    float mass;

    Particle(float x, float y) {
        pos = sf::Vector2f(x, y);
        vel = sf::Vector2f((rand() % 100 - 50) / 5.f, (rand() % 100 - 50) / 5.f);
        radius = 6.f;
        mass = radius * 10.f;
    }

    void update(float dt, sf::Vector2u win) {
        pos += vel * dt * speedFactor;

        if (pos.x < radius) { pos.x = radius; vel.x *= -1; }
        if (pos.x > win.x - radius) { pos.x = win.x - radius; vel.x *= -1; }
        if (pos.y < radius) { pos.y = radius; vel.y *= -1; }
        if (pos.y > win.y - radius) { pos.y = win.y - radius; vel.y *= -1; }
    }

    void draw(sf::RenderWindow& win, sf::Color c) {
        sf::CircleShape s(radius);
        s.setOrigin(sf::Vector2f(radius, radius));
        s.setPosition(pos);
        s.setFillColor(c);
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
// QUADTREE
// --------------------------------------------------
struct Quadtree {
    Rect boundary;
    int capacity;
    vector<sf::Vector2f> points;

    bool divided = false;
    Quadtree *NE = nullptr, *NW = nullptr, *SE = nullptr, *SW = nullptr;

    Quadtree(Rect b, int cap) : boundary(b), capacity(cap) {}

    bool insert(const sf::Vector2f& p) {
        if (!boundary.contains(p)) return false;

        if (points.size() < capacity) {
            points.push_back(p);
            return true;
        }

        if (!divided) subdivide();

        return NE->insert(p) || NW->insert(p) ||
               SE->insert(p) || SW->insert(p);
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

    void query(const Rect& area, vector<sf::Vector2f>& found) {
        if (!boundary.intersects(area)) return;

        for (auto& p : points)
            if (area.contains(p))
                found.push_back(p);

        if (!divided) return;

        NE->query(area, found);
        NW->query(area, found);
        SE->query(area, found);
        SW->query(area, found);
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
    for (int i = 0; i < 300; i++)
        particles.emplace_back((float)(rand() % 1200), (float)(rand() % 600));

    bool useQuadtree = true;

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
                        particles.emplace_back((float)(rand() % window.getSize().x), (float)(rand() % window.getSize().y));
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

        // Brute force collision
        for (size_t i = 0; i < particles.size(); i++)
            for (size_t j = i + 1; j < particles.size(); j++)
                resolveCollision(particles[i], particles[j]);

        // Build quadtree
        Rect world{0.f, 0.f, (float)window.getSize().x, (float)window.getSize().y};
        Quadtree qt(world, 6);
        for (auto& p : particles)
            qt.insert(p.pos);

        // Cursor
        sf::Vector2i mouseI = sf::Mouse::getPosition(window);
        sf::Vector2f mouse((float)mouseI.x, (float)mouseI.y);
        float radius = 50.f;

        vector<sf::Vector2f> neighbors;
        if (useQuadtree) {
            Rect area{mouse.x - radius, mouse.y - radius, radius * 2, radius * 2};
            qt.query(area, neighbors);
        } else {
            for (auto& p : particles) {
                float dx = p.pos.x - mouse.x;
                float dy = p.pos.y - mouse.y;
                if (sqrt(dx*dx + dy*dy) <= radius)
                    neighbors.push_back(p.pos);
            }
        }

        // Pull particles toward cursor
        for (auto& p : particles) {
            float dx = mouse.x - p.pos.x;
            float dy = mouse.y - p.pos.y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist > 0 && dist < radius) {
                float force = (radius - dist) * 1.0f;
                p.vel.x += dx / dist * force;
                p.vel.y += dy / dist * force;
            }
        }

        // Draw
        window.clear(sf::Color(20, 20, 30));
        if (useQuadtree) qt.draw(window);

        for (auto& p : particles)
            p.draw(window, sf::Color(200, 200, 200));

        // Draw cursor
        sf::CircleShape cursor(5.f);
        cursor.setOrigin(sf::Vector2f(5.f, 5.f));
        cursor.setFillColor(sf::Color::Yellow);
        cursor.setPosition(mouse);
        window.draw(cursor);

        // Draw UI with formatted speed
        std::ostringstream speedStream;
        speedStream << std::fixed << std::setprecision(1) << speedFactor;

        uiText.setString(
            "FPS: " + to_string(int(fps)) +
            " | Particles: " + to_string(particles.size()) +
            " | Mode: " + (useQuadtree ? "Quadtree" : "Brute Force") +
            " | Speed: " + speedStream.str()
        );
        window.draw(uiText);

        window.display();
    }

    return 0;
}