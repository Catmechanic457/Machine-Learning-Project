#include "lib/Interactables.hpp"

int main() {
    using namespace gui;
    
    assets::load_assets();

    // Create window
    sf::RenderWindow window(sf::VideoMode(500,500), "GUI Test", sf::Style::Close);
    // Create cell grid with the same size
    CellGrid cell_grid(window.getSize().x, window.getSize().y, 32);

    // Define interactables
    Slider<unsigned int> slider(4,0,100,50);
    TextBox slider_label(4);

    PushButton push_button;
    TextBox button_label(4);

    CheckBox check_box;
    TextBox check_box_label(4);

    // Set positions
    slider.setPosition(sf::Vector2f(128,32), cell_grid);
    slider_label.setPosition(sf::Vector2f(0,32));

    push_button.setPosition(sf::Vector2f(128,64), cell_grid);
    button_label.setPosition(sf::Vector2f(0,64));

    check_box.setPosition(sf::Vector2f(128,96), cell_grid);
    check_box_label.setPosition(sf::Vector2f(0,96));

    sf::Vector2i mouse_pos;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed : 
                    window.close();
                    goto exit_window;

                case sf::Event::MouseMoved :
                    // Clamp to avoid seg_fault
                    mouse_pos = sf::Vector2i(std::clamp<int>(sf::Mouse::getPosition(window).x, 0, window.getSize().x), std::clamp<int>(sf::Mouse::getPosition(window).y, 0, window.getSize().y));
                    break;
                default: break;
            }
            cell_grid.handle(event, mouse_pos);
        }
        window.clear(sf::Color::White);

        slider_label.set_text("Slider: " + std::to_string(slider.value));
        button_label.set_text((push_button.value ? "Button: True" : "Button: False"));
        check_box_label.set_text((check_box.value ? "Check Box: True" : "Check Box: False"));

        // Render interactables
        slider.render();
        push_button.render();
        check_box.render();

        // Draw sprites
        window.draw(slider_label);
        window.draw(slider);
        window.draw(slider.handle());
        
        window.draw(button_label);
        window.draw(push_button);

        window.draw(check_box_label);
        window.draw(check_box);

        window.display();
    }
    exit_window:
    return 0;
}