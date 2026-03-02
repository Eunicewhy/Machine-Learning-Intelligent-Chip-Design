#include "systemc.h"
#include "core.h"
#include "router.h"
#include "clockreset.h"
#include "controller.h"
#include "ROM.h"
#include <sstream>

#define NOC_SIZE 4
// enum Dir { EAST = 0, SOUTH = 1, WEST = 2, NORTH = 3, LOCAL = 4 };

// local connection
void connect_local(Core* core, Router* router, sc_signal<sc_lv<34> > (&flit)[2],
                   sc_signal<bool> (&req)[2], sc_signal<bool> (&ack)[2]) {
    core->flit_tx(flit[0]);
    core->req_tx(req[0]);
    router->in_flit[LOCAL](flit[0]);
    router->in_req[LOCAL](req[0]);
    router->in_ack[LOCAL](ack[0]);
    core->ack_rx(ack[0]);

    router->out_flit[LOCAL](flit[1]);
    router->out_req[LOCAL](req[1]);
    core->flit_rx(flit[1]);
    core->req_rx(req[1]);
    core->ack_tx(ack[1]);
    router->out_ack[LOCAL](ack[1]);
}

// connection between routers
void connect_dir(Router* from, Router* to, Dir out_dir, Dir in_dir,
                 sc_signal<sc_lv<34> >& flit, sc_signal<bool>& req, sc_signal<bool>& ack) {
    from->out_flit[out_dir](flit);
    from->out_req[out_dir](req);
    from->in_ack[out_dir](ack);
    to->in_flit[in_dir](flit);
    to->in_req[in_dir](req);
    to->out_ack[in_dir](ack);
}

