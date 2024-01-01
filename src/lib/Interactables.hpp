#ifndef INTERACTABLES_H
#define INTERACTABLES_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string.h>
#include "Assets.hpp"

namespace gui {
    using namespace assets::textures::gui;

    const sf::Vector2u unit_size(32,32); // Size of each texture file (in pixels)

    class CellGrid;

    class Interactable : public sf::Sprite {
        protected:
        const sf::Vector2f& _pos;
        sf::Texture _texture;
        sf::Vector2u _bounding_box;
        public:
        std::vector<unsigned int> cell_index;
        Interactable(sf::Vector2u bb) : _pos(getPosition()), _bounding_box(bb) {}

        enum Action {
            ON_TARGETED_CLICK,
            ON_TARGETED_RELEASE,
            CLICK_TRUE,
            CLICK_FALSE,
            IN_BOUNDS,
            OUTSIDE_BOUNDS
        };

        /**
         * \return `true` if the position lies in the bounds
        */
        bool within_bounds(sf::Vector2f tpos_) const {
            // Make more readable using std::clamp == ect?
            return tpos_.x >= _pos.x and tpos_.y >= _pos.y and tpos_.x <= (_pos.x + _bounding_box.x) and tpos_.y <= (_pos.y + _bounding_box.y);
        }
        /**
         * \return The size of the bounding box
        */
        sf::Vector2u bounding_box() const {return _bounding_box;}
        void setPosition(sf::Vector2f, CellGrid&);
        /**
         * \brief Default actions function
        */
        virtual void actions(const sf::Vector2i& mouse_pos_, std::vector<Action> actions_) {}
        /**
         * \brief Default render function
        */
        virtual void render() {}
    };

    struct Cell {
        std::vector<Interactable*> members;
        ~Cell() {
            for (auto p : members) {
                delete p; //TODO : Compiler warn [-Wdelete-non-virtual-dtor]
            }
        }
    };

    class CellGrid {
        private:
        unsigned int _winx;
        unsigned int _winy;
        unsigned int _cell_size;
        unsigned int _cell_count_x;
        unsigned int _cell_count_y;
        std::vector<Cell> _cells;

        std::vector<Interactable*>& targets(const sf::Vector2i& mouse_pos) {
            // Locate target cell
            auto& target_cell = _cells[cell_index(sf::Vector2f(mouse_pos))];
            return target_cell.members;
        }
        unsigned int cell_index(sf::Vector2f pos_) {
            int x = std::clamp<float>(pos_.x/_cell_size, 0, _cell_count_x);
            int y = std::clamp<float>(pos_.y/_cell_size, 0, _cell_count_y);
            return (y*_cell_count_y) + x;
        }


        public:
        CellGrid(unsigned int winx_, unsigned int winy_, unsigned int cell_size_) :
        _winx(winx_),
        _winy(winy_),
        _cell_size(cell_size_),
        _cell_count_x((winx_ + cell_size_ - 1) / cell_size_),
        _cell_count_y((winy_ + cell_size_ - 1) / cell_size_),
        _cells(std::vector<Cell>(_cell_count_x*_cell_count_y))
        {}

        void move(Interactable* interactable) {
            // Remove iteractable from old cells
            for (auto index : interactable->cell_index) {
                auto& cell_members = _cells[index].members;
                for (unsigned int i = 0; i < cell_members.size(); i++) {
                    // Check if addresses match
                    if (cell_members[i] == interactable) {
                        cell_members.erase(cell_members.begin()+i);
                        // break; could be added here, but could cause issues
                        // if the same obj apears multiple times
                    }
                }
            }
            // Add to new cells
            auto bb = interactable->bounding_box();
            auto pos = interactable->getPosition();
            // Half a unit is added as a "buffer" to ensure the mouse does
            // not leave the bounds of the interactable at the same time
            // as it leaves the cell, causing issues
            unsigned int index1 = cell_index(sf::Vector2f(pos.x - (unit_size.x/2), pos.y - (unit_size.y/2))); // Top left
            unsigned int index2 = cell_index(sf::Vector2f(pos.x + bb.x + (unit_size.x/2), pos.y + bb.y + (unit_size.x/2))); // Bottom Right
            std::vector<unsigned int> occupied_cells;
            unsigned int x1 = (index1 % _cell_count_x);
            unsigned int x2 = (index2 % _cell_count_x);
            unsigned int y1 = (index1 / _cell_count_x);
            unsigned int y2 = (index2 / _cell_count_x);
            for (unsigned int y = y1; y <= y2; y++) {
                for (unsigned int x = x1; x <= x2; x++) {
                    occupied_cells.push_back((y*_cell_count_x) + x);
                    _cells[(y*_cell_count_x) + x].members.push_back(interactable);
                }
            }
            interactable->cell_index = occupied_cells;
        }

