#include "core.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

enum DATA { INPUT = 0, WEIGHT = 1, BIAS = 2, OUTPUT = 3 };

// Zero Padding
vector<float> Core::zero_pad_flat(const vector<float>& input, int c, int h, int w,
                       int pad_top, int pad_left, int pad_bottom, int pad_right) {
    int new_h = h + pad_top + pad_bottom;
    int new_w = w + pad_left + pad_right;
    vector<float> output(c * new_h * new_w, 0.0f);
    for (int ch = 0; ch < c; ++ch) {
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                output[ch * new_h * new_w + (i + pad_top) * new_w + (j + pad_left)] =
                    input[ch * h * w + i * w + j];
            }
        }
    }
    return output;
}

// Convolutional Layer + ReLU
vector<float> Core::conv_layer(const vector<float>& input, const vector<float>& weight, const vector<float>& bias,
                   int in_c, int in_h, int in_w, int f_size, int out_c, int stride, int pad) {
    int out_h = (in_h - f_size + 2 * pad) / stride + 1;
    int out_w = (in_w - f_size + 2 * pad) / stride + 1;
    vector<float> output(out_c * out_h * out_w);

    for (int oc = 0; oc < out_c; ++oc) {
        for (int oh = 0; oh < out_h; ++oh) {
            for (int ow = 0; ow < out_w; ++ow) {
                float sum = 0.0f;
                for (int ic = 0; ic < in_c; ++ic) {
                    for (int i = 0; i < f_size; ++i) {
                        for (int j = 0; j < f_size; ++j) {
                            int in_i = oh * stride + i - pad;
                            int in_j = ow * stride + j - pad;
                            float in_val = 0.0f;
                            if (in_i >= 0 && in_j >= 0 && in_i < in_h && in_j < in_w) {
                                int input_idx = ic * in_h * in_w + in_i * in_w + in_j;
                                in_val = input[input_idx];
                            }
                            int filter_idx = oc * in_c * f_size * f_size + ic * f_size * f_size + i * f_size + j;
                            sum += in_val * weight[filter_idx];
                        }
                    }
                }
                float val = sum + bias[oc];
                val = (val > 0) ? val : 0; // ReLU
                output[oc * out_h * out_w + oh * out_w + ow] = val;
            }
        }
    }
    return output;
}

// Max Pooling
vector<float> Core::max_pool(const vector<float>& input, int c, int h, int w,
                  int pool_size, int stride) {
    int out_h = (h - pool_size) / stride + 1;
    int out_w = (w - pool_size) / stride + 1;
    vector<float> output(c * out_h * out_w);

    for (int ch = 0; ch < c; ++ch) {
        for (int i = 0; i < out_h; ++i) {
            for (int j = 0; j < out_w; ++j) {
                float max_val = -1e9f;
                for (int m = 0; m < pool_size; ++m) {
                    for (int n = 0; n < pool_size; ++n) {
                        int r = i * stride + m;
                        int cidx = j * stride + n;
                        int idx = ch * h * w + r * w + cidx;
                        if (r < h && cidx < w) {
                            max_val = max(max_val, input[idx]);
                        }
                    }
                }
                output[ch * out_h * out_w + i * out_w + j] = max_val;
            }
        }
    }
    return output;
}

// Fully Connected Layer
vector<float> Core::fully_connect(const vector<float>& input, const vector<float>& weight, const vector<float>& bias,
                       int input_size, int num_neurons) {
    vector<float> output(num_neurons);
    for (int n = 0; n < num_neurons; ++n) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; ++i)
            sum += input[i] * weight[n * input_size + i];
        float val = sum + bias[n];
        if(core_id != 8) val = (val > 0) ? val : 0;
        output[n] = val;
    }
    return output;
}

