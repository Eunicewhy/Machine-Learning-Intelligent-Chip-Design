// controller.h
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "systemc.h"
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
using namespace std;

enum DATA { INPUT = 0, WEIGHT = 1, BIAS = 2 };

SC_MODULE( Controller ) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;
    
    // AXI4 Master Interface - Read Address Channel
    sc_out < sc_lv<4> >  ARID;       // Read address ID
    sc_out < sc_lv<32> > ARADDR;     // Read address
    sc_out < sc_lv<8> >  ARLEN;      // Burst length
    sc_out < sc_lv<3> >  ARSIZE;     // Burst size
    sc_out < sc_lv<2> >  ARBURST;    // Burst type
    sc_out < bool >      ARVALID;    // Read address valid
    sc_in  < bool >      ARREADY;    // Read address ready

    // AXI4 Master Interface - Read Data Channel
    sc_in  < sc_lv<4> >  RID;        // Read ID tag
    sc_in  < sc_lv<32> > RDATA;      // Read data
    sc_in  < sc_lv<2> >  RRESP;      // Read response
    sc_in  < bool >      RLAST;      // Read last
    sc_in  < bool >      RVALID;     // Read valid
    sc_out < bool >      RREADY;     // Read ready

    // to router0
    sc_out < sc_lv<34> > flit_tx;
    sc_out < bool > req_tx;
    sc_in  < bool > ack_tx;

    // from router0
    sc_in  < sc_lv<34> > flit_rx;
    sc_in  < bool > req_rx;
    sc_out < bool > ack_rx;

    SC_CTOR(Controller) {
        SC_THREAD(fetch);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(receive);
        sensitive << clk.pos();
        dont_initialize();
    }

    void fetch();
    void receive();
    void packet2flit(std::vector<float> data, int dst, int data_type);
    void axi_read_transaction(sc_lv<32> addr, int burst_len);

    int curr_layer;
    int curr_type;
    
    // Memory mapping addresses (same as in controller)
    static const uint32_t INPUT_BASE_ADDR = 0x00000000;
    static const uint32_t CONV1_WEIGHT_ADDR = 0x01000000;
    static const uint32_t CONV1_BIAS_ADDR = 0x02000000;
    static const uint32_t CONV2_WEIGHT_ADDR = 0x03000000;
    static const uint32_t CONV2_BIAS_ADDR = 0x04000000;
    static const uint32_t CONV3_WEIGHT_ADDR = 0x05000000;
    static const uint32_t CONV3_BIAS_ADDR = 0x06000000;
    static const uint32_t CONV4_WEIGHT_ADDR = 0x07000000;
    static const uint32_t CONV4_BIAS_ADDR = 0x08000000;
    static const uint32_t CONV5_WEIGHT_ADDR = 0x09000000;
    static const uint32_t CONV5_BIAS_ADDR = 0x0A000000;
    static const uint32_t FC6_WEIGHT_ADDR = 0x0B000000;
    static const uint32_t FC6_BIAS_ADDR = 0x1B000000;
    static const uint32_t FC7_WEIGHT_ADDR = 0x2B000000;
    static const uint32_t FC7_BIAS_ADDR = 0x3B000000;
    static const uint32_t FC8_WEIGHT_ADDR = 0x4B000000;
    static const uint32_t FC8_BIAS_ADDR = 0x5B000000;
};
#endif