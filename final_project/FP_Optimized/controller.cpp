// controller.cpp
#include "controller.h"

#define get_cycle() int(sc_time_stamp().to_seconds() * 1e9 / 10)
void print_pass();

bool comparePairs(const std::pair<float, int>& a, const std::pair<float, int>& b) {
    return a.first > b.first;
}

// Softmax
vector<float> softmax(const vector<float>& input) {
    vector<float> output(input.size());
    float sum = 0;
    for (int i = 0; i < input.size(); ++i) {
        output[i] = exp(input[i]);
        sum += output[i];
    }
    for (int i = 0; i < output.size(); ++i) {
        output[i] /= sum;
    }
    return output;
}

void Controller::packet2flit(vector<float> data, int dst, int data_type){
    // packet to flits and give them to NoC (PE0, dest_id = 0)
    int src = 15;
    for (int i = 0; i < data.size() + 2; ++i) {
        sc_lv<34> flit;
        string type;
        if (i == 0) type = "10";  // header
        else if (i == data.size() + 1) type = "01";  // tail
        else type = "00";  // body

        flit.range(33, 32) = type.c_str();
        if (type == "10" || type == "01") {
            flit.range(31, 28) = sc_lv<4>(src);
            flit.range(27, 24) = sc_lv<4>(dst);
            flit.range(23, 22) = sc_lv<2>(data_type);  // use only 2 bits
            flit.range(21, 0)  = 0; // optional: padding
        }
        else {
            uint32_t bits;
            // if(i == data.size())
            //     cout << "weight end: " << data[i - 1] << endl;
            std::memcpy(&bits, &data[i - 1], sizeof(float));
            flit.range(31, 0) = sc_lv<32>(bits);  // full float body
        }

        req_tx.write(true);
        flit_tx.write(flit);
        do { wait(); } while (!ack_tx.read());
        req_tx.write(false);
    }
}

void Controller::axi_read_transaction(sc_lv<32> addr, int burst_len) {
    // AXI4 Read Address Phase
    ARID.write(sc_lv<4>(0));
    ARADDR.write(addr);
    ARLEN.write(sc_lv<8>(burst_len-1));
    ARSIZE.write(sc_lv<3>(2));
    ARBURST.write(sc_lv<2>("01"));
    ARVALID.write(true);

    // Wait for address handshake
    ARVALID.write(true);
    do { wait(); } while (!ARREADY.read());
    ARVALID.write(false);
    
    // Prepare for data phase
    RREADY.write(true);
}

void Controller::fetch() {
    vector<float> input_data;  // collect all input / weights / biases
    int data_type;
    
    while(true){
        wait();

        if (rst.read()) {
            req_tx.write(false);
            ARVALID.write(false);
            RREADY.write(false);
            curr_layer = -1;
            curr_type = 0;
            wait();  
            continue;
        }

        if (curr_type == 0 && curr_layer != -1 && curr_layer != 0) {
            data_type = BIAS;
            curr_type = 1;
        }
        else { 
            data_type = WEIGHT;
            curr_type = 0; 
            curr_layer++; 
        }
        if(curr_layer == 0) data_type = INPUT;
        
        if(curr_layer < 9){
            layer_id.write(curr_layer);
            layer_id_type.write(curr_type);
            layer_id_valid.write(true);
            wait();
            layer_id_valid.write(false);
            // Determine memory address based on layer and type
            int expected_data_count = 0;
            
            if(curr_layer == 0) {
                expected_data_count = 150528; // 3*224*224
            }
            else if(curr_layer == 1) {
                if(data_type == WEIGHT) {
                    expected_data_count = 23232; // 64*3*11*11
                } else {
                    expected_data_count = 64;
                }
            }
            else if(curr_layer == 2) {
                if(data_type == WEIGHT) {
                    expected_data_count = 307200; // 192*64*5*5
                } else {
                    expected_data_count = 192;
                }
            }
            else if(curr_layer == 3) {
                if(data_type == WEIGHT) {
                    expected_data_count = 663552; // 384*192*3*3
                } else {
                    expected_data_count = 384;
                }
            }
            else if(curr_layer == 4) {
                if(data_type == WEIGHT) {
                    expected_data_count = 884736; // 256*384*3*3
                } else {
                    expected_data_count = 256;
                }
            }
            else if(curr_layer == 5) {
                if(data_type == WEIGHT) {
                    expected_data_count = 589824; // 256*256*3*3
                } else {
                    expected_data_count = 256;
                }
            }
            else if(curr_layer == 6) {
                if(data_type == WEIGHT) {
                    expected_data_count = 37748736; // 4096*9216
                } else {
                    expected_data_count = 4096;
                }
            }
            else if(curr_layer == 7) {
                if(data_type == WEIGHT) {
                    expected_data_count = 16777216; // 4096*4096
                } else {
                    expected_data_count = 4096;
                }
            }
            else if(curr_layer == 8) {
                if(data_type == WEIGHT) {
                    expected_data_count = 4096000; // 1000*4096
                } else {
                    expected_data_count = 1000;
                }
            }

           // Calculate the base address for AXI read
            int bytes_per_word = 4;
            int max_axi_burst = 256;

            uint32_t base_addr = 0;
            int remaining = expected_data_count;
            // cout << "remaining: " << remaining << endl;

            while (remaining > 0) {
                int burst_len = std::min(remaining, max_axi_burst);

                // Send AXI read address transaction
                axi_read_transaction(sc_lv<32>(base_addr), burst_len);

                int received = 0;
                while (true) {
                    if (RVALID.read() && RREADY.read()) {
                        // data start to receive
                        uint32_t data_bits = RDATA.read().to_uint();
                        float value;
                        std::memcpy(&value, &data_bits, sizeof(float));
                        input_data.push_back(value);
                        received++;
                        // last data
                        if (RLAST.read()) {
                            // cout << "last value: " << value << endl;
                            break;
                        }
                        if (RRESP.read() != "00") {
                            cout<<"AXI read error, resp="<<RRESP.read().to_uint()<<endl;
                        }
                    }
                    wait(); // next clock
                
                }

                // Ensure that this burst actually received burst_len beats
                if (received != burst_len) {
                    cout << "Warning: expected " << burst_len
                        << " beats, but got " << received << endl;
                }

                remaining -= burst_len;
                base_addr += burst_len * bytes_per_word;
            }

            RREADY.write(false);
            // cout << "Data fetch complete: layer " << curr_layer
            //     << ", type " << (data_type == WEIGHT ? "WEIGHT" : (data_type == BIAS ? "BIAS" : "INPUT"))
            //     << ", count = " << input_data.size() << endl;

            // packet to flits and give them to NoC (PE0, dest_id = 0)
            packet2flit(input_data, curr_layer, data_type);
            input_data.clear();
        }
    }
}

