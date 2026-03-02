// ROM.h
#ifndef ROM_H
#define ROM_H

#include <systemc.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
using namespace std;

SC_MODULE(ROM) {
    // system signals
    sc_in<bool>        clk;
    sc_in<bool>        rst;

    // layer-based control
    sc_in<int>         layer_id;        // 0=input, 1..8=conv/fc
    sc_in<bool>        layer_id_type;   // 0=weight, 1=bias (ignored when layer_id==0)
    sc_in<bool>        layer_id_valid;

    // AXI4 Slave Interface - Read Address Channel
    sc_in< sc_lv<4> >  ARID;
    sc_in< sc_lv<32> > ARADDR;
    sc_in< sc_lv<8> >  ARLEN;
    sc_in< sc_lv<3> >  ARSIZE;
    sc_in< sc_lv<2> >  ARBURST;
    sc_in<bool>        ARVALID;
    sc_out<bool>       ARREADY;

    // AXI4 Slave Interface - Read Data Channel
    sc_out< sc_lv<4> >  RID;
    sc_out< sc_lv<32> > RDATA;
    sc_out< sc_lv<2> >  RRESP;
    sc_out<bool>        RLAST;
    sc_out<bool>        RVALID;
    sc_in<bool>         RREADY;

    string DATA_PATH ;
    string IMAGE_FILE_NAME;     

    vector<float>       file_buf;
    sc_lv<4>            current_rid;
    unsigned int        beat_index;
    unsigned int        total_beats;
    sc_lv<2>            current_burst;
    bool                transaction_active;

    SC_CTOR( ROM )
    {
        DATA_PATH = "./data/";      // Please change this to your own data path

        const char* env_file = getenv("IMAGE_FILE_NAME");
        //IMAGE_FILE_NAME = "cat.txt"; // You can change this to test another image file
        if (env_file != NULL) {
            IMAGE_FILE_NAME = env_file;
        } else {
            IMAGE_FILE_NAME = "cat.txt"; // default fallback
        }
        SC_THREAD( run );
        sensitive << clk.pos() << rst.neg();
    }
    void run();
};

#endif // ROM_H