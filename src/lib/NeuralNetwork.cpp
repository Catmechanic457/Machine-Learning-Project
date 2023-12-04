#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <json/json.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

namespace jcv {
    using namespace std;
    template<typename T>
    /**
     * \brief Converts `Json::Value` array into `vector<T>`
     * \param i_ The json array
     * \return The json array converted into a vector
    */
    vector<T> to_vector(Json::Value i_) {
        T size = i_.size();
        vector<T> v(size);
        for (unsigned int i = 0; i < size; i++) {
            v[i] = i_[i].as<T>();
        }
        return v;
    }

    template<typename T>
    /**
     * \brief Converts  `vector<T>` into `Json::Value` array
     * \param i_ The vector
     * \return The vector converted into a json array
    */
    Json::Value from_vector(vector<T> i_) {
        Json::Value j;
        for (unsigned int i : i_) {
            j.append(i);
        }
        return j;
    }
}

namespace nn {
    using namespace std;
    class Values {

        public:
        vector<unsigned int> shape;
        vector<double> weights;
        vector<double> bias;

        Values() {}
        Values(vector<unsigned int> shape_, vector<double> weights_, vector<double> bias_) :
            shape(shape_),
            weights(weights_),
            bias(bias_) {}
    };

    class Network {

        private:
        unsigned int w_index(unsigned int l_, unsigned int n_, unsigned int w_) const {
            unsigned int index = 0;
            for (unsigned int i = 0; i < l_; i++) {
                index += _shape[i] * _shape[i+1];
            }
            index += n_ * _shape[l_+1];
            index += w_;
            return index;
        }

        unsigned int b_index(unsigned int l_, unsigned int n_) const {
            unsigned int index = 0;
            for (unsigned int i = 0; i < l_; i++) {
                index += _shape[i];
            }
            index += n_;
            return index;
        }

        double af_sig(double value_, double bias_) const {
            const double _e = 2.71828;
            double value = 1.0/(1 + (pow(_e, -(value_ + bias_))));
        
            return value;
        }

        double af_sig_est(double value_, double bias_) const {
            double x = (value_ + bias_);
            double value = 0.5 * (x / (1 + abs(x)) + 1);
            return value;
        }

        double af_bin(double value_, double bias_) const {
            if ((value_ + bias_) >= 0) {return 1.0;}
            return 0.0;
        }

        double af_lin(double value_, double bias_) const {
            return value_ + bias_;
        }

        vector<unsigned int> _shape;
        unsigned int _layers;

        vector<double> _weights;
        unsigned int _w_size;

        vector<double> _bias;
        unsigned int _b_size;

        public:
        Network(vector<unsigned int> s_) : 
            _shape(s_),
            _layers(s_.size())
            {

            // Calculate number of weights
            unsigned int w_size = 0;
            for (unsigned int i = 1; i < _layers; i++) {
                w_size += _shape[i-1] * _shape[i];
            }
            _w_size = w_size;

            // Calculate number of bias
            unsigned int b_size = 0;
            for (unsigned int i = 0; i < _layers; i++) {
                _b_size += _shape[i];
            }
            _b_size = b_size;

            // Size weight vector
            _weights.resize(_w_size);
            _weights.shrink_to_fit();

            // Size weight vector
            _bias.resize(_b_size);
            _bias.shrink_to_fit();
        }
        /**
         * \brief The number of nodes in each layer.
         * Index `0` denotes the input layer
         * \return The shape as a vector
        */
        vector<unsigned int> shape() const {return _shape;}
        /**
         * \brief The bias of each node in each layer
         * \return The bias as a vector
        */
        vector<double> bias() const {return _bias;}
        /**
         * \brief The weights of all connections across all layers
         * \return The weights as a vector
        */
        vector<double> weights() const {return _weights;}
        /**
         * \brief Packages network values into a `nn::Values` object
         * \return The network values
        */
        Values package_values() const {
            Values v(_shape, _weights, _bias);
            return v;
        }
        /**
         * \brief loads `nn::Values` into the network
         * The shape of the new values must match the shape of the network
         * \param v_ New network values
        */
        void load_values(Values& v_) {
            // Check shape of new values matches shape of old
            if (v_.shape != _shape) {
                throw invalid_argument("Shape of new values does not match shape of the network.");
                return;
            }
            _weights = v_.weights;
            _bias = v_.bias;
        }

        // RE-WRITE TO NOT USE b_index & w_index

