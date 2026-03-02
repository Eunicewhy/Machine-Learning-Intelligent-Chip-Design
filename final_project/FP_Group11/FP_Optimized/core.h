// core.h
#ifndef CORE_H
#define CORE_H

#include "systemc.h"
#include <vector>
using namespace std;

SC_MODULE( Core ) {
    sc_in  < bool >  rst_n;
    sc_in  < bool >  clk;
    // receive
    sc_in  < sc_lv<34> > flit_rx;	// The input channel
    sc_in  < bool > req_rx;	        // The request associated with the input channel
    sc_out < bool > ack_rx;	        // The outgoing ack signal associated with the input channel
    // transmit
    sc_out < sc_lv<34> > flit_tx;	// The output channel
    sc_out < bool > req_tx;	        // The request associated with the output channel
    sc_in  < bool > ack_tx;	        // The outgoing ack signal associated with the output channel

    int core_id;

    SC_CTOR(Core) {
        SC_THREAD(receive);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(send);
        sensitive << clk.pos();
        dont_initialize();
    }
    void receive();
    void send();

    bool send_req;
    int data_type;

    std::vector<float> result;
    long data_in_pos;

    long layer_compute_start;
    long layer_compute_end ;

    vector<float> zero_pad_flat(const vector<float>& input, int c, int h, int w, int pad_top, int pad_left, int pad_bottom, int pad_right);
    vector<float> conv_layer(const vector<float>& input, const vector<float>& weight, const vector<float>& bias, int in_c, int in_h, int in_w, int f_size, int out_c, int stride, int pad);
    vector<float> max_pool(const vector<float>& input, int c, int h, int w, int pool_size, int stride);
    vector<float> fully_connect(const vector<float>& input, const vector<float>& weight, const vector<float>& bias, int input_size, int num_neurons);
};

#endif