        void handle(sf::Event event, sf::Vector2i mouse_pos) {
            // Actions carried out by the foucsed cell member
            // (where the mouse is within their bounding box)
            std::vector<Interactable::Action> focused_actions;

            // Actions carried out by non-focused members
            // These ramain constant
            std::vector<Interactable::Action> unfocused_actions {
                Interactable::OUTSIDE_BOUNDS
            };

            focused_actions.push_back(Interactable::IN_BOUNDS);

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Button::Left) {
                    focused_actions.push_back(Interactable::ON_TARGETED_CLICK);
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Button::Left) {
                    focused_actions.push_back(Interactable::ON_TARGETED_RELEASE);
                }
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                focused_actions.push_back(Interactable::CLICK_TRUE);
            }
            else {focused_actions.push_back(Interactable::CLICK_FALSE);}

            auto& cell_members = targets(mouse_pos);
            for (auto& member : cell_members) {
                if (member->within_bounds(sf::Vector2f(mouse_pos))) {
                    member->actions(mouse_pos, focused_actions);
                }
                else {
                    member->actions(mouse_pos, unfocused_actions);
                }
            }
        }
    };

    /**
     * \brief Sets the Sprite's position and moves it to the corrosponding cell
     * \param pos_ New position
     * \param cell_grid_ Cell grid to be repositioned in
    */
    void Interactable::setPosition(sf::Vector2f pos_, CellGrid& cell_grid_) {
        Interactable::Sprite::Transformable::setPosition(pos_);
        cell_grid_.move(this);
    }

    template <typename T>
    class Slider : public Interactable {
        const unsigned int _width_units;
        const T _min;
        const T _max;

        sf::Texture _rail_sized;
        sf::Sprite _handle;

        void size_rail() {

            // Stitches the 3 rail textures into one
            // complete texture to apply to the sprite
            sf::RenderTexture t;

            // Draw left side
            t.create(unit_size.x*_width_units, unit_size.y);
            sf::Sprite rail_left(slider::left);
            t.draw(rail_left);

            // Draw the middle at the correct length
            // (no middle units if width = 2)
            if (_width_units > 2) {
                sf::Sprite rail(slider::centre, sf::IntRect(sf::Vector2i(0,0), sf::Vector2i(unit_size.x*(_width_units-2), unit_size.y)));
                rail.setPosition(sf::Vector2f(unit_size.x,0));
                t.draw(rail);
            }

            // Draw right side
            sf::Sprite rail_right(slider::right);
            rail_right.setPosition(sf::Vector2f(unit_size.x*(_width_units-1), 0));
            t.draw(rail_right);
            t.display();

            // Texture needs to be available to refrance when drawn
            _rail_sized = t.getTexture();
        }

        void position_handle() {
            double progress = (value - _min)/static_cast<double>(_max - _min); // Cast to double to ensure floating-point division
            _handle.setPosition(_pos.x + (progress*(_width_units-1)*unit_size.x) - (unit_size.x/2) + unit_size.x/2, _pos.y);
        }

        public:
        T value;
        Slider<T>(unsigned int width_, T min_, T max_, T default_) :
        Interactable(sf::Vector2u(std::clamp<unsigned int>(width_, 2, 16)*32, 32)),
        _width_units(std::clamp<unsigned int>(width_, 2, 16)), // Width is at least 2 units (Left + Right)
        _min(min_),
        _max(max_),
        value(default_)
        {
            size_rail();
            _handle.setTexture(slider::handle);
        }
        Slider<T>(unsigned int width_, T min_, T max_) : Slider(width_, min_, max_, min_) {}

        void actions(const sf::Vector2i& mouse_pos_, std::vector<Action> actions_) override {
            int mouse_x = std::clamp<int>(mouse_pos_.x, _pos.x + (unit_size.x/2), _pos.x + ((_width_units-0.5) * unit_size.x));
            for (auto act : actions_) {
                switch (act) {
                    case CLICK_TRUE:
                        // Clamp mouse to avoid possible overflow errors
                        value = static_cast<T>(((mouse_x-(unit_size.x/2)) - _pos.x)*(_max-_min)/((_width_units-1) * unit_size.x) + _min);

                        // Clamp value in case of rounding errors
                        value = std::clamp<T>(value, _min, _max);
                        break;
                    default:
                        break;
                }
            }
        }

        void render() override {
            position_handle();
            setTexture(_rail_sized);
        }
        /**
         * \return The slider's handle to be drawn to a window
        */
        const sf::Sprite& handle() {return _handle;}
        /**
         * \return The slider's minimum value
        */
        const T& get_min() {return _min;}
        /**
         * \return The slider's maximum value
        */
        const T& get_max() {return _max;}
    };

    class CheckBox : public Interactable {
        public:
        bool value;
        CheckBox(bool default_) : Interactable(sf::Vector2u(32,32)), value(default_) {}
        CheckBox() : CheckBox(false) {}

        void render() override {
            _texture = (value ? tick_box::checked : tick_box::unchecked);
            setTexture(_texture);
        }

        void actions(const sf::Vector2i& mouse_pos, std::vector<Action> actions) override {
            for (auto act : actions) {
                switch (act) {
                    case ON_TARGETED_CLICK:
                        value = !value;
                        break;
                    default: break;
                }
            }
        }
    };

    class PushButton : public Interactable {
        public:
        bool value;
        PushButton() : Interactable(sf::Vector2u(32,32)), value(false) {}
        void render() override {
            _texture = (value ? button::pushed : button::neutral);
            setTexture(_texture);
        }
        void actions(const sf::Vector2i& mouse_pos, std::vector<Action> actions_) override {
            for (auto act : actions_) {
                switch (act) {
                case CLICK_TRUE:
                    value = true;
                    break;
                case CLICK_FALSE:
                    value = false;
                    break;
                case OUTSIDE_BOUNDS:
                    value = false;
                    break;
                default: break;
                }
            }
        }
    };

    class TextBox : public sf::Sprite {
        unsigned int _width_units;
        unsigned int _char_size;

        std::string _text;

        sf::Texture _t_text;
        sf::Texture _t_box_sized;

        void size_box() {
            sf::RenderTexture t;
            t.create(unit_size.x*_width_units, unit_size.y);
            sf::Sprite box_left(text_box::left);
            t.draw(box_left);
            if (_width_units > 2) {
                sf::Sprite box(text_box::centre, sf::IntRect(sf::Vector2i(0,0), sf::Vector2i(unit_size.x*(_width_units-2), unit_size.y)));
                box.setPosition(sf::Vector2f(unit_size.x,0));
                t.draw(box);
            }
            sf::Sprite rail_right(text_box::right);
            rail_right.setPosition(sf::Vector2f(unit_size.x*(_width_units-1), 0));
            t.draw(rail_right);
            t.display();
            // Texture needs to be available to refrance when drawn
            _t_box_sized = t.getTexture();
            setTexture(_t_box_sized);
        }

        void load_text() {
            sf::Sprite s(_t_box_sized);

            sf::Text txt;
            txt.setFont(assets::fonts::arial);
            txt.setCharacterSize(_char_size);
            txt.setString(_text);
            txt.setOutlineColor(sf::Color::Black);
            txt.setFillColor(sf::Color::Black);
            txt.setPosition(_char_size/4, _char_size/2);

            sf::RenderTexture t;
            t.create(unit_size.x*_width_units, unit_size.y);
            t.draw(s);
            t.draw(txt);
            t.display();
            _t_text = t.getTexture();
        }

        public:
        TextBox(unsigned int width_, std::string text_, unsigned int char_size_) : Sprite(), _width_units(width_), _char_size(char_size_), _text(text_) {
            size_box();
            load_text();
            setTexture(_t_text);
        }
        TextBox(unsigned int width_, std::string text_) : TextBox(width_, text_, 16) {}
        TextBox(unsigned int width_) : TextBox(width_, "", 16) {}
        /**
         * \brief Set the text to be displayed
        */
        void set_text(std::string text_) {
            _text = text_;
            load_text();
        }
    };

    class ProgressBar : public sf::Sprite {
        public:
        enum State {NORMAL,SUCCESS,FAIL};

        private:
        double _progress;
        sf::Texture _texture;

        State _state = NORMAL;

        public:

        ProgressBar() : Sprite(), _progress(0.0) {
            render();
        }

        void render() {
            auto texture_size = progress_bar::empty.getSize();
            unsigned int progress_pixels = _progress * texture_size.x;

            sf::Texture fill_texture;
            switch (_state)
            {
            case NORMAL:
                fill_texture = progress_bar::filled;
                break;
            case SUCCESS:
                fill_texture = progress_bar::success;
                break;
            case FAIL:
                fill_texture = progress_bar::failed;
                break;
            default:
                break;
            }

            sf::Sprite empty(progress_bar::empty);
            sf::Sprite filled(fill_texture, sf::IntRect(sf::Vector2i(0,0), sf::Vector2i(progress_pixels, texture_size.y)));
            sf::RenderTexture t;
            t.create(texture_size.x, texture_size.y);

            t.draw(empty);
            t.draw(filled);
            t.display();
            _texture = t.getTexture();
            setTexture(_texture);
        }

        void set_progress(double p_) {_progress = std::clamp(p_, 0.0, 1.0);}
        double get_progress() {return _progress;}
        void set_state(State s_) {_state = s_;}
        State get_state() {return _state;}
    };
}

#endif