#include <SFML/Graphics.hpp>

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Bouncing Circle - Basic");
    window.setFramerateLimit(60);

    float radius = 20.f;
    sf::Vector2f position(200.f, 150.f);
    sf::Vector2f velocity(220.f, 160.f); 
    sf::Clock clock;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (!event) continue;
            if (auto* closeEvent = event->getIf<sf::Event::Closed>()) {
                window.close();
            }
        }

        const float dt = clock.restart().asSeconds();
        const auto size = window.getSize();
        const float w = static_cast<float>(size.x);
        const float h = static_cast<float>(size.y);

        
        position += velocity * dt;

        if (position.x - radius <= 0.f) {
            position.x = radius;
            velocity.x = -velocity.x;
        } else if (position.x + radius >= w) {
            position.x = w - radius;
            velocity.x = -velocity.x;
        }

        
        if (position.y - radius <= 0.f) {
            position.y = radius;
            velocity.y = -velocity.y;
        } else if (position.y + radius >= h) {
            position.y = h - radius;
            velocity.y = -velocity.y;
        }

        window.clear(sf::Color(20, 20, 40));
        sf::CircleShape circle(radius);
        circle.setOrigin({radius, radius});
        circle.setPosition(position);
        circle.setFillColor(sf::Color(100, 200, 255));
        window.draw(circle);
        window.display();
    }

    return 0;
}