        /**
         * \brief Calculate the output layer values based on input layer values
         * \param input_ Values of the input layer as a vector
         * \return Values of the output layer as a vector
        */
        vector<double> calculate(const vector<double>& input_) const {
            vector<double> l_inputs = input_;
            vector<double> l_outputs;
            for (unsigned int l = 1; l < _layers; l++) {
                // For every layer (excluding input layer)
                l_outputs.resize(_shape[l]);
                for (unsigned int n = 0; n < _shape[l]; n++) {
                    // For every node in current layer
                    double n_output = 0.0;
                    double n_bias = _bias[b_index(l, n)];
                    for (unsigned int np = 0; np < _shape[l-1]; np++) {
                        // For node in previous layer
                        unsigned int index = w_index(l-1, np, n);
                        double weight = _weights[index]; // Get weight of node
                        double weighted_input = l_inputs[np] * weight;
                        n_output += weighted_input;
                    }
                    // Apply activation function
                    // Output layers may have a different f(x)
                    if (l == (_layers - 1)) {l_outputs[n] = af_sig(n_output, n_bias);}
                    else {l_outputs[n] = af_sig(n_output, n_bias);}
                }
                    l_inputs = l_outputs;
                    l_inputs.resize(_shape[l]);
            }
            return l_outputs;
        }
    };

    class Storage {
        private:

        string directory;
        Json::Value data;

        public:

        explicit Storage(string dir_) {
            directory = dir_;
        }

        /**
         * \brief Read the data stored in the file
        */
        void read_data() {
            ifstream values_file(directory, ifstream::binary);
            if (values_file.good()) {
                values_file >> data;
            }
            else {
                std::cout << "Cannot locate: " << directory << std::endl;
                throw std::runtime_error("File or directory does not exist");
            }
            values_file.close();
        }

        /**
         * \brief Write loaded data to the file
        */
        void write_data() const {
            ofstream values_file(directory, ofstream::binary);
            Json::StyledWriter writer;
            values_file << writer.write(data);
            values_file.close();
        }

        /**
         * \brief Package the data stored under the id into a nn::Values object
         * \param id_ The id to look for
         * \return The stored network values
        */
        Values read_values(string id_) const {
            Json::Value network = data[id_];
            vector<unsigned int> shape = jcv::to_vector<unsigned int>(network["shape"]); shape.shrink_to_fit();
            vector<double> bias = jcv::to_vector<double>(network["bias"]); bias.shrink_to_fit();
            vector<double> weights = jcv::to_vector<double>(network["weights"]); weights.shrink_to_fit();
            Values values(shape, weights, bias);
            return values;
        }

        /**
         * \brief Load `nn::Values` under an id.
         * `write_data()` must be called to update the file
        */
        void load_values(Values& n_, string id_) {
            data[id_]["shape"] = jcv::from_vector<unsigned int>(n_.shape);
            data[id_]["bias"] = jcv::from_vector<double>(n_.bias);
            data[id_]["weights"] = jcv::from_vector<double>(n_.weights);
        }
        /**
         * \brief Load `nn::Network` under an id.
         * `write_data()` must be called to update the file
        */
        void load_values(Network& n_, string id_) {
            auto v = n_.package_values();
            load_values(v, id_);
        }
    };
    class Display {

        private:
        string _title = "Network";
        unsigned int _winx = 1000;
        unsigned int _winy = 1000;
        
        double sig(double x_) const {
            const double _e = 2.71828;
            double y = 1.0/(1 + (pow(_e, -x_)));
            return y;
        }
        
        public:
        /**
         * \brief Display `nn::Values` in a window
         * \param n_ Values to display
        */
        void plot_network(Network& n_) const {

            sf::RenderWindow window(sf::VideoMode(_winx, _winy), _title, sf::Style::Close);

            
            sf::Event event;
            window.clear(sf::Color::White);

            auto shape = n_.shape();
            auto weights = n_.weights();
            auto bias = n_.bias();
            unsigned int layer_count = shape.size();

            const unsigned int node_r = 5;

            sf::CircleShape node(node_r);
            sf::VertexArray weight(sf::Lines, 2);
            unsigned int bias_index = 0;
            unsigned int weights_index = 0;
            for (unsigned int l = 0; l < layer_count; l++) {
                unsigned int node_count = shape[l];
                unsigned int nx = ((l + 1)*_winx) / (layer_count + 1);
                for (unsigned int n = 0; n < node_count; n++) {
                    unsigned int ny = ((n + 1)*_winy) / (node_count + 1);
                    weight[0].position = sf::Vector2f(nx,ny);
                    if (l < (layer_count - 1)) {
                        unsigned int n2x = ((l + 2)*_winx) / (layer_count + 1);
                        for (unsigned int w = 0; w < shape[l+1]; w++) {
                            unsigned int n2y = ((w + 1)*_winy) / (shape[l+1] + 1);
                            weight[1].position = sf::Vector2f(n2x, n2y);

                            double s = sig(weights[weights_index]);
                            weight[0].color = sf::Color(255*s, 255*-s, 255, 255);
                            weight[1].color = sf::Color(255*s, 255*-s, 255, 255);


                            window.draw(weight);
                            weights_index++;
                        }
                    }
                    double s = sig(bias[bias_index]);
                    node.setFillColor(sf::Color(255, 255*s, 255*(1-s), 255));
                    node.setPosition(nx - node_r, ny - node_r);
                    window.draw(node);
                    bias_index++;
                }
            }
            window.display();
            while (window.isOpen()) {
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed)
                        window.close();
                }
            }
        }
    };
}