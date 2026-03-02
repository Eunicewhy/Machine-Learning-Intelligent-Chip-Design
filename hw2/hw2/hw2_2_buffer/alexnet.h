#ifndef ALEXNET_H
#define ALEXNET_H

#include <systemc.h>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace std;

// 定義三維動態陣列
typedef vector<vector<vector<float> > > Vector3D;
// 定義一維動態陣列
typedef vector<float> Vector1D;

Vector3D zero_pad(const Vector3D& input, int pad_top, int pad_left, int pad_bottom, int pad_right);
Vector3D Input(int height, int width, int channels, string file_name);
Vector3D Convolution(int num_filters, int filter_size, int stride_conv, int channels, int padding_conv, Vector3D padded_image, string conv_weight_file, string conv_bias_file);
Vector3D Max_Pooling(int num_filters, int pool_size, int stride_pool, Vector3D conv_output);
Vector1D Flatten(int num_filters, int pool_output_height, int pool_output_width, Vector3D pool_output);
Vector1D Fully_Connect(int input_size, int num_neurons, string fc_weight_file, string fc_bias_file, Vector1D fc_input);
Vector1D Soft_Max(Vector1D fc_output);
bool comparePairs(const pair<float, int>& a, const pair<float, int>& b);

// AlexNet Module
SC_MODULE(AlexNet) {
    // output port 傳遞 Softmax 與 Fully Connected Layer 最後一層的結果
    sc_out<float> softmax_output_result[1000];
    sc_out<float> fc_output_result[1000];
    sc_out<int> num_out;
    // 輸入影像資料
    string file_name;

    SC_CTOR(AlexNet) {
        // SC_THREAD(run);
        SC_METHOD(run);
    }
    void run();
};

// Monitor Module
SC_MODULE(Monitor) {
    // input port 連接 AlexNet Module 傳來的資料
    sc_in<float> in_prob[1000];
    sc_in<float> in_value[1000];
    sc_in<int> num_in;

    SC_CTOR(Monitor) {
        // SC_THREAD(monitor_output);
        SC_METHOD(monitor_output);
        sensitive << num_in;
        for (int i = 0; i < 1000; i++) {
            sensitive << in_prob[i] << in_value[i];
            // sensitive << in_value[i];
        }
        dont_initialize();
    }
    void monitor_output();
};

#endif
