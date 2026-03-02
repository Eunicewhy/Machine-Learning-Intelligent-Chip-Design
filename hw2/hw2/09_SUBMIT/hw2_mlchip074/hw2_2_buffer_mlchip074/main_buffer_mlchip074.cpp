#include "alexnet.h"

int sc_main(int argc, char* argv[]) {
    // 宣告 port
    sc_buffer<float> sig_prob[1000];
    sc_buffer<float> sig_value[1000];
    sc_buffer<int> sig_num;

    // 建立 AlexNet 模組
    AlexNet alexnet("AlexNet");

    // 確保只有提供一個檔案
    if(argc != 2){
        cerr << "Usage: " << argv[0] << " <file>" << endl;
        return 1;
    }

    // 設定輸入檔案名稱
    string file = argv[1];
    alexnet.file_name = "../data/" + file;

    // 建立 Monitor 模組以接收 AlexNet 輸出
    Monitor mon("Monitor");

    // 利用 port 連接 AlexNet 與 Monitor 模組
    alexnet.num_out(sig_num);
    mon.num_in(sig_num);
    for (int i = 0; i < 1000; i++) {
        alexnet.softmax_output_result[i](sig_prob[i]);
        alexnet.fc_output_result[i](sig_value[i]);
        mon.in_prob[i](sig_prob[i]);
        mon.in_value[i](sig_value[i]);
    }

    // 啟動模擬
    sc_start();
    return 0;
}
