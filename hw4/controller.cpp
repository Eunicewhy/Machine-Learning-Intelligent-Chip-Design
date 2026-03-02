#include "controller.h"
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <iomanip>
using namespace std;

enum DATA { INPUT = 0, WEIGHT = 1, BIAS = 2 };

bool comparePairs(const std::pair<float, int>& a, const std::pair<float, int>& b) {
    return a.first > b.first;
}

// Softmax
vector<float> softmax(const vector<float>& input) {
    vector<float> output(input.size());
    float sum = 0;
    for (int i = 0; i < input.size(); ++i) {
        output[i] = exp(input[i]);
        sum += output[i];
    }
    for (int i = 0; i < output.size(); ++i) {
        output[i] /= sum;
    }
    return output;
}

void Controller::packet2flit(vector<float> data, int dst, int data_type){
    // packet to flits and give them to NoC (PE0, dest_id = 0)
    int src = 15;
    for (int i = 0; i < data.size() + 2; ++i) {
        sc_lv<34> flit;
        string type;
        if (i == 0) type = "10";  // header
        else if (i == data.size() + 1) type = "01";  // tail
        else type = "00";  // body

        flit.range(33, 32) = type.c_str();
        if (type == "10" || type == "01") {
            flit.range(31, 28) = sc_lv<4>(src);
            flit.range(27, 24) = sc_lv<4>(dst);
            flit.range(23, 22) = sc_lv<2>(data_type);  // use only 2 bits
            flit.range(21, 0)  = 0; // optional: padding
        }
        else {
            uint32_t bits;
            if(i == data.size())
                cout << "weight end: " << data[i - 1] << endl;
            std::memcpy(&bits, &data[i - 1], sizeof(float));
            flit.range(31, 0) = sc_lv<32>(bits);  // full float body
        }

        req_tx.write(true);
        flit_tx.write(flit);
        do { wait(); } while (!ack_tx.read());
        req_tx.write(false);
    }
}

void Controller::fetch() {
    vector<float> input_data;  // collect all input / weights / biases
    int data_type;
    while(true){
        wait();

        if (rst.read()) {
            req_tx.write(false);
            layer_id_valid.write(false);
            curr_layer = -1;
            curr_type = 0;
            wait();  
            continue;
        }

        if (curr_type == 0 && curr_layer != -1 && curr_layer != 0) {
            data_type = BIAS;
            curr_type = 1;
        }
        else { 
            data_type = WEIGHT;
            curr_type = 0; 
            curr_layer++; 
        }
        if(curr_layer == 0) data_type = INPUT;
        if(curr_layer < 7){
            layer_id_valid.write(true);
            layer_id.write(curr_layer);
            layer_id_type.write(curr_type);
            wait();
            layer_id_valid.write(false);

            do{
                wait();
            } while (!data_valid.read());

            while (true) {
                if (data_valid.read()) {
                    input_data.push_back(data.read());
                } 
                else {
                    packet2flit(input_data, curr_layer, data_type);
                    input_data.clear();
                    // cout << "input packet completed!" << endl;
                    break;
                }   
                wait();
            }
        }
    }
}

void Controller::receive(){
    // Wait for result (from PE9)
    std::vector<float> result;
    bool set;
    while (true) {
        wait();
        if (rst.read()) {
            ack_rx.write(false);
            set = false;
            continue;
        }
        if (req_rx.read()) {
            sc_lv<34> flit = flit_rx.read();
            sc_lv<2> type = flit.range(33, 32);
            if (type == "00") {
                // body flit → data
                uint32_t bits = flit.range(31, 0).to_uint();
                float val;
                std::memcpy(&val, &bits, sizeof(float));
                // append to correct vector (based on current receiving context)
                result.push_back(val);
            }
            if (type == "01") {
                // cout << "Contorller: Read..." << endl;
                // open classes file
                ifstream file("data/imagenet_classes.txt");
                vector<string> img_class;
                string line;
                while (getline(file, line)) {
                    img_class.push_back(line);
                }
                file.close();

                vector<float> possibility = softmax(result);
                std::vector<std::pair<float, int> > scores;
                for (int i = 0; i < result.size(); ++i)
                    scores.push_back(std::make_pair(result[i], i));
                
                std::sort(scores.begin(), scores.end(), comparePairs);
                std::sort(possibility.begin(), possibility.end(), greater<float>());

                cout << "Top 100 classes:" << endl;
                cout << "=================================================" << endl;
                cout << right << setw(5) << "idx"
                    << " | " << setw(8) << "val"
                    << " | " << setw(11) << "possibility"
                    << " | " << "class name" << endl;
                cout << "-------------------------------------------------" << endl;

                for (int i = 0; i < 100 && i < scores.size(); ++i) {
                    int idx = scores[i].second;
                    cout << right << setw(5) << idx
                        << " | " << setw(8) << fixed << setprecision(2) << scores[i].first
                        << " | " << setw(11) << fixed << setprecision(2) << possibility[i] * 100
                        << " | " /*<< img_class[idx]*/ << endl;
                }

                cout << "=================================================" << endl;

                sc_stop();
            }
            ack_rx.write(true);
        } else {
            ack_rx.write(false);
        }
    }
}



