#include "alexnet.h"

int sc_main(int argc, char* argv[]) {
    int num = 1000;
    // �ŧi port
    sc_signal<float> sig_prob[num];
    sc_signal<float> sig_value[num];

    // �إ� AlexNet �Ҳ�
    AlexNet alexnet("AlexNet");

    // �T�O�u�����Ѥ@���ɮ�
    if(argc != 2){
        cerr << "Usage: " << argv[0] << " <file>" << endl;
        return 1;
    }

    // �]�w��J�ɮצW��
    string file = argv[1];
    alexnet.file_name = "./data/" + file;

    // �إ� Monitor �ҲեH���� AlexNet ��X
    Monitor mon("Monitor");

    // �Q�� port �s�� AlexNet �P Monitor �Ҳ�
    for (int i = 0; i < num; i++) {
        alexnet.softmax_output_result[i](sig_prob[i]);
        alexnet.fc_output_result[i](sig_value[i]);
        mon.in_prob[i](sig_prob[i]);
        mon.in_value[i](sig_value[i]);
    }

    // �Ұʼ���
    sc_start();
    return 0;
}
