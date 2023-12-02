
#include <vector>
#include <iostream>
#include <cmath>
#include <chrono>

#include <SFML/Graphics.hpp>

#include "lib\NeuralNetwork.cpp"
#include "lib\PerlinNoise.hpp"

#include<windows.h>

#define TEXTURES_PATH "assets/textures/"
#define FONTS_PATH "assets/fonts/"

const double pi = 3.14159265358979;

namespace radians {
    double from_degrees(double a_) {return ((a_*pi)/180);}
    double wrap(double angle) {
        return angle - (2*pi) * floor(angle / (2*pi));
    }
    double to_degrees(double a_) {return (180 * a_)/pi;}
}

namespace rs {
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
    struct Position {
        Vector2<double> position;
        double rotation;
        Position() {}
        Position(Vector2<double> pos_, double rot_) : position(pos_), rotation(rot_) {}
        Position(double x_, double y_, double rot_) : position(x_, y_), rotation(rot_) {}
    };
}

namespace stage {

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
         * \return The amplitude of the noise at the given point
        */
        double value(rs::Vector2<double> pos_) const {
            const double fx = _frequency/_win.x;
            const double fy = _frequency/_win.y;
            return _noise.octave2D_01(pos_.x * fx, pos_.y * fy, _octaves);
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
        void octaves(unsigned int o_) {_octaves = std::clamp<unsigned int>(o_, 1, 16);}
        /**
         * \brief Set the frequency of the noise.
         * Defaults to `4.0`
         * \param f_ New frequency value
        */
        void frequency(double f_) {_frequency = std::clamp<double>(f_, 0.1, 64.0);}
        /**
         * \brief Set the threshold ("height" value) used for the map.
         * Defaults to `0.6`
         * \param t_ New threshold value
        */
        void threshold(double t_) {_threshold = std::clamp<double>(t_, 0.0, 1.0);}
        /**
         * \brief Generate new noise
        */
        void generate() {_noise = siv::PerlinNoise(seed);}
        /**
         * \brief Check a point lies inside a collision area
         * \return `true` if the point is inside a collision area
        */
        bool collision(rs::Vector2<double> pos_) const {
            if (!in_bounds(pos_)) {return false;} // Credit Michael
            return value(pos_) > _threshold;
            }
        /**
         * \brief Check a point lies inside the stage confines
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
            catch (std::runtime_error) {
                possible = false;
            }
            return possible;
        }
        rs::Vector2<unsigned int> spawn_point() const {return _sp;}
        rs::Vector2<unsigned int> window_size() const {return _win;}
    };

    class DisplayedStage : public Stage {
        
        private:
        sf::Font _arial;
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
            for (int x = 0; x < win.x; x++) {
                for (int y = 0; y < win.y; y++) {
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
        DisplayedStage(rs::Vector2<unsigned int> win_, sf::RenderWindow& window_) : Stage(win_), _window(window_) {
            _arial.loadFromFile(FONTS_PATH "arial.ttf");
        }
        DisplayedStage(unsigned int x_, unsigned int y_, sf::RenderWindow& window_) : Stage(x_, y_), _window(window_) {
            _arial.loadFromFile(FONTS_PATH "arial.ttf");
        }
        void render() {
            _t_collision.loadFromImage(collision_boundaries());
            auto texture_source = possible() ? TEXTURES_PATH "stage/possible.png" : TEXTURES_PATH "stage/impossible.png";
            _t_possible.loadFromFile(texture_source);
            //delete(texture_source);
        }
        enum POI {
            COLLISION,
            SPAWNPOINT,
            POSSIBLE,
            AXIS,
            SEED
        };
        void draw(POI poi_) {
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
                t.loadFromFile(TEXTURES_PATH "stage/spawnpoint.png");
                sprite.setTexture(t);
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
                text.setFont(_arial);
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
                text.setFont(_arial);
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
    class Sonar {
        private:
        rs::Position& _parent_pos;
        stage::Stage& _stage;

        const double _rots = pi/2; // Sonar rotation speed in rad/s
        const unsigned int _cast_count = 18;
        const double _cast_resolution = 1.0;
        const double _fov = pi;
        const double _max_dist = 100.0;
        
        int _step = 0;
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
        _data(_cast_count),
        _stage(stage_) {
            _data.resize(_cast_count);
            _data.shrink_to_fit();
        }

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

        bool at_end() const {
            return (_step == 0 or _step == (_cast_count));
        }

        std::vector<DataPoint> data() const {
            if (!at_end()) {throw std::runtime_error("Data not available");}
            return _data;
        }

        void step() {
            if (_bounce) {_step--;}
            else {_step++;}
            manage_bounce();
            DataPoint data_point(rotation(), distance());
            _data[data_index] = data_point;
            data_index++;
            if (data_index == _cast_count) {data_index = 0;}
        }

        rs::Position position() const {
            rs::Position pos = _parent_pos;
            pos.rotation = pos.rotation + rotation();
            return pos;
        }
        double gap() {
            return _fov/(_rots*_cast_count);
        }
    };

    class Bot {
        private:

        double _pxs = 5.0; // Movement speed in pixels/s
        double _spc; // (Seconds per Cycle) How much time has "passed" between each cycle;
        const double _turn_r = 15;

        rs::Position _pos;
        stage::Stage& _stage;
        
        protected:
        Sonar _sonar;
        MoveType _current_move = FORWARD;

        public:

        Bot(stage::Stage& stage_) :
            _stage(stage_),
            _sonar(_pos, stage_)
        {
            auto sp = _stage.spawn_point();
            _pos.position = rs::Vector2<double>(sp.x, sp.y);
            _pos.rotation = 0;
        }

        rs::Vector2<double> helix(double angle_, double r_) {
            // https://en.wikipedia.org/wiki/Helix
            // adapted from (cos(t), sin(t))
            rs::Vector2<double> pos;
            pos.x = r_*sin(angle_);
            pos.y = r_*(cos(angle_));
            return pos;
        }

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

        rs::Position get_position() {
            return _pos;
        }

        void set_position(rs::Position pos_) {
            _pos = pos_;
        }
        void set_position(rs::Vector2<double> v_) {
            _pos.position = v_;
        }

        virtual void step() { // Overwritten by Bot_wBrain
            double time_elapsed = _sonar.gap();
            move(time_elapsed);
            _sonar.step();

            //cout << "Move Type: " << _current_move << endl;

            //cout << "Bot: [" << _pos.position.x << ", " << _pos.position.y << "](" << radians::to_degrees(_pos.rotation) << ")<" << radians::to_degrees(_sonar.position().rotation - _pos.rotation) << ">\n";
            // Nothing more is done as this bot has no "brain"
        }

        bool in_bounds() {
            return _stage.in_bounds(_pos.position);
        }
        bool collided() {
            return _stage.collision(_pos.position);
        }
    };

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
        bot::MoveType calc_move(std::vector<DataPoint> data_) {
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
        void step() override {
            if (_sonar.at_end()) {
                auto data = _sonar.data();
                _current_move = calc_move(data);
            }
            Bot::step(); // Moves the bot to it's next pos, step the sonar
        }
        nn::Network brain() {
            return _brain;
        }
    };
    class DisplayedBot : public Bot_wBrain {
        private:
        sf::Sprite _s_bot;
        sf::Sprite _s_sonar;
        sf::Texture _t_bot;
        sf::Texture _t_sonar;
        sf::RenderWindow& _window;
        sf::Image _path;

        bool _trace_path = false;

        public:
        DisplayedBot(stage::Stage& stage_, nn::Network n_, sf::RenderWindow& window_) : Bot_wBrain(stage_, n_), _window(window_) {
            _t_bot.loadFromFile("assets/textures/bot/body.png");
            _t_sonar.loadFromFile("assets/textures/bot/sonar.png");
            auto bts = _t_bot.getSize();
            auto sts = _t_sonar.getSize();
            _s_bot.setTexture(_t_bot);
            _s_sonar.setTexture(_t_sonar);
            _s_bot.setOrigin(sf::Vector2f((bts.x/2),(bts.y/2)));
            _s_sonar.setOrigin(sf::Vector2f((sts.x/2),(sts.y/2)));

            reset_path();
        }

        void step() override {
            Bot_wBrain::step();
            if (_trace_path) {
                auto pos = get_position().position;
                auto win = _window.getSize();
                //_path.setPixel(std::clamp<int>(pos.x, 0, win.x - 1), std::clamp<int>(pos.y, 0, win.y - 1), sf::Color(0,255,255,255));
                if (in_bounds()) {
                    _path.setPixel(pos.x, pos.y, sf::Color(0,255,255,255));
                }
            }
        }

        void draw_bot() {
            rs::Position bot_pos = get_position();
            _s_bot.setPosition(sf::Vector2f(bot_pos.position.x,bot_pos.position.y));
            _s_bot.setRotation(radians::to_degrees(bot_pos.rotation));
            _window.draw(_s_bot);
        }
        void draw_sonar() {
            rs::Position sonar_pos = _sonar.position();
            _s_sonar.setPosition(sf::Vector2f(sonar_pos.position.x,sonar_pos.position.y));
            _s_sonar.setRotation(radians::to_degrees(sonar_pos.rotation));
            _window.draw(_s_sonar);
        }
        void draw() {
            draw_bot();
            draw_sonar();
        }
        void trace_path(bool enable_) {_trace_path = enable_;}
        void reset_path() {
            auto win = _window.getSize();
            _path.create(win.x, win.y, sf::Color::Transparent);
        }
        void draw_path() {
            if (not _trace_path) {return;}
            sf::Texture t;
            t.loadFromImage(_path);
            sf::Sprite s(t);
            _window.draw(s);
        }
    };
}

void draw_stage(sf::RenderWindow& window, stage::DisplayedStage& stage, bot::DisplayedBot& bot, unsigned int fps) {
    stage.draw(stage::DisplayedStage::COLLISION);
    stage.draw(stage::DisplayedStage::SEED);
    stage.draw(stage::DisplayedStage::SPAWNPOINT);
    bot.draw_path();
    bot.draw_bot();

    sf::Font arial;
    arial.loadFromFile("assets/fonts/arial.ttf");
    sf::Text text;
    text.setFont(arial);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::Green);
    text.setPosition(0,980);
    text.setString("FPS: " + std::to_string(fps));
    window.draw(text);
}

int main() {
    nn::Network network({18,4});
    nn::Storage storage("data/network/base.json");
    storage.read_data();
    auto values = storage.read_values("nn");
    network.load_values(values);

    sf::RenderWindow window(sf::VideoMode(1500,1500), "Stage", sf::Style::Close);
    stage::DisplayedStage map(1500, 1500, window);
    bot::DisplayedBot bot(map, network, window);
    bot.trace_path(true);

    map.seed = 0;

    #define MAX_STAGES 1000
    for (unsigned int i = 0; i < MAX_STAGES; i++) {
        std::cout << map.seed << " : ";
        map.generate();
        map.render();
        bool end = false;
        if (map.possible()) {
            #define MAX_CYCLES 150000
            #define STEPS_PER_FRAME 3600

            unsigned int fps = 0;
            unsigned int cycle = 0;
            while (true) {
                auto start = std::chrono::high_resolution_clock::now();
                for (unsigned int j = 0; j < STEPS_PER_FRAME; j++) {
                    if (bot.collided()) {
                        std::cout << "Bot ded.\n";
                        draw_stage(window, map, bot, fps);
                        window.display();
                        goto next_stage;
                    }
                    if (not bot.in_bounds()) {
                        std::cout << "Bot win.\n";
                        draw_stage(window, map, bot, fps);
                        window.display();
                        goto next_stage;
                    }
                    bot.step();
                }

                // Manage window
                draw_stage(window, map, bot, fps);
                window.display();
                sf::Event event;
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window.close();
                    }
                }
                if (!window.isOpen()) {goto next_stage;}
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                auto period_ms = duration.count();
                fps = 1000/period_ms;

                cycle += STEPS_PER_FRAME;
                if (cycle > MAX_CYCLES) {
                    std::cout << "Timed out.\n";
                    goto next_stage;
                }
            }
        }
        else {
            std::cout << "Stage impossible.\n";
        }
        next_stage:
        rs::Position pos;
        pos.position = map.spawn_point();
        pos.rotation = 0;
        bot.set_position(pos);
        bot.reset_path();
        map.seed++;
    }
    std::cout << "end.";
    while (true);

    return 0;
}