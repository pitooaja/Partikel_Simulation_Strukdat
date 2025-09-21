#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <cmath>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple Particle class with bouncing physics
class Particle {
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float life;
    float maxLife;
    float size;
    
    Particle(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float l, float s) 
        : position(pos), velocity(vel), color(col), life(l), maxLife(l), size(s) {}
    
    void update(float deltaTime, float windowWidth, float windowHeight) {
        // Update position
        position += velocity * deltaTime;
        life -= deltaTime;
        
        // Wall collision detection and bouncing
        if (position.x - size <= 0 || position.x + size >= windowWidth) {
            velocity.x = -velocity.x;  // Bounce horizontally
            // Keep particle inside window
            position.x = std::max(size, std::min(windowWidth - size, position.x));
        }
        
        if (position.y - size <= 0 || position.y + size >= windowHeight) {
            velocity.y = -velocity.y;  // Bounce vertically
            // Keep particle inside window
            position.y = std::max(size, std::min(windowHeight - size, position.y));
        }
        
        // Fade out effect
        float alpha = (life / maxLife) * 255;
        color.a = static_cast<unsigned char>(alpha);
    }
    
    bool isDead() const { return life <= 0; }
};

// Simple Particle System with bouncing
class ParticleSystem {
private:
    std::vector<Particle> particles;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> dis;
    
public:
    ParticleSystem() : gen(rd()), dis(-1.0f, 1.0f) {}
    
    void addExplosion(sf::Vector2f position, int count = 30) {
        for (int i = 0; i < count; ++i) {
            float angle = (2.0f * M_PI * i) / count;
            float speed = 80.0f + dis(gen) * 40.0f;
            
            sf::Vector2f vel(cos(angle) * speed, sin(angle) * speed);
            sf::Color color = sf::Color::Red;
            
            // Randomize color
            if (dis(gen) > 0) color = sf::Color::Yellow;
            if (dis(gen) > 0.5f) color = sf::Color(255, 165, 0);
            
            float life = 1.0f + dis(gen) * 0.5f;
            float size = 3.0f + dis(gen) * 2.0f;
            
            particles.emplace_back(position, vel, color, life, size);
        }
    }
    
    void addSparkle(sf::Vector2f position) {
        sf::Vector2f vel(dis(gen) * 40.0f, dis(gen) * 40.0f);
        sf::Color color = sf::Color::White;
        
        if (dis(gen) > 0) color = sf::Color::Cyan;
        if (dis(gen) > 0.5f) color = sf::Color::Magenta;
        
        float life = 0.5f + dis(gen) * 0.3f;
        float size = 2.0f + dis(gen) * 1.0f;
        
        particles.emplace_back(position, vel, color, life, size);
    }
    
    void update(float deltaTime, float windowWidth, float windowHeight) {
        // Update particles with wall collision
        for (auto& particle : particles) {
            particle.update(deltaTime, windowWidth, windowHeight);
        }
        
        // Remove dead particles
        for (auto it = particles.begin(); it != particles.end();) {
            if (it->isDead()) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void draw(sf::RenderWindow& window) {
        for (const auto& particle : particles) {
            sf::CircleShape shape(particle.size);
            shape.setPosition(particle.position);
            shape.setFillColor(particle.color);
            shape.setOrigin(sf::Vector2f(particle.size, particle.size));
            window.draw(shape);
        }
    }
    
    void clear() {
        particles.clear();
    }
    
    size_t getParticleCount() const {
        return particles.size();
    }
};

int main() {
    // Create window
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Bouncing Particle System!");
    window.setFramerateLimit(60);
    
    // Get window dimensions
    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    
    // Create particle system
    ParticleSystem particles;
    
    // Clock for delta time
    sf::Clock clock;
    
    // Main loop
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        

        
        // Handle events - SFML 3.0 style
        while (auto event = window.pollEvent()) {
            // Check if event is valid
            if (!event) continue;
            
            // Handle different event types
            if (auto* closeEvent = event->getIf<sf::Event::Closed>()) {
                window.close();
            }
            
            if (auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(
                        sf::Mouse::getPosition(window)
                    );
                    
                    // Add explosion
                    particles.addExplosion(mousePos);
                    
                    // Add some sparkles
                    for (int i = 0; i < 5; ++i) {
                        particles.addSparkle(mousePos);
                    }
                }
            }
            
            if (auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::R) {
                    particles.clear();
                }
            }
        }
        
        // Update particles with wall collision
        particles.update(deltaTime, windowWidth, windowHeight);
        
        // Clear window
        window.clear(sf::Color(20, 20, 40));
        
        // Draw particles
        particles.draw(window);
        
        // Display
        window.display();
    }
    
    return 0;
}       