void Controller::receive(){
    // Wait for result (from PE9)
    std::vector<float> result;
    bool set;
    while (true) {
        wait();
        if (rst.read()) {
            ack_rx.write(false);
            set = false;
            continue;
        }
        if (req_rx.read()) {
            sc_lv<34> flit = flit_rx.read();
            sc_lv<2> type = flit.range(33, 32);
            if (type == "00") {
                // body flit → data
                uint32_t bits = flit.range(31, 0).to_uint();
                float val;
                std::memcpy(&val, &bits, sizeof(float));
                // append to correct vector (based on current receiving context)
                result.push_back(val);
            }
            if (type == "01") {
                // cout << "Controller: Read..." << endl;
                // open classes file
                ifstream file("data/imagenet_classes.txt");
                vector<string> img_class;
                string line;
                while (getline(file, line)) {
                    img_class.push_back(line);
                }
                file.close();

                vector<float> possibility = softmax(result);
                std::vector<std::pair<float, int> > scores;
                for (int i = 0; i < result.size(); ++i)
                    scores.push_back(std::make_pair(result[i], i));
                
                std::sort(scores.begin(), scores.end(), comparePairs);
                std::sort(possibility.begin(), possibility.end(), greater<float>());

                cout << "Top 100 classes:" << endl;
                cout << "=================================================" << endl;
                cout << right << setw(5) << "idx"
                    << " | " << setw(8) << "val"
                    << " | " << setw(11) << "possibility"
                    << " | " << "class name" << endl;
                cout << "-------------------------------------------------" << endl;

                for (int i = 0; i < 100 && i < scores.size(); ++i) {
                    int idx = scores[i].second;
                    cout << right << setw(5) << idx
                        << " | " << setw(8) << fixed << setprecision(2) << scores[i].first
                        << " | " << setw(11) << fixed << setprecision(2) << possibility[i] * 100
                        << " | " << img_class[idx] << endl;
                }

                cout << "=================================================" << endl;
                print_pass();
                sc_stop();
            }
            ack_rx.write(true);
        } else {
            ack_rx.write(false);
        }
    }
}

void print_pass(){
    string pass_str = "\033[0m\033[1;32mCongratulations !!\033[0m";
    
    
    ostringstream cycle_stream;
    cycle_stream << get_cycle();  
    string cycle_str = "at " + cycle_stream.str() + " th cycle"; 

    cout << endl;
    cout << "  ============================                  " << endl;
    cout << "  +                          +         |\\__|\\ " << endl;
    cout << "  +   " << pass_str << "     +        / O.O  |  " << endl;
    cout << "  +                          +      /_____   |  " << endl;
    cout << "  +   Simulation completed   +     /^ ^ ^ \\  | " << endl;
    cout << "  +   " << left << setw(20) << cycle_str << "   +    |^ ^ ^ ^ |w| " << endl;
    cout << "  +                          +     \\m___m__|_| " << endl;
    cout << "  ============================                  " << endl;
}