void Core::receive() {
    std::vector<float> weight;
    std::vector<float> bias;
    std::vector<float> input_data;
    bool has_input;
    bool has_weight;
    bool has_bias;
    bool receive;

    while (true) {
        wait();
        if (rst_n.read()) {            
            ack_rx.write(false);
            has_input = false;
            has_weight = false;
            has_bias = false;
            receive = false;
            continue;
        }

        if(req_rx.read()) {
            sc_lv<34> flit = flit_rx.read();
            std::string type = flit.range(33, 32).to_string();
            if(type == "10"){  // head
                receive = true;
                data_type = flit.range(23, 22).to_uint();
            }
            else if(type == "00"){  // body
                uint32_t bits = flit.range(31, 0).to_uint();
                float val;
                std::memcpy(&val, &bits, sizeof(float));
            
                if (data_type == INPUT) input_data.push_back(val);
                else if (data_type == WEIGHT) weight.push_back(val);
                else if (data_type == BIAS) bias.push_back(val);
            }
            else if (type == "01") {  // tail flit
                if (data_type == INPUT) has_input = true;
                else if (data_type == WEIGHT) has_weight = true;
                else if (data_type == BIAS) has_bias = true;
                // cout << "Core: tail" << endl;
            }
            ack_rx.write(true);
            continue;
        }
        else{
            ack_rx.write(false);
        }

        // executing the layer according to PE ID
        if (core_id == 0 && has_input) {                
            cout << "-----------------------------------" << endl;
            cout << "core_id: " << core_id << " start!" << endl;
            cout << "input_data.size(): " << input_data.size() << endl;
            result = zero_pad_flat(input_data, 3, 224, 224, 2, 2, 1, 1);
            cout << "core_id: " << core_id << " done!" << endl;
            cout << "result.size(): " << result.size() << endl;
            input_data.clear();
            weight.clear();
            bias.clear();
            send_req = true;
            has_input = false;
            has_weight = false;
            has_bias = false;
        } 
        else if(has_input && has_weight && has_bias){
            cout << "-----------------------------------" << endl;
            cout << "core_id: " << core_id << " start!" << endl;
            cout << "input_data.size(): " << input_data.size() << endl;
            cout << "weight.size(): " << weight.size() << endl;
            cout << "bias.size(): " << bias.size() << endl;
            cout << "input_data[0]: " << input_data[0] << endl;
            cout << "weight[0]: " << weight[0] << endl;
            // for (int i = 0; i < bias.size(); i++){
            //     cout << "bias[i]: " << bias[i] << " ";
            // }
            if (core_id == 1) {
                // cout << "weight.size(): " << weight.size() << endl;
                // cout << "bias.size(): " << bias.size() << endl;
                vector<float> conv = conv_layer(input_data, weight, bias, 3, 227, 227, 11, 64, 4, 0);
                result = max_pool(conv, 64, 55, 55, 3, 2);
            } 
            else if (core_id == 2) {
                vector<float> conv = conv_layer(input_data, weight, bias, 64, 27, 27, 5, 192, 1, 2);
                result = max_pool(conv, 192, 27, 27, 3, 2);
            } 
            else if (core_id == 3) {
                result = conv_layer(input_data, weight, bias, 192, 13, 13, 3, 384, 1, 1);
            } 
            else if (core_id == 4) {
                result = conv_layer(input_data, weight, bias, 384, 13, 13, 3, 256, 1, 1);
            } 
            else if (core_id == 5) {
                vector<float> conv = conv_layer(input_data, weight, bias, 256, 13, 13, 3, 256, 1, 1);
                result = max_pool(conv, 256, 13, 13, 3, 2);
            } 
            else if (core_id == 6) {
                // cout << endl;
                // for(int i = 0; i < input_data.size(); i++){
                //     cout << input_data[i] << " ";
                // }
                result = fully_connect(input_data, weight, bias, 9216, 4096);
            } 
            else if (core_id == 7) {
                result = fully_connect(input_data, weight, bias, 4096, 4096);
            } 
            else if (core_id == 8) {
                result = fully_connect(input_data, weight, bias, 4096, 1000);
            } 
            cout << "core_id: " << core_id << " done!" << endl;
            cout << "result.size(): " << result.size() << endl;
            
            input_data.clear();
            weight.clear();
            bias.clear();
            send_req = true;
            has_input = false;
            has_weight = false;
            has_bias = false;
        }
        
    }
}

void Core::send(){
    while(true){
        wait();
        if (rst_n.read()) {
            req_tx.write(false);
            flit_tx.write(0);
            continue;
        }
        // send flits to next PE（or Controller）
        if(send_req){
            do{ wait(); } while(req_rx.read());
            for (int i = 0; i < result.size() + 2; ++i) {
                sc_lv<34> flit;
                string type = (i == 0) ? "10" : (i == result.size() + 1) ? "01" : "00";
                
                flit.range(33, 32) = type.c_str();
                if(type == "10" || type == "01"){
                    int num = 6;
                    flit.range(31, 28) = sc_lv<4>(core_id);
                    flit.range(27, 24) = (core_id < num) ? sc_lv<4>(core_id + 1) : sc_lv<4>(0); // next PE or Controller
                    flit.range(23, 22) = (core_id < num) ? INPUT : OUTPUT;
                }
                else{
                    uint32_t bits;
                    std::memcpy(&bits, &result[i - 1], sizeof(float));
                    flit.range(31, 0) = bits;
                }
                req_tx.write(true);
                flit_tx.write(flit);
                do { wait(); } while (!ack_tx.read());
                req_tx.write(false);
                // cout << "sending..." << endl;
                // wait();
            }
            send_req = false;
            cout << "core send!" << endl;
            cout << "-----------------------------------" << endl;
        }
    }
}
