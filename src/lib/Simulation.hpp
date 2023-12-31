#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <iostream>
#include <cmath>
#include <chrono>

#include <SFML/Graphics.hpp>

#include "NeuralNetwork.hpp"
#include "PerlinNoise.hpp"
#include "Assets.hpp"

#include<windows.h>

const double pi = 3.14159265358979;

namespace radians {
    /**
     * \brief Convert from degrees to radians
     * \param a_ Angle in degrees
     * \return Angle in radians
    */
    double from_degrees(double a_) {return ((a_*pi)/180);}
    /**
     * \brief Ensures angle is within range 0pi to 2pi
     * \param a_ Angle
     * \return Wrapped angle
    */
    double wrap(double angle) {
        return angle - (2*pi) * floor(angle / (2*pi));
    }
    /**
     * \brief Convert from radians to degrees
     * \param a_ Angle in radians
     * \return Angle in degrees
    */
    double to_degrees(double a_) {return (180 * a_)/pi;}
}

namespace rs {
    /**
     * \brief Represents a point in 2D space
    */
    template <typename T>
    struct Vector2 {
        T x;
        T y;
        Vector2() {}
        template <typename G>
        Vector2(Vector2<G> vect_) : x(vect_.x), y(vect_.y) {}
        Vector2(T x_, T y_) : x(x_), y(y_) {}
        void from_bearing(double d_, double r_) {
            x = cos(r_) * d_;
            y = sin(r_) * d_;
        }
    };
    /**
     * \brief Represents a point and rotation in 2D space
    */
    struct Position {
        Vector2<double> position;
        double rotation;
        Position() {}
        Position(Vector2<double> pos_, double rot_) : position(pos_), rotation(rot_) {}
        Position(double x_, double y_, double rot_) : position(x_, y_), rotation(rot_) {}
    };
}

namespace stage {
    using namespace assets::textures::stage;
    /**
     * \brief An environment with collision areas.
     * Based on Perlin noise
    */
    class Stage {
        private:
        rs::Vector2<unsigned int> _win;
        rs::Vector2<unsigned int> _sp;

        siv::PerlinNoise _noise;

        unsigned int _octaves = 2;
        double _frequency = 6.0;
        double _threshold = 0.55;

        const double trace_distance = 50.0; // Length of each ray
        const unsigned int cast_count = 32; // Number of rays cast
        const unsigned int collision_points = 20; // Number of probes per ray
        const unsigned int max_cast_iterations = 10000; // Number of casts before timeout

        unsigned int fix_ray_index(unsigned int ri_) const {
            if (ri_ >= cast_count) {ri_-=cast_count;}
            return ri_;
        }

        bool cast(rs::Vector2<double> pos_, unsigned int pri_, unsigned int& count) const {
            // Throw if max casts has been reach
            if (count > max_cast_iterations) {
                throw std::runtime_error("Iteration limit reached");
            }
            count++;

            for (unsigned int i = 0; i < cast_count; i++) {
                // Ignore the parent ray to avoid back-tracking
                if (pri_ < cast_count and i == (cast_count/2) and (i%2)==0) {continue;}

                unsigned int ray_index = i + pri_;
                ray_index = fix_ray_index(ray_index);
                rs::Vector2<double> vect;
                vect.from_bearing(trace_distance/collision_points, (2*pi*ray_index)/cast_count);
                bool collides = false;

                // Probe points along the ray
                for (unsigned int p = 1; p <= collision_points; p++) {
                    pos_.x += vect.x;
                    pos_.y += vect.y;
                    if (collision(pos_)) {collides = true; break;}
                }
                if (!in_bounds(pos_)) {return true;} // Return true if the edge has been reached
                if (!collides) {
                    if (cast(pos_, ray_index, count)) {return true;} // Cast a new ray in an empty space
                }
            }
            return false;
        }

        protected:
        /**
         * \param pos_ Point
         * \return The amplitude of the noise at the given point
        */
        double value(rs::Vector2<double> pos_) const {
            return value(pos_, _frequency, _octaves);
        }
        /**
         * \param pos_ Point
         * \param f_ Frequency to use
         * \param o_ Number of octaves to use
         * \return The amplitude of the noise at the given point
        */
        double value(rs::Vector2<double> pos_, double f_, unsigned int o_) const {
            const unsigned int s = _win.x < _win.y ? _win.x : _win.y; // Use the smaller value
            const double fx = f_/_win.x;
            const double fy = f_/_win.y;
            return _noise.octave2D_01(pos_.x * fx, pos_.y * fy, o_);
        }

        public:

