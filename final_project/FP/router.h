#ifndef ROUTER_H
#define ROUTER_H
#include "systemc.h"
#include <deque>

// enumeration for directions
enum Dir { EAST=0, SOUTH=1, WEST=2, NORTH=3, LOCAL=4 };
#define OUTPUT 3

SC_MODULE(Router) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;

    sc_out < sc_lv<34> >  out_flit[5];
    sc_out < bool >  out_req[5];
    sc_in  < bool >  in_ack[5];

    sc_in  < sc_lv<34> >  in_flit[5];
    sc_in  < bool >  in_req[5];
    sc_out < bool >  out_ack[5];

    int router_id;      // router id
    int stored_o[5];    // store selected output direction for each input
    int locked_by[5];   // output port lock owner (-1 if free)
    std::deque<sc_lv<34> > in_buf[5];
    std::deque<sc_lv<34> > out_buf[5];

    SC_CTOR(Router) {
        SC_THREAD(route);
        sensitive << clk.pos();
    }

    void route();
    int compute_dir(int dest, int type);
    bool send_to_controller;
};

void Router::route() {
    send_to_controller = false;
    // Reset all state and buffers
    for (int i = 0; i < 5; ++i) { 
        out_req[i].write(false);
        out_ack[i].write(false);

        in_buf[i].clear();
        out_buf[i].clear();
        stored_o[i] = -1;
        locked_by[i] = -1;
    }
    wait();     // wait for reset
    int num = 0;

    while (true) {
        // receive flits
        for (int i = 0; i < 5; ++i) {
            if (in_req[i].read()) {
                sc_lv<34> flit = in_flit[i].read();
                in_buf[i].push_back(flit);
                out_ack[i].write(true);
                ++num;
            } 
            else {
                out_ack[i].write(false);
            }
        }
        // routing decision and forwarding to output buffer
        // calculate direction only if it is header flit and locked it
        for (int i = 0; i < 5; ++i) {
            if (!in_buf[i].empty()) {
                sc_lv<34> f = in_buf[i].front();
                std::string type = f.range(33, 32).to_string();
                if (stored_o[i] == -1) {
                    // header
                    if (type == "10") {
                        // cout << "head router: " << router_id << " ";
                        int dst = f.range(27, 24).to_uint();  // get destination
                        int type = f.range(23, 22).to_uint();  // get type
                        // cout << "type: " << type << endl;
                        int o = compute_dir(dst, type);   // compute destination
                        // cout << "locked_by[o]: " << locked_by[o] << endl;
                        // cout << "direction: " << o << endl;
                        if (locked_by[o] == -1) {
                            stored_o[i] = o;
                            locked_by[o] = i;
                        }
                    }
                }

                int o = stored_o[i];
                if (locked_by[o] == i && out_buf[o].size() < 4) {
                    out_buf[o].push_back(f);
                    in_buf[i].pop_front();
                    // tail
                    if (type == "01") {
                        // cout << "router: " << router_id;
                        // cout << " dst: " << f.range(27, 24).to_uint() << " done" << endl;
                        // cout << ", data_type: " << f.range(23, 22).to_uint() << endl;
                        stored_o[i] = -1;
                        locked_by[o] = -1;
                    }
                }
            }
        }

        // transmit flit
        for (int o = 0; o < 5; ++o) {
            if (!out_buf[o].empty()) {
                out_flit[o].write(out_buf[o].front());
                out_req[o].write(true);

                if (in_ack[o].read()) {
                    out_buf[o].pop_front();
                }
            } 
            else {
                out_req[o].write(false);
            }
        }
        // cout << "router sent packet" << endl;
        wait(); // wait for next clock cycle
    }
}

// calculate destination (route in x direction first, then y)
int Router::compute_dir(int dest, int type) {
    int NOC_SIZE = 3;
    int x  = router_id % NOC_SIZE;
    int y = router_id / NOC_SIZE;
    int dx = dest % NOC_SIZE;
    int dy = dest / NOC_SIZE;

    if (router_id == 0 && type == OUTPUT) return NORTH;
    // if (router_id == 8 && dest == 0) return WEST;
    // if (x - dx == -2){
    //     if (y == 1) return WEST;
    //     if (y == 2) return SOUTH;
    // }
    // if (x - dx == 2){
    //     if (y == 1) return EAST;
    //     if (y == 2) return SOUTH;
    // }
    if (x < dx) return EAST;
    if (x > dx) return WEST;
    // if (abs(y - dy) == 2){
    //     if (x == 0) return WEST;
    //     if (x == 1) return (y < dy) ? NORTH : SOUTH;
    //     if (x == 2) return EAST;
    // }
    if (y < dy) return SOUTH;
    if (y > dy) return NORTH;
    return LOCAL;
}

#endif
