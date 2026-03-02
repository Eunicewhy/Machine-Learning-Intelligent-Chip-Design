// ROM.cpp
#include "ROM.h"

// load file data into memory
void ROM::load_file(const string& filename, uint32_t base_addr) {
    ifstream file(filename.c_str());
    if (!file.is_open()) {
        cout << "Warning: Could not open file " << filename << endl;
        return;
    }
    
    float value;
    uint32_t addr = base_addr;
    while (file >> value) {
        memory[addr] = value;
        addr += 4; // 4 bytes per float
    }
    file.close();
    cout << "ROM: Loaded " << filename << " at address 0x" << hex << base_addr << dec << endl;
};

void ROM::load_data_files() {
    // Memory mapping addresses
    const uint32_t INPUT_BASE_ADDR = 0x00000000;
    const uint32_t CONV1_WEIGHT_ADDR = 0x01000000;
    const uint32_t CONV1_BIAS_ADDR = 0x02000000;
    const uint32_t CONV2_WEIGHT_ADDR = 0x03000000;
    const uint32_t CONV2_BIAS_ADDR = 0x04000000;
    const uint32_t CONV3_WEIGHT_ADDR = 0x05000000;
    const uint32_t CONV3_BIAS_ADDR = 0x06000000;
    const uint32_t CONV4_WEIGHT_ADDR = 0x07000000;
    const uint32_t CONV4_BIAS_ADDR = 0x08000000;
    const uint32_t CONV5_WEIGHT_ADDR = 0x09000000;
    const uint32_t CONV5_BIAS_ADDR = 0x0A000000;
    const uint32_t FC6_WEIGHT_ADDR = 0x0B000000;
    const uint32_t FC6_BIAS_ADDR = 0x1B000000;
    const uint32_t FC7_WEIGHT_ADDR = 0x2B000000;
    const uint32_t FC7_BIAS_ADDR = 0x3B000000;
    const uint32_t FC8_WEIGHT_ADDR = 0x4B000000;
    const uint32_t FC8_BIAS_ADDR = 0x5B000000;

    // Load input data
    load_file(DATA_PATH + IMAGE_FILE_NAME, INPUT_BASE_ADDR);
    
    // Load conv layer weights and biases
    for (int i = 1; i <= 5; i++) {
        stringstream ss;
        ss << i;
        uint32_t weight_addr = CONV1_WEIGHT_ADDR + (i-1) * 0x2000000;
        uint32_t bias_addr = CONV1_BIAS_ADDR + (i-1) * 0x2000000;
        
        load_file(DATA_PATH + "conv" + ss.str() + "_weight.txt", weight_addr);
        load_file(DATA_PATH + "conv" + ss.str() + "_bias.txt", bias_addr);
    }
    
    // Load FC layer weights and biases
    for (int i = 6; i <= 8; i++) {
        stringstream ss;
        ss << i;
        uint32_t weight_addr = FC6_WEIGHT_ADDR + (i-6) * 0x20000000;
        uint32_t bias_addr = FC6_BIAS_ADDR + (i-6) * 0x20000000;
        
        load_file(DATA_PATH + "fc" + ss.str() + "_weight.txt", weight_addr);
        load_file(DATA_PATH + "fc" + ss.str() + "_bias.txt", bias_addr);
    }
    cout << "ROM memory size: " << memory.size() << endl;
}

uint32_t ROM::read_memory(uint32_t addr) {
    std::map<uint32_t, float>::iterator it = memory.find(addr);
    if (it != memory.end()) {
        uint32_t result;
        memcpy(&result, &(it->second), sizeof(float));
        return result;
    } else {
        // Return 0 if address not found
        return 0;
    }
}

void ROM::run() {
    while (true) {
        uint32_t burst_type;
        wait();
        if (rst.read()) {
            transaction_active = false;
            ARREADY.write(false);
            RVALID.write(false);
            RLAST.write(false);
            continue;
        }
        // Handle read address channel
        if (ARVALID.read() && !transaction_active) {
            // Accept read address
            current_rid = ARID.read();
            current_addr = ARADDR.read().to_uint();
            remaining_beats = ARLEN.read().to_uint() + 1; // ARLEN is actual length - 1
            burst_type = ARBURST.read().to_uint();
            transaction_active = true;
            
            ARREADY.write(true);
            wait();
            ARREADY.write(false);
        }
        
        // Handle read data channel
        if (transaction_active && RREADY.read()) {
            // Send data
            RID.write(current_rid);
            RDATA.write(read_memory(current_addr));
            RRESP.write(0); // OKAY
            RVALID.write(true);
            
            remaining_beats--;
            if (remaining_beats == 0) {
                RLAST.write(true);
                transaction_active = false;
            } else {
                RLAST.write(false);
                if (burst_type == 1) {
                    current_addr += 4; // INCR burst
                } else if (burst_type == 0) {
                    // FIXED burst：do nothing
                } else {
                    // unsupported burst type
                    cout << "Unsupported burst type: " << burst_type << endl;
                }
            }
            
            wait();
            RVALID.write(false);
            RLAST.write(false);
        } else {
            wait();
        }
    }
}