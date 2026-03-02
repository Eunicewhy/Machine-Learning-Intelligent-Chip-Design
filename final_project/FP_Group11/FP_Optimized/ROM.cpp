// ROM.cpp
#include "ROM.h"

void ROM::run() {
    while (true) {
        wait();
        if (rst.read()) {
            transaction_active = false;
            ARREADY.write(false);
            RVALID.write(false);
            RLAST.write(false);
            continue;
        }

        // on new layer_id_valid: load corresponding file
        if (layer_id_valid.read()) {
            // Read signals
            int id = layer_id.read();
            bool type = layer_id_type.read();
            string filename;
            if (id == 0)
            {
                filename = string(DATA_PATH) + IMAGE_FILE_NAME;
            }
            else if (id <= 5)
            {
                std::stringstream ss;
                ss << id;
                if (type == 0)
                    filename = string(DATA_PATH) + "conv" + ss.str() + "_weight.txt";
                else
                    filename = string(DATA_PATH) + "conv" + ss.str() + "_bias.txt";
            }
            else if (id <= 8)
            {
                std::stringstream ss;
                ss << id;
                if (type == 0)
                    filename = string(DATA_PATH) + "fc" + ss.str() + "_weight.txt";
                else
                    filename = string(DATA_PATH) + "fc" + ss.str() + "_bias.txt";
            }
            else
            {
                cout << "Error: Invalid layer id " << id << "." << endl;
                sc_stop();
            }
            // load into file_buf
            file_buf.clear();
            ifstream f(filename.c_str());
            if (!f) {
                cout << "ROM: failed to open " << filename << endl;
                sc_stop();
            }
            float v;
            while (f >> v) file_buf.push_back(v);
            f.close();
            cout << "ROM: Loaded " << filename << " at address 0x" << 0 << endl;
            cout << "ROM memory size: " << file_buf.size() << endl;

            // cout << "ROM: loaded layer " << id
            //      << (type ? " bias" : " weight") 
            //      << " (" << file_buf.size() << " floats)" << endl;
        }

        // handle AXI read address
        if (!transaction_active && ARVALID.read()) {
            ARREADY.write(true);
            wait();
            ARREADY.write(false);

            current_rid    = ARID.read();
            current_burst= ARBURST.read();
            unsigned int addr = ARADDR.read().to_uint();
            // address is byte offset, index = addr / 4
            beat_index     = addr / 4;
            unsigned int len = ARLEN.read().to_uint() + 1;
            total_beats    = len;
            transaction_active = true;
        }

        // handle AXI read data
        if (transaction_active && RREADY.read()) {
            // drive outputs
            RID.write(current_rid);
            float val = 0.0f;
            if (beat_index < file_buf.size())
                val = file_buf[beat_index];
            uint32_t bits;
            memcpy(&bits, &val, sizeof(float));
            RDATA.write( sc_lv<32>(bits) );
            RRESP.write( sc_lv<2>("00") ); // OKAY
            RVALID.write(true);

            // last beat?
            if (total_beats == 1) {
                RLAST.write(true);
                transaction_active = false;
            } else {
                RLAST.write(false);
            }

            wait();  // hold one cycle
            // update beat index and total beats
            if (current_burst == "01") {
                // next beat
                beat_index++;
            }
            total_beats--;
            RVALID.write(false);
            RLAST.write(false);
        }
    }
}