#include "systemc.h"
#include "core.h"
#include "router.h"
#include "clockreset.h"
#include <sstream>

#define NoC_SIZE 4

// enumeration for directions
// enum Dir { EAST = 0, SOUTH = 1, WEST = 2, NORTH = 3, LOCAL = 4 };

// local port connections between cores and routers
void connect_local(Core* core, Router* router, sc_signal<sc_lv<34> > (&flit)[2], sc_signal<bool> (&req)[2], sc_signal<bool> (&ack)[2]) {
    // core (tx) -> router
    core->flit_tx(flit[0]);              // sc_out<sc_lv<34> > flit_tx;
    router->in_flit[LOCAL](flit[0]);     // sc_in<sc_lv<34> > in_flit[5];
    core->req_tx(req[0]);               // sc_out<bool> req_tx;
    router->in_req[LOCAL](req[0]);      // sc_in<bool> in_req[5];
    router->out_ack[LOCAL](ack[1]);     // sc_out<bool> out_ack[5];
    core->ack_tx(ack[1]);               // sc_in<bool> ack_tx;

    // router -> core (rx)
    router->out_flit[LOCAL](flit[1]);   // sc_out<sc_lv<34> > out_flit[5];
    core->flit_rx(flit[1]);             // sc_in<sc_lv<34> > flit_rx;
    router->out_req[LOCAL](req[1]);     // sc_out<bool> out_req[5];
    core->req_rx(req[1]);               // sc_in<bool> req_rx;
    core->ack_rx(ack[0]);               // sc_out<bool> ack_rx;
    router->in_ack[LOCAL](ack[0]);      // sc_in<bool> in_ack[5];
}

// router to router connections based on directions
void connect_dir(Router* from, Router* to, Dir out_dir, Dir in_dir, sc_signal<sc_lv<34> >& flit, sc_signal<bool>& req, sc_signal<bool>& ack) {
    // send packet
    from->out_flit[out_dir](flit); 
    from->out_req[out_dir](req); 
    from->in_ack[out_dir](ack);
    
    // receive packet
    to->in_flit[in_dir](flit);     
    to->in_req[in_dir](req);     
    to->out_ack[in_dir](ack);
}

// edge connections for the last row and column (assuming edge connections are similar to router connections, but not connected to other routers)
void connect_edge(Router* r, Dir dir, sc_signal<sc_lv<34> >& flit1, sc_signal<bool>& req1, sc_signal<bool>& ack1, sc_signal<sc_lv<34> >& flit2, sc_signal<bool>& req2, sc_signal<bool>& ack2) {
    r->out_flit[dir](flit1);
    r->out_req[dir](req1);
    r->in_ack[dir](ack1);
    r->in_flit[dir](flit2);
    r->in_req[dir](req2);
    r->out_ack[dir](ack2);
}

int sc_main(int argc, char* argv[]) {
    // =======================
    //   signals declaration
    // =======================
    sc_signal < bool > clk;
    sc_signal < bool > rst;
    
    // local connections
    sc_signal<sc_lv<34> > flit_local[NoC_SIZE][NoC_SIZE][2]; 
    sc_signal<bool> req_local[NoC_SIZE][NoC_SIZE][2];
    sc_signal<bool> ack_local[NoC_SIZE][NoC_SIZE][2];

    // router to router connections
    sc_signal<sc_lv<34> > flit[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> req[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> ack[NoC_SIZE][NoC_SIZE][4];

    // edge connections for the last row and column
    sc_signal<sc_lv<34> > edge_flit1[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> edge_req1[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> edge_ack1[NoC_SIZE][NoC_SIZE][4];

    sc_signal<sc_lv<34> > edge_flit2[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> edge_req2[NoC_SIZE][NoC_SIZE][4];
    sc_signal<bool> edge_ack2[NoC_SIZE][NoC_SIZE][4];

    // =======================
    //   modules declaration
    // =======================
    Clock m_clock("m_clock", 10);
    Reset m_reset("m_reset", 15);
    
    // create routers and cores (4x4 NoC)
    Router* routers[NoC_SIZE][NoC_SIZE];
    Core* cores[NoC_SIZE][NoC_SIZE];

    // =======================
    //   modules connection
    // =======================
    m_clock( clk ); 
    m_reset( rst );

    // Initialize routers and cores
    for (int i = 0; i < NoC_SIZE; i++) {
        for (int j = 0; j < NoC_SIZE; j++) {
            stringstream router_name, core_name;
            int id = i * NoC_SIZE + j;

            core_name << "Router_" << i << "_" << j;    // create unique names for routers
            routers[i][j] = new Router(router_name.str().c_str());  // create router instances
            routers[i][j]->clk(clk); 
            routers[i][j]->rst(rst); 
            routers[i][j]->router_id = id;  // set router ID

            router_name << "Core_" << i << "_" << j;    // create unique names for cores
            cores[i][j] = new Core(core_name.str().c_str());    // create core instances
            cores[i][j]->clk(clk);
            cores[i][j]->rst(rst);
            cores[i][j]->core_id = id;  // set core ID

            // local connections
            connect_local(cores[i][j], routers[i][j], flit_local[i][j], req_local[i][j], ack_local[i][j]);
        }
    }

    // Connect routers to routers (if) and edges (else)
    for (int i = 0; i < NoC_SIZE; i++) {
        for (int j = 0; j < NoC_SIZE; j++) {
            // EAST -> WEST connection
            if (j < NoC_SIZE - 1)
                connect_dir(routers[i][j], routers[i][j+1], EAST, WEST, flit[i][j][EAST], req[i][j][EAST], ack[i][j][EAST]);
            else
                connect_edge(routers[i][j], EAST, edge_flit1[i][j][EAST], edge_req1[i][j][EAST], edge_ack1[i][j][EAST], edge_flit2[i][j][EAST], edge_req2[i][j][EAST], edge_ack2[i][j][EAST]);
            // SOUTH -> NORTH connection
            if (i < NoC_SIZE - 1)
                connect_dir(routers[i][j], routers[i+1][j], SOUTH, NORTH, flit[i][j][SOUTH], req[i][j][SOUTH], ack[i][j][SOUTH]);
            else
                connect_edge(routers[i][j], SOUTH, edge_flit1[i][j][SOUTH], edge_req1[i][j][SOUTH], edge_ack1[i][j][SOUTH], edge_flit2[i][j][SOUTH], edge_req2[i][j][SOUTH], edge_ack2[i][j][SOUTH]);
            // WEST -> EAST connection
            if (j > 0)
                connect_dir(routers[i][j], routers[i][j-1], WEST, EAST, flit[i][j][WEST], req[i][j][WEST], ack[i][j][WEST]);
            else 
                connect_edge(routers[i][j], WEST, edge_flit1[i][j][WEST], edge_req1[i][j][WEST], edge_ack1[i][j][WEST], edge_flit2[i][j][WEST], edge_req2[i][j][WEST], edge_ack2[i][j][WEST]);
            // NORTH -> SOUTH connection
            if (i > 0)
                connect_dir(routers[i][j], routers[i-1][j], NORTH, SOUTH, flit[i][j][NORTH], req[i][j][NORTH], ack[i][j][NORTH]);
            else
                connect_edge(routers[i][j], NORTH, edge_flit1[i][j][NORTH], edge_req1[i][j][NORTH], edge_ack1[i][j][NORTH], edge_flit2[i][j][NORTH], edge_req2[i][j][NORTH], edge_ack2[i][j][NORTH]);
        }
    }
    
    // start simulation
    sc_start();
    return 0;
}
