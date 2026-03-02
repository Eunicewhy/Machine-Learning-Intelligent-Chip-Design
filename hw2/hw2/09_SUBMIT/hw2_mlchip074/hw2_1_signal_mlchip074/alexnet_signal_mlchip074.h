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

// ��堆蕭��Ｘ��嚙踝蕭��砍��嚙踝蕭嚙踝蕭嚙踝蕭嚙踝蕭嚙踝蕭嚙�
typedef vector<vector<vector<float> > > Vector3D;
// ��堆蕭��Ｘ��嚙踝蕭��砍��嚙踝蕭嚙踝蕭嚙踝蕭嚙踝蕭嚙踝蕭嚙�
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
    // output port 嚙踝蕭��莎蕭嚙� Softmax 嚙踝蕭嚙� Fully Connected Layer 嚙踝蕭嚙賣�綽蕭���嚙賣����歹蕭嚙質�荔蕭嚙踝蕭嚙�
    sc_port<sc_signal_out_if<float> > softmax_output_result[1000];
    sc_port<sc_signal_out_if<float> > fc_output_result[1000];
    sc_port<sc_signal_out_if<int> > num_out;
    // ���閰剁蕭鈭����嚙踝蕭嚙賡��嚙踝蕭嚙踝蕭
    string file_name;

    SC_CTOR(AlexNet) {
        // SC_THREAD(run);
        SC_METHOD(run);
    }
    void run();
};

// Monitor Module
SC_MODULE(Monitor) {
    // input port 嚙踝蕭嚙踝蕭嚙踝蕭 AlexNet Module 嚙踝蕭���嚙踝蕭嚙踝蕭嚙賡��嚙踝蕭嚙踝蕭
    sc_port<sc_signal_in_if<float> > in_prob[1000];
    sc_port<sc_signal_in_if<float> > in_value[1000];
    sc_port<sc_signal_in_if<int> > num_in;

    SC_CTOR(Monitor) {
        // SC_THREAD(monitor_output);
        SC_METHOD(monitor_output);
        sensitive << num_in;
        for (int i = 0; i < 1000; i++) {
            sensitive << in_prob[i];
            sensitive << in_value[i];
        }
        dont_initialize();
    }
    void monitor_output();
};

#endif
