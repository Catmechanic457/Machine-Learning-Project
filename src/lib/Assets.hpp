#ifndef ASSETS_H
#define ASSETS_H

#include <SFML/Graphics.hpp>
#include <iostream>

#define TEXTURES_PATH "../assets/textures/"
#define FONTS_PATH "../assets/fonts/"

namespace assets {
    using namespace sf;
    namespace textures {
        namespace bot {
            Texture body;
            Texture sonar;
        }
        namespace stage {
            Texture spawnpoint;
            namespace evaluation {
                Texture possible;
                Texture impossible;
            }
        }
        namespace gui {
            namespace tick_box {
                Texture checked;
                Texture unchecked;
            }
            namespace slider {
                Texture left;
                Texture centre;
                Texture right;
                Texture handle;
            }
            namespace button {
                Texture pushed;
                Texture neutral;
            }
            namespace text_box {
                Texture left;
                Texture centre;
                Texture right;
            }
            namespace progress_bar {
                Texture empty;
                Texture filled;
                Texture success;
                Texture failed;
            }
        }
    }

    namespace fonts {
        Font arial;
    }

    void load_assets() {

        std::cout << "Loading Assets...\n";

        bool success = true;
        success = success and textures::bot::body.loadFromFile(TEXTURES_PATH "bot/body.png");
        success = success and textures::bot::sonar.loadFromFile(TEXTURES_PATH "bot/sonar.png");


        success = success and textures::stage::spawnpoint.loadFromFile(TEXTURES_PATH "stage/spawnpoint.png");

        success = success and textures::stage::evaluation::possible.loadFromFile(TEXTURES_PATH "stage/possible.png");
        success = success and textures::stage::evaluation::impossible.loadFromFile(TEXTURES_PATH "stage/impossible.png");


        success = success and textures::gui::tick_box::checked.loadFromFile(TEXTURES_PATH "gui/box_checked.png");
        success = success and textures::gui::tick_box::unchecked.loadFromFile(TEXTURES_PATH "gui/box_unchecked.png");

        success = success and textures::gui::slider::left.loadFromFile(TEXTURES_PATH "gui/slider_rail_l.png");
        success = success and textures::gui::slider::centre.loadFromFile(TEXTURES_PATH "gui/slider_rail.png");
        success = success and textures::gui::slider::right.loadFromFile(TEXTURES_PATH "gui/slider_rail_r.png");
        success = success and textures::gui::slider::handle.loadFromFile(TEXTURES_PATH "gui/slider_handle.png");

        success = success and textures::gui::button::pushed.loadFromFile(TEXTURES_PATH "gui/push_button_pushed.png");
        success = success and textures::gui::button::neutral.loadFromFile(TEXTURES_PATH "gui/push_button.png");

        success = success and textures::gui::text_box::left.loadFromFile(TEXTURES_PATH "gui/text_box_l.png");
        success = success and textures::gui::text_box::centre.loadFromFile(TEXTURES_PATH "gui/text_box.png");
        success = success and textures::gui::text_box::right.loadFromFile(TEXTURES_PATH "gui/text_box_r.png");

        success = success and textures::gui::progress_bar::empty.loadFromFile(TEXTURES_PATH "gui/progress_bar_empty.png");
        success = success and textures::gui::progress_bar::filled.loadFromFile(TEXTURES_PATH "gui/progress_bar_filled.png");
        success = success and textures::gui::progress_bar::success.loadFromFile(TEXTURES_PATH "gui/progress_bar_success.png");
        success = success and textures::gui::progress_bar::failed.loadFromFile(TEXTURES_PATH "gui/progress_bar_failed.png");


        success = success and fonts::arial.loadFromFile(FONTS_PATH "arial.ttf");

        if (!success) {
            throw std::runtime_error("Unable to load file(s). Please add the files or re-install.");
        }
        std::cout << "Complete." << std::endl;
    }
}

#endif