        unsigned int seed;

        Stage(rs::Vector2<unsigned int> win_) : _win(win_), _sp(win_.x/2, win_.y/2) {}
        Stage(unsigned int winx_, unsigned int winy_) : _win(winx_, winy_), _sp(winx_/2, winy_/2) {}

        /**
         * \brief Set the number of octaves of noise.
         * Defaults to `2`
         * \param o_ New octave value
        */
        void set_octaves(unsigned int o_) {_octaves = std::clamp<unsigned int>(o_, 1, 16);}
        /**
         * \brief Set the frequency of the noise.
         * Defaults to `4.0`
         * \param f_ New frequency value
        */
        void set_frequency(double f_) {_frequency = std::clamp<double>(f_, 0.1, 64.0);}
        /**
         * \brief Set the threshold ("height" value) used for the map.
         * Defaults to `0.6`
         * \param t_ New threshold value
        */
        void set_threshold(double t_) {_threshold = std::clamp<double>(t_, 0.0, 1.0);}
        /**
         * \returns The threshold value for collisions
        */
        double get_threshold() {return _threshold;}
        /**
         * \brief Generate new noise
        */
        void generate() {_noise = siv::PerlinNoise(seed);}
        /**
         * \brief Check a point lies inside a collision area
         * \param pos_ The point to check
         * \return `true` if the point is inside a collision area
        */
        bool collision(rs::Vector2<double> pos_) const {
            if (!in_bounds(pos_)) {return false;} // Credit Michael
            return value(pos_) > _threshold;
            }
        /**
         * \brief Check a point lies inside the stage confines
         * \param pos_ The point to check
         * \return `true` if the point is inside stage confines
        */
        bool in_bounds(rs::Vector2<double> pos_) const {return (pos_.x >= 0 and pos_.y >= 0 and pos_.x < _win.x and pos_.y < _win.y);}
        /**
         * \brief Check if the stage can be navigated
         * \return `true` if the stage is possible
        */
        bool possible() const {
            if (collision(_sp)) {return false;}
            unsigned int iteration = 0;
            bool possible = false;
            try {
                possible = cast(_sp, cast_count, iteration);
            }
            catch (std::runtime_error const&) {
                possible = false;
            }
            return possible;
        }
        /**
         * \return The stage spawnpoint (located in the centre)
        */
        rs::Vector2<unsigned int> spawn_point() const {return _sp;}
        /**
         * \return The stage size
        */
        rs::Vector2<unsigned int> window_size() const {return _win;}
    };
    /**
     * \brief A stage that can be drawn to an sf::RenderWindow
    */
    class DisplayedStage : public Stage {
        private:
        sf::Texture _t_collision;
        sf::Texture _t_possible;
        sf::RenderWindow& _window;

        sf::Sprite from_image(sf::Image image_) const {
            sf::Texture t;
            t.loadFromImage(image_);
            sf::Sprite s(t);
            return s;
        }
        void centre(sf::Sprite& s_) const {
            auto t_size = s_.getTexture()->getSize();
            s_.setOrigin(sf::Vector2f((t_size.x/2),(t_size.y/2)));
        }
        sf::Image collision_boundaries() const {
            // Create image
            sf::Image image;
            auto win = window_size();
            image.create(win.x, win.y, sf::Color::White);
            for (unsigned int x = 0; x < win.x; x++) {
                for (unsigned int y = 0; y < win.y; y++) {
                    unsigned int pv = 255;
                    rs::Vector2<double> point(x,y);
                    if (collision(point)) {
                        image.setPixel(x,y,sf::Color(pv,0,0,255));
                    }
                }
            }
            // Create sprite from image
            return image;
        }

        public:
        DisplayedStage(rs::Vector2<unsigned int> win_, sf::RenderWindow& window_) : Stage(win_), _window(window_) {}
        DisplayedStage(unsigned int x_, unsigned int y_, sf::RenderWindow& window_) : Stage(x_, y_), _window(window_) {}
        /**
         * \brief Calculate the textures of non-constant sprites.
         * Should be called when changes have been made to the stage
         * (such a seed change)
        */
        void render() {
            _t_collision.loadFromImage(collision_boundaries());
            _t_possible = possible() ? evaluation::possible : evaluation::possible;
        }
        enum POI {
            COLLISION,
            SPAWNPOINT,
            POSSIBLE,
            AXIS,
            SEED
        };
        /**
         * \brief Draw a poi to the window
         * \param poi_ The poi to draw
        */
        void draw(POI poi_) const {
            auto win = window_size();
            auto sp = spawn_point();

            sf::Sprite sprite;
            sf::Text text;
            sf::Texture t;
            switch (poi_)
            {
            case COLLISION :
                sprite.setTexture(_t_collision);
                _window.draw(sprite);
                break;

            case SPAWNPOINT :
                sprite.setTexture(spawnpoint);
                centre(sprite);
                sprite.setPosition(sf::Vector2f(sp.x, sp.y));
                _window.draw(sprite);
                break;
            
            case POSSIBLE :
                sprite.setTexture(_t_possible);
                sprite.setPosition(win.x/50, win.y/50);
                _window.draw(sprite);
                break;

            case AXIS :
                text.setFont(assets::fonts::arial);
                text.setCharacterSize(16);

                text.setString("x");
                text.setFillColor(sf::Color(180,0,0,255));
                text.setPosition(sf::Vector2f(win.x/2, win.y/500));
                _window.draw(text);

                text.setString("y");
                text.setFillColor(sf::Color(0,180,0,255));
                text.setPosition(sf::Vector2f(win.x/500, win.y/2));
                _window.draw(text);
                break;
            
            case SEED :
                text.setFont(assets::fonts::arial);
                text.setCharacterSize(16);
                text.setString("Seed: " + std::to_string(seed));
                text.setFillColor(sf::Color(10,10,10,255));
                text.setPosition(sf::Vector2f(win.x/50, win.y/500));
                _window.draw(text);
                break;

            default:
                throw std::runtime_error("Invalid POI type.");
                break;
            }
        }
    };
}

namespace bot {
    using namespace assets::textures::bot;
    /**
     * \brief A distance and angle
    */
    struct DataPoint {
        double angle;
        double distance;
        DataPoint() {} 
        DataPoint(double r_, double d_) {
            angle = r_;
            distance = d_;
        }
    };

    enum MoveType : unsigned int {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };
    /**
     * \brief A sonar that is attached to a `bot::Bot` object
    */
    class Sonar {
        private:
        rs::Position& _parent_pos;
        stage::Stage& _stage;

        const double _rots = pi/2; // Sonar rotation speed in rad/s
        const unsigned int _cast_count = 18;
        const double _cast_resolution = 1.0;
        const double _fov = pi;
        const double _max_dist = 100.0;
        
        unsigned int _step = 0;
        bool _bounce = false;

        unsigned int data_index = 0;

        std::vector<DataPoint> _data;

        void manage_bounce() {
            if (_step == 0) {_bounce = false;}
            else if (_step == _cast_count) {_bounce = true;}
        }
        double rotation() const {
            double rot = (_step*_fov)/(_cast_count);
            double offset = -(_fov/2.0);
            return rot + offset;
        }
        
        public:
        Sonar(rs::Position& parent_pos_, stage::Stage& stage_) :
        _parent_pos(parent_pos_),
        _stage(stage_),
        _data(_cast_count) {
            _data.resize(_cast_count);
            _data.shrink_to_fit();
        }
        /**
         * \return The distance reading of the sonar
        */
        double distance() const {
            rs::Vector2<double> cast;
            auto current = position();

            for (double probe_dist = 0.0 ;; probe_dist+=_cast_resolution) {

                cast.from_bearing(probe_dist, current.rotation);

                rs::Vector2<double> probe_point = current.position;
                probe_point.x += cast.x;
                probe_point.y += cast.y;

                if (_stage.collision(probe_point) or probe_dist == _max_dist) {return probe_dist;}
            }
        }
        /**
         * \return `true` if the sonar is at it's last step in the cycle
        */
        bool at_end() const {
            return (_step == 0 or _step == (_cast_count));
        }
        /**
         * \brief Can only be called during the last step of the cycle
         * \return A vector of the data collected during the cycle
        */
        std::vector<DataPoint> data() const {
            if (!at_end()) {throw std::runtime_error("Data not available");}
            return _data;
        }
        /**
         * \brief Moves the sonar into the next step of its cycle
        */
        void step() {
            if (_bounce) {_step--;}
            else {_step++;}
            manage_bounce();
            DataPoint data_point(rotation(), distance());
            _data[data_index] = data_point;
            data_index++;
            if (data_index == _cast_count) {data_index = 0;}
        }
        /**
         * \return The sonar's position
        */
        rs::Position position() const {
            rs::Position pos = _parent_pos;
            pos.rotation = pos.rotation + rotation();
            return pos;
        }
        /**
         * \return The time in seconds that passes after each step of the cycle
        */
        double gap() const {
            return _fov/(_rots*_cast_count);
        }
        /**
         * \return The fov
        */
        double fov() const {
            return _fov;
        }
        /**
         * \return The maximum distance that can be measured
        */
        double range() const {
            return _max_dist;    
        }
    };
    /**
     * \brief A bot.
     * Has a `bot::Sonar`
    */
    class Bot {
        private:

