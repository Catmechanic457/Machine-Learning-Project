#include "lib/NeuralNetwork.hpp"
#include <iostream>
#include <string.h>

int main() {
    using namespace nn;
    std::cout << "Program to output a visual representation of a Neural Network" << endl;

    std::cout << "Enter a file path:\n > ../";
    std::string file_name;
    std::cin >> file_name;
    cout << endl;

    std::cout << "Enter a network id:\n > ";
    std::string id;
    std::cin >> id;
    cout << endl;

    Storage storage("../" + file_name);
    storage.read_data();
    auto values = storage.read_values(id);

    Network network(values.shape);
    network.load_values(values);

    sf::RenderWindow window(sf::VideoMode(1000,1000), "Network", sf::Style::Close);
    Display display(window.getSize().x, window.getSize().y);
    sf::Texture t = display.plot_network(network);
    sf::Sprite s(t);
    window.draw(s);
    window.display();
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }

    return 0;
}