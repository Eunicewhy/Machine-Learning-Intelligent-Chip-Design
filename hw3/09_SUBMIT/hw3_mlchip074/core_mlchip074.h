#ifndef CORE_H
#define CORE_H

#include "systemc.h"
#include "pe.h"
#include <vector>
#include <cstring>  // for memcpy

// Flit format: [33:32]=type, [31:16]=src, [15:0]=dst
SC_MODULE(Core) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;
    // receive
    sc_in  < sc_lv<34> > flit_rx;	// The input channel
    sc_in  < bool > req_rx;	        // The request associated with the input channel
    sc_out < bool > ack_rx;	        // The outgoing ack signal associated with the input channel
    // transmit
    sc_out < sc_lv<34> > flit_tx;	// The output channel
    sc_out < bool > req_tx;	        // The request associated with the output channel
    sc_in  < bool > ack_tx;	        // The outgoing ack signal associated with the output channel

    PE pe;
    int core_id;    // unique core id

    // internal transmission state
    std::vector<sc_lv<34> > tx_buf;     // buffer to send flit
    unsigned tx_idx;     // current index in buffer
    bool packet_requested;              // flag to indicate if a packet has been requeseted

    // internal receive state
    std::vector<sc_lv<34> > rx_buf;     // buffer to receive flits

    SC_CTOR(Core){
        SC_THREAD(init_proc);
        sensitive << rst.pos();

        SC_THREAD(tx_proc);
        sensitive << clk.pos();

        SC_THREAD(rx_proc);
        sensitive << clk.pos();
    }

    void init_proc();   // initialize PE (calling pa.init)
    void tx_proc();     // get packets from PE and send it into flits
    void rx_proc();     // receive and check packets

    std::vector<sc_lv<34> > packet2flits(Packet* p);    // convert packet to flits
    Packet* flits2packet(const std::vector<sc_lv<34> >& v);    // convert flits to packet
};

void Core::init_proc() {
    pe.init(core_id);       // initialize PE and load input/output packets
    wait();         // wait for reset
}

std::vector<sc_lv<34> > Core::packet2flits(Packet* p) {
    std::vector<sc_lv<34> > v;
    // Construct header flit
    sc_lv<34> h = 0;
    h.range(33,32) = "10";                    // header type
    h.range(31,16) = sc_lv<4>(p->source_id);    // source id
    h.range(15,0) = sc_lv<4>(p->dest_id);      //destination id
    v.push_back(h);

    // Construct body/tail flits
    for (size_t i = 0; i < p->datas.size(); ++i) {
        float f = p->datas[i];
        unsigned u;
        std::memcpy(&u, &f, sizeof(float));     // convert float to bit pattern
        sc_lv<34> b = 0;
        b.range(33,32) = (i+1 == p->datas.size()) ? "01" : "00";  // tail or body
        b.range(31,0) = sc_lv<32>(u);       // data bits
        v.push_back(b);
    }
    return v;
}

void Core::tx_proc() {
    packet_requested = false;
    req_tx.write(false);
    flit_tx.write(0);
    wait();  // wait for reset

    // Stagger injection across cores
    for (int d = 0; d < core_id; ++d)
        wait();

    while (true) {
        wait();  // wait for next clock

        if (tx_buf.empty() && !packet_requested) {
            Packet* p = pe.get_packet();        // fetch next packet from PE
            if (p) {
                tx_buf = packet2flits(p);
                tx_idx = 0;
                packet_requested = true;
                delete p;
            }
        }

        if (!tx_buf.empty()) {
            flit_tx.write(tx_buf[tx_idx]);
            req_tx.write(true);
            // flit successfully sent
            if (ack_tx.read()) {
                tx_idx++;
                if (tx_idx >= tx_buf.size()) {
                    tx_buf.clear();
                    packet_requested = false;
                }
            }
        } else {
            req_tx.write(false);
        }
    }
}


void Core::rx_proc() {
    ack_rx.write(false);    
    while (true) {
        wait();     // wait for next clock
        if (req_rx.read()) {
            sc_lv<34> flit = flit_rx.read();        // read incoming flit
            ack_rx.write(true);
            sc_lv<2> type = flit.range(33,32);

            // header flit
            if (type == "10") {
                rx_buf.clear();     // start of a new packet
            }
            rx_buf.push_back(flit);
            // tail flit (packet complete)
            if (type == "01") { 
                Packet* p = flits2packet(rx_buf);
                pe.check_packet(p);     // verify correctness
                delete p;
                rx_buf.clear();
            }
        } else {
            ack_rx.write(false);
        }
    }
}


Packet* Core::flits2packet(const std::vector<sc_lv<34> >& v) {
    Packet* p = new Packet;
    sc_lv<34> h = v[0];
    p->source_id = h.range(31,16).to_uint();
    p->dest_id   = h.range(15,0).to_uint();
    for (size_t i = 1; i < v.size(); ++i) {
        sc_lv<32> d = v[i].range(31,0);
        unsigned u = d.to_uint();
        float f;
        std::memcpy(&f, &u, sizeof(float));  // convert bits to float
        p->datas.push_back(f);
    }
    return p;
}


#endif
