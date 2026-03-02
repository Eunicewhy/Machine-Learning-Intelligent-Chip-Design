// ROM.h
#ifndef ROM_H
#define ROM_H

#include "systemc.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

SC_MODULE( ROM ) {
    sc_in  < bool >  clk;
    sc_in  < bool >  rst;
    
    // AXI4 Slave Interface - Read Address Channel
    sc_in  < sc_lv<4> >  ARID;       // Read address ID
    sc_in  < sc_lv<32> > ARADDR;     // Read address
    sc_in  < sc_lv<8> >  ARLEN;      // Burst length
    sc_in  < sc_lv<3> >  ARSIZE;     // Burst size
    sc_in  < sc_lv<2> >  ARBURST;    // Burst type
    sc_in  < bool >      ARVALID;    // Read address valid
    sc_out < bool >      ARREADY;    // Read address ready

    // AXI4 Slave Interface - Read Data Channel
    sc_out < sc_lv<4> >  RID;        // Read ID tag
    sc_out < sc_lv<32> > RDATA;      // Read data
    sc_out < sc_lv<2> >  RRESP;      // Read response
    sc_out < bool >      RLAST;      // Read last
    sc_out < bool >      RVALID;     // Read valid
    sc_in  < bool >      RREADY;     // Read ready

    // Load all data files into memory at startup
    void load_data_files();
    uint32_t read_memory(uint32_t addr);

    // Memory storage
    std::map<uint32_t, float> memory;
    
    // Data file management
    string DATA_PATH;
    string IMAGE_FILE_NAME;
    
    // Read transaction state
    sc_lv<4> current_rid;
    uint32_t current_addr;
    int remaining_beats;
    bool transaction_active;

    SC_CTOR( ROM ) {
        DATA_PATH = "./data/";
        
        const char* env_file = getenv("IMAGE_FILE_NAME");
        if (env_file != NULL) {
            IMAGE_FILE_NAME = env_file;
        } else {
            IMAGE_FILE_NAME = "cat.txt";
        }

        SC_THREAD( run );
        sensitive << clk.pos();

        load_data_files();
    }
    void load_file(const string& filename, uint32_t base_addr);
    void run();
};
#endif