        const double _pxs = 5.0; // Movement speed in pixels/s
        const double _turn_r = 15;

        rs::Position _pos;
        stage::Stage& _stage;

        rs::Vector2<double> helix(double angle_, double r_) const {
            // https://en.wikipedia.org/wiki/Helix
            // adapted from (cos(t), sin(t))
            rs::Vector2<double> pos;
            pos.x = r_*sin(angle_);
            pos.y = r_*(cos(angle_));
            return pos;
        }
        
        protected:
        Sonar _sonar;
        MoveType _current_move = FORWARD;

        void fw(double s_) {
            double const m = 1;
            _pos.position.x += m*s_*cos(_pos.rotation);
            _pos.position.y += m*s_*sin(_pos.rotation);
        }
        void bw(double s_) {
            double const m = -0.6;
            _pos.position.x += m*s_*cos(_pos.rotation);
            _pos.position.y += m*s_*sin(_pos.rotation);
        }
        void lft(double s_) {
            double const m = 0.4;
            double angle_change = (s_*m)/_turn_r;
            double angle = _pos.rotation - angle_change;
            auto v1 = helix(-_pos.rotation, _turn_r);
            auto v2 = helix(-angle, _turn_r);
            _pos.position.x += v2.x - v1.x;
            _pos.position.y += v2.y - v1.y;
            _pos.rotation = radians::wrap(angle);;
        }
        void rgt(double s_) {
            double const m = 0.4;
            double angle_change = (s_*m)/_turn_r;
            double angle = _pos.rotation + angle_change;
            auto v1 = helix(_pos.rotation, _turn_r);
            auto v2 = helix(angle, _turn_r);
            _pos.position.x += v2.x - v1.x;
            _pos.position.y -= v2.y - v1.y;
            _pos.rotation = radians::wrap(angle);
        }

        public:

        Bot(stage::Stage& stage_) :
            _stage(stage_),
            _sonar(_pos, stage_)
        {
            auto sp = _stage.spawn_point();
            _pos.position = rs::Vector2<double>(sp.x, sp.y);
            _pos.rotation = 0;
        }
        /**
         * \brief Move the bots position.
         * Based on a move type and the amount of time passed
         * \param s_ Time passed in seconds
        */
        void move(double s_) {
            rs::Position move;
            switch (_current_move)
            {
            case FORWARD: fw(s_); break;
            case BACKWARD: bw(s_); break;
            case LEFT: lft(s_); break;
            case RIGHT: rgt(s_); break;
            default:
                throw std::runtime_error("Invalid move type");
                break;
            }
        }
        /**
         * \return The bot's position and rotation
        */
        rs::Position get_position() const {
            return _pos;
        }
        /**
         * \brief Sets the bots position and rotation
         * \param pos_ The new position and rotation
        */
        void set_position(rs::Position pos_) {
            _pos = pos_;
        }
        /**
         * \brief Sets the bots position.
         * Rotation remains unchanged
         * \param v_ The new position
        */
        void set_position(rs::Vector2<double> v_) {
            _pos.position = v_;
        }
        /**
         * \brief Moves the bot and steps the sonar
        */
        virtual void step() { // Overwritten by Bot_wBrain
            double time_elapsed = _sonar.gap();
            move(time_elapsed);
            _sonar.step();

            // Nothing more is done as this bot has no "brain"
        }
        /**
         * \return `true` if the bot is in-bounds
        */
        bool in_bounds() const {
            return _stage.in_bounds(_pos.position);
        }
        /**
         * \return `true` if the bot is in a collision area
        */
        bool collided() const {
            return _stage.collision(_pos.position);
        }
        stage::Stage& stage() {return _stage;}
    };
    /**
     * \brief Inherits `bot::Bot`.
     * Move type is based on the output of a `nn::Network`
    */
    class Bot_wBrain : public Bot {
        private:
        double data_func(double x_) const {
            const double e = 2.71828;
            const double i = 20.0; // X-axis intercept
            const double s = 15; // Scale
            double y = 2.0/(1 + pow(e, -((x_-i)/s)));
            return y;
        }

        nn::Network _brain;
        bot::MoveType calc_move(std::vector<DataPoint> data_) const {
            std::sort(
                data_.begin(), data_.end(),
                [] (const DataPoint &a, const DataPoint &b)
                {
                    return a.angle < b.angle;
                }
            );
            std::vector<double> nn_input;
            for (auto dp : data_) {
                nn_input.push_back(data_func(dp.distance));
            }
            nn_input.shrink_to_fit();
            auto nn_output = _brain.calculate(nn_input);
            bot::MoveType best_move = FORWARD;
            for (unsigned int i = 1; i < 4; i++) {
                if (nn_output[i] > nn_output[best_move]) {
                    best_move = static_cast<bot::MoveType>(i);
                }
            }
            return best_move;
        }

        public:
        Bot_wBrain(stage::Stage& stage_, nn::Network nn_) :
            Bot(stage_),
            _brain(nn_)
        {}
        /**
         * \brief Calculates a move. Moves the bot and steps the sonar
        */
        void step() override {
            if (_sonar.at_end()) {
                auto data = _sonar.data();
                _current_move = calc_move(data);
            }
            Bot::step(); // Moves the bot to it's next pos, step the sonar
        }
        /**
         * \return The `nn::Network` object used for calculating moves
        */
        nn::Network brain() const {
            return _brain;
        }
    };
    class DisplayedBot : public Bot_wBrain {
        private:
        sf::Sprite _s_bot;
        sf::Sprite _s_sonar;
        sf::RenderWindow& _window;
        sf::Image _path;

        bool _trace_path = false;

        public:
        DisplayedBot(stage::Stage& stage_, nn::Network n_, sf::RenderWindow& window_) : Bot_wBrain(stage_, n_), _window(window_) {
            auto bts = sonar.getSize();
            auto sts = body.getSize();
            _s_bot.setTexture(body);
            _s_sonar.setTexture(sonar);
            _s_bot.setOrigin(sf::Vector2f((bts.x/2),(bts.y/2)));
            _s_sonar.setOrigin(sf::Vector2f((sts.x/2),(sts.y/2)));

            reset_path();
        }
        /**
         * \brief Moves the bot and steps the sonar.
         * If `trace_path` is `true`, it will also mark a point along its path
        */
        void step() override {
            Bot_wBrain::step();
            if (_trace_path) {
                auto pos = get_position().position;
                if (in_bounds()) {
                    _path.setPixel(pos.x, pos.y, sf::Color(0,255,255,255));
                }
            }
        }
        /**
         * \brief Draws the bot
        */
        void draw_bot() {
            rs::Position bot_pos = get_position();
            _s_bot.setPosition(sf::Vector2f(bot_pos.position.x,bot_pos.position.y));
            _s_bot.setRotation(radians::to_degrees(bot_pos.rotation));
            _window.draw(_s_bot);
        }
        /**
         * \brief Draws the sonar
        */
        void draw_sonar() {
            rs::Position sonar_pos = _sonar.position();
            _s_sonar.setPosition(sf::Vector2f(sonar_pos.position.x,sonar_pos.position.y));
            _s_sonar.setRotation(radians::to_degrees(sonar_pos.rotation));
            _window.draw(_s_sonar);
        }
        /**
         * \brief Draws the bot and sonar
        */
        void draw() {
            draw_bot();
            draw_sonar();
        }
        /**
         * \brief If `true`, the bot will mark its journey
         * \param enable_
        */
        void trace_path(bool enable_) {_trace_path = enable_;}
        /**
         * \brief Clears the traced path
        */
        void reset_path() {
            auto win = _window.getSize();
            _path.create(win.x, win.y, sf::Color::Transparent);
        }
        /**
         * \brief Draws the traced path
        */
        void draw_path() const {
            if (not _trace_path) {return;}
            sf::Texture t;
            t.loadFromImage(_path);
            sf::Sprite s(t);
            _window.draw(s);
        }
        void draw_fov() const {
            rs::Position bot_pos = get_position();
            double fov = _sonar.fov();
            sf::ConvexShape fov_area;
            const int point_count = 33;
            fov_area.setPointCount(point_count);
            fov_area.setPoint(0,sf::Vector2f(bot_pos.position.x, bot_pos.position.y));
            double range = _sonar.range();
            for (unsigned int i = 1; i < point_count; i++) {
                double angle = bot_pos.rotation - (fov/2.0) + ((fov*(i-1))/(point_count-2));
                rs::Vector2<float> v;
                v.from_bearing(range, angle);
                fov_area.setPoint(i, sf::Vector2f(bot_pos.position.x + v.x, bot_pos.position.y + v.y));
            }
            fov_area.setFillColor(sf::Color(255,255,0,100));
            _window.draw(fov_area);
        }
    };
}

#endif