// dummy port connections (for edges)
void connect_edge(Router* r, Dir dir,
                  sc_signal<sc_lv<34> >& flit_out, sc_signal<bool>& req_out, sc_signal<bool>& ack_in,
                  sc_signal<sc_lv<34> >& flit_in,  sc_signal<bool>& req_in,  sc_signal<bool>& ack_out) {
    r->out_flit[dir](flit_out);
    r->out_req[dir](req_out);
    r->in_ack[dir](ack_in);
    r->in_flit[dir](flit_in);
    r->in_req[dir](req_in);
    r->out_ack[dir](ack_out);
}

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clk, rst;

    Clock clock("clock", 10);
    Reset reset("reset", 20);
    clock(clk);
    reset(rst);

    // Controller / ROM connection with AXI4 interface
    Controller* controller = new Controller("controller");
    ROM* rom = new ROM("rom");

    // AXI4 Read Address Channel signals
    sc_signal<sc_lv<4> >  axi_arid;
    sc_signal<sc_lv<32> > axi_araddr;
    sc_signal<sc_lv<8> >  axi_arlen;
    sc_signal<sc_lv<3> >  axi_arsize;
    sc_signal<sc_lv<2> >  axi_arburst;
    sc_signal<bool>       axi_arvalid;
    sc_signal<bool>       axi_arready;

    // AXI4 Read Data Channel signals
    sc_signal<sc_lv<4> >  axi_rid;
    sc_signal<sc_lv<32> > axi_rdata;
    sc_signal<sc_lv<2> >  axi_rresp;
    sc_signal<bool>       axi_rlast;
    sc_signal<bool>       axi_rvalid;
    sc_signal<bool>       axi_rready;

    sc_signal<int>        l_id;       // '0' means input data
    sc_signal<bool>       l_id_type;  // '0' weight, '1' bias
    sc_signal<bool>       l_id_valid;

    // Connect Controller (Master)
    controller->clk(clk);
    controller->rst(rst);
    
    // AXI4 Read Address Channel - Controller as Master
    controller->ARID(axi_arid);
    controller->ARADDR(axi_araddr);
    controller->ARLEN(axi_arlen);
    controller->ARSIZE(axi_arsize);
    controller->ARBURST(axi_arburst);
    controller->ARVALID(axi_arvalid);
    controller->ARREADY(axi_arready);

    // AXI4 Read Data Channel - Controller as Master
    controller->RID(axi_rid);
    controller->RDATA(axi_rdata);
    controller->RRESP(axi_rresp);
    controller->RLAST(axi_rlast);
    controller->RVALID(axi_rvalid);
    controller->RREADY(axi_rready);

    controller->layer_id(l_id);
    controller->layer_id_type(l_id_type);    
    controller->layer_id_valid(l_id_valid);

    // Connect ROM (Slave)
    rom->clk(clk);
    rom->rst(rst);
    
    // AXI4 Read Address Channel - ROM as Slave
    rom->ARID(axi_arid);
    rom->ARADDR(axi_araddr);
    rom->ARLEN(axi_arlen);
    rom->ARSIZE(axi_arsize);
    rom->ARBURST(axi_arburst);
    rom->ARVALID(axi_arvalid);
    rom->ARREADY(axi_arready);

    // AXI4 Read Data Channel - ROM as Slave
    rom->RID(axi_rid);
    rom->RDATA(axi_rdata);
    rom->RRESP(axi_rresp);
    rom->RLAST(axi_rlast);
    rom->RVALID(axi_rvalid);
    rom->RREADY(axi_rready);
    rom->layer_id(l_id);
    rom->layer_id_type(l_id_type);  
    rom->layer_id_valid(l_id_valid);


    // Router/Controller connection（ Router[0][0] LOCAL port）
    sc_signal<sc_lv<34> > ctrl_flit[2];
    sc_signal<bool> ctrl_req[2], ctrl_ack[2];

    // NoC mesh building
    Core* cores[NOC_SIZE][NOC_SIZE];
    Router* routers[NOC_SIZE][NOC_SIZE];

    sc_signal<sc_lv<34> > flit_local[NOC_SIZE][NOC_SIZE][2];
    sc_signal<bool> req_local[NOC_SIZE][NOC_SIZE][2];
    sc_signal<bool> ack_local[NOC_SIZE][NOC_SIZE][2];

    sc_signal<sc_lv<34> > flit[NOC_SIZE][NOC_SIZE][4];
    sc_signal<bool> req[NOC_SIZE][NOC_SIZE][4];
    sc_signal<bool> ack[NOC_SIZE][NOC_SIZE][4];

    sc_signal<sc_lv<34> > dummy_flit1[NOC_SIZE][NOC_SIZE][4], dummy_flit2[NOC_SIZE][NOC_SIZE][4];
    sc_signal<bool> dummy_req1[NOC_SIZE][NOC_SIZE][4], dummy_req2[NOC_SIZE][NOC_SIZE][4];
    sc_signal<bool> dummy_ack1[NOC_SIZE][NOC_SIZE][4], dummy_ack2[NOC_SIZE][NOC_SIZE][4];

    for (int i = 0; i < NOC_SIZE; ++i) {
        for (int j = 0; j < NOC_SIZE; ++j) {
            std::ostringstream name;
            int id = i * NOC_SIZE + j;

            name << "Router_" << id;
            routers[i][j] = new Router(name.str().c_str());
            routers[i][j]->clk(clk);
            routers[i][j]->rst(rst);
            routers[i][j]->router_id = id;  // set router ID

            name.str(""); name.clear();
            name << "Core_" << id;
            cores[i][j] = new Core(name.str().c_str());
            cores[i][j]->clk(clk);
            cores[i][j]->rst_n(rst);
            cores[i][j]->core_id = id;

            connect_local(cores[i][j], routers[i][j],
                                            flit_local[i][j], req_local[i][j], ack_local[i][j]);
        }
    }

    // Controller <-> Router[0][0] LOCAL
    controller->flit_tx(ctrl_flit[0]);
    controller->req_tx(ctrl_req[0]);
    routers[0][0]->in_flit[NORTH](ctrl_flit[0]);
    routers[0][0]->in_req[NORTH](ctrl_req[0]);
    routers[0][0]->in_ack[NORTH](ctrl_ack[0]);
    controller->ack_rx(ctrl_ack[0]);

    routers[0][0]->out_flit[NORTH](ctrl_flit[1]);
    routers[0][0]->out_req[NORTH](ctrl_req[1]);
    controller->flit_rx(ctrl_flit[1]);
    controller->req_rx(ctrl_req[1]);
    controller->ack_tx(ctrl_ack[1]);
    routers[0][0]->out_ack[NORTH](ctrl_ack[1]);


    // Mesh direction connection
    for (int i = 0; i < NOC_SIZE; ++i) {
        for (int j = 0; j < NOC_SIZE; ++j) {
            if (j < NOC_SIZE-1)
                connect_dir(routers[i][j], routers[i][j+1], EAST, WEST, flit[i][j][EAST], req[i][j][EAST], ack[i][j][EAST]);
            else
                connect_edge(routers[i][j], EAST,
                             dummy_flit1[i][j][EAST], dummy_req1[i][j][EAST], dummy_ack1[i][j][EAST],
                             dummy_flit2[i][j][EAST], dummy_req2[i][j][EAST], dummy_ack2[i][j][EAST]);

            if (i < NOC_SIZE-1)
                connect_dir(routers[i][j], routers[i+1][j], SOUTH, NORTH, flit[i][j][SOUTH], req[i][j][SOUTH], ack[i][j][SOUTH]);
            else
                connect_edge(routers[i][j], SOUTH,
                             dummy_flit1[i][j][SOUTH], dummy_req1[i][j][SOUTH], dummy_ack1[i][j][SOUTH],
                             dummy_flit2[i][j][SOUTH], dummy_req2[i][j][SOUTH], dummy_ack2[i][j][SOUTH]);

            if (j > 0)
                connect_dir(routers[i][j], routers[i][j-1], WEST, EAST, flit[i][j][WEST], req[i][j][WEST], ack[i][j][WEST]);
            else
                connect_edge(routers[i][j], WEST,
                             dummy_flit1[i][j][WEST], dummy_req1[i][j][WEST], dummy_ack1[i][j][WEST],
                             dummy_flit2[i][j][WEST], dummy_req2[i][j][WEST], dummy_ack2[i][j][WEST]);

            if (i > 0)
                connect_dir(routers[i][j], routers[i-1][j], NORTH, SOUTH, flit[i][j][NORTH], req[i][j][NORTH], ack[i][j][NORTH]);
            else
                if(i!=0 || j!=0) connect_edge(routers[i][j], NORTH,
                             dummy_flit1[i][j][NORTH], dummy_req1[i][j][NORTH], dummy_ack1[i][j][NORTH],
                             dummy_flit2[i][j][NORTH], dummy_req2[i][j][NORTH], dummy_ack2[i][j][NORTH]);
        }
    }

    sc_start();
    return 0;
}
