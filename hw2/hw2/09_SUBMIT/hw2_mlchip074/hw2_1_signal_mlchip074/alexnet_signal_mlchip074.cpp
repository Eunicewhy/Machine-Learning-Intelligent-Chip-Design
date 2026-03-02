#include "alexnet.h"

// Zero Padding
Vector3D zero_pad(const Vector3D& input, int pad_top, int pad_left, int pad_bottom, int pad_right) {
    int channels = input.size();
    int height = input[0].size();
    int width = input[0][0].size();
    int new_height = height + pad_top + pad_bottom;
    int new_width = width + pad_left + pad_right;
    Vector3D output(channels, vector<vector<float> >(new_height, vector<float>(new_width, 0.0f)));
    for (int c = 0; c < channels; c++){
        for (int i = 0; i < height; i++){
            for (int j = 0; j < width; j++){
                output[c][i + pad_top][j + pad_left] = input[c][i][j];
            }
        }
    }
    return output;
}

// Input Layer
Vector3D Input(int height, int width, int channels, string file_name){
    Vector3D image(channels, vector<vector<float> >(height, vector<float>(width, 0.0f)));
    ifstream fin(file_name.c_str());
    if (!fin) {
         cout << "Error opening file: " << file_name << endl;
         sc_stop();
    }
    // ???頦???剖??鞈ｇ蕭嚙賢??嚙? raster scan 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚙賢??鞈ｇ蕭嚙踝蕭嚙賢?刻?剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭嚙? image data
    for (int c = 0; c < channels; c++){
         for (int i = 0; i < height; i++){
              for (int j = 0; j < width; j++){
                  fin >> image[c][i][j];
              }
         }
    }
    fin.close();
    // Zero Padding???頦???剖??鞈ｇ蕭嚙賡????渲?剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙質????哨蕭嚙踝蕭??佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭???嚙? 2 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚙? 0 ???頦???剖??鞈ｇ蕭嚙賡????渲?剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭??刻?剖??鞈ｇ蕭嚙踝蕭嚙賢??嚙踝蕭?????渲?剖??鞈ｇ蕭嚙賢??嚙? 1 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚙? 0
    Vector3D padded_image = zero_pad(image, 2, 2, 1, 1);
    return padded_image;
}

// Convolutional Layer + ReLU
Vector3D Convolution(int num_filters, int filter_size, int stride_conv, int channels, int padding_conv, Vector3D padded_image, string conv_weight_file, string conv_bias_file){
    vector<Vector3D> conv_filters(num_filters, Vector3D(channels, vector<vector<float> >(filter_size, vector<float>(filter_size, 0.0f))));
    Vector1D conv_biases(num_filters, 0.0f);
    ifstream wfin(conv_weight_file.c_str());
    if (!wfin) {
         cout << "Error opening weight file: " << conv_weight_file << endl;
         sc_stop();
    }

    // ???頦???剔?曄??嚙踝蕭 filter ???頦???剖??鞈ｇ蕭嚙賡????渲?剔?曄??嚙踝蕭 (kernels * channels * filter_size * filter_size)
    for (int f = 0; f < num_filters; f++){
         for (int c = 0; c < channels; c++){
              for (int i = 0; i < filter_size; i++){
                   for (int j = 0; j < filter_size; j++){
                        wfin >> conv_filters[f][c][i][j];
                   }
              }
         }
    }
    wfin.close();
    ifstream bfin(conv_bias_file.c_str());
    if (!bfin) {
         cout << "Error opening bias file: " << conv_bias_file << endl;
         sc_stop();
    }
    for (int f = 0; f < num_filters; f++){
         bfin >> conv_biases[f];
    }
    bfin.close();
    
    int input_height = padded_image[0].size();
    int input_width = padded_image[0][0].size();
    // ???頦???剔?曄??嚙賢?刻?剖??頦???哨蕭嚙踝蕭???嚙? Convolution output size
    int output_height = (input_height - filter_size + 2 * padding_conv) / stride_conv + 1;
    int output_width  = (input_width - filter_size + 2 * padding_conv) / stride_conv + 1;

    Vector3D conv_output(num_filters, vector<vector<float> >(output_height, vector<float>(output_width, 0.0f)));
    // ???頦???剖??鞈ｇ蕭嚙賡??嚙賢??鞈ｇ蕭嚙踝蕭嚙賢?刻?剖??頦???哨蕭嚙踝蕭???嚙賢??鞈哨蕭嚙踝蕭嚙賢??嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙? filter 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???餈???剖??鞈ｇ蕭嚙賡??嚚???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭??刻?剖??鞈芣?抬蕭嚙踝蕭嚙踝蕭嚙賡?佗蕭嚙踝蕭嚙? ReLU???頦???剖??鞈ｇ蕭嚙賢??嚙?
    for (int f = 0; f < num_filters; f++){
         for (int i = 0; i < output_height; i++){
              for (int j = 0; j < output_width; j++){
                   float sum = 0.0f;
                   for (int c = 0; c < channels; c++){
                        for (int ki = 0; ki < filter_size; ki++){
                             for (int kj = 0; kj < filter_size; kj++){
                                  int in_i = i * stride_conv + ki - padding_conv;
                                  int in_j = j * stride_conv + kj - padding_conv;
                                  float in_val = 0.0f;
                                  if (in_i >= 0 && in_i < input_height && in_j >= 0 && in_j < input_width) {
                                      in_val = padded_image[c][in_i][in_j];
                                  }
                                  sum += in_val * conv_filters[f][c][ki][kj];
                             }
                        }
                   }
                   // ???頦???剔?曄??嚙賢?刻?剖??頦???哨蕭嚙踝蕭???嚙? bias
                   sum += conv_biases[f];
                   // ReLU
                   conv_output[f][i][j] = (sum > 0) ? sum : 0;
              }
         }
    }
    return conv_output;
}

// Max Pooling
Vector3D Max_Pooling(int num_filters, int pool_size, int stride_pool, Vector3D conv_output){
    int pool_input_height = conv_output[0].size();
    int pool_input_width  = conv_output[0][0].size();
    int pool_output_height = (pool_input_height - pool_size) / stride_pool + 1;
    int pool_output_width  = (pool_input_width - pool_size) / stride_pool + 1;
    Vector3D pool_output(num_filters, vector<vector<float> >(pool_output_height, vector<float>(pool_output_width, 0.0f)));

    // ???頦???剖??鞈ｇ蕭嚙賡??嚙賢??鞈ｇ蕭嚙踝蕭嚙賢?刻?剖??頦???哨蕭嚙踝蕭???嚙賢??鞈哨蕭嚙踝蕭嚙賢??嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙? pool 嚙踝蕭嚙賡?佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭???嚙踝蕭嚙踝蕭嚙踝蕭嚙賡?佗蕭嚙踝蕭??刻?剖??鞈ｇ蕭嚙賢??鞈ｇ蕭嚙踝蕭嚙賢??嚙踝蕭???嚙?
    for (int f = 0; f < num_filters; f++){
         for (int i = 0; i < pool_output_height; i++){
              for (int j = 0; j < pool_output_width; j++){
                   float max_val = -1e9;
                   for (int m = 0; m < pool_size; m++){
                        for (int n = 0; n < pool_size; n++){
                             int in_i = i * stride_pool + m;
                             int in_j = j * stride_pool + n;
                             if (in_i < pool_input_height && in_j < pool_input_width)
                                 max_val = max(max_val, conv_output[f][in_i][in_j]);
                        }
                   }
                   pool_output[f][i][j] = max_val;
              }
         }
    }
    return pool_output;
}

// Flatten Layer
Vector1D Flatten(int num_filters, int pool_output_height, int pool_output_width, Vector3D pool_output){
    Vector1D fc_input;
    // ???頦???剖??鞈ｇ蕭嚙賢??嚙? feature map ???頦???剛?祈釭嚙賢?刻?剖??頦???哨蕭嚙踝蕭???餈???哨蕭嚙踝蕭???頦???哨蕭嚙踝蕭???嚙賢??頦???剖??頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭?????渲?剖??鞈ｇ蕭嚙賡??餈???剛??頛荔蕭嚙?
    for (int f = 0; f < num_filters; f++){
         for (int i = 0; i < pool_output_height; i++){
              for (int j = 0; j < pool_output_width; j++){
                   fc_input.push_back(pool_output[f][i][j]);
              }
         }
    }
    return fc_input;
}

// Fully Connected Layer
Vector1D Fully_Connect(int input_size, int num_neurons, string fc_weight_file, string fc_bias_file, Vector1D fc_input){
     // ???頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭嚙? weight ???頦???剔??嚙踝蕭嚙賢?刻?剖??頦?嚙踝蕭嚙踝蕭嚙?
    ifstream fc_wfin(fc_weight_file.c_str());
    if (!fc_wfin) {
         cout << "Error opening weight file: " << fc_weight_file << endl;
         sc_stop();
    }
    vector<Vector1D> fc_weights(num_neurons, Vector1D(input_size, 0.0f));
    for (int n = 0; n < num_neurons; n++){
          for (int i = 0; i < input_size; i++){
               fc_wfin >> fc_weights[n][i];
          }
    }
    fc_wfin.close();
    // ???頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭嚙? bias ???頦???剔??嚙踝蕭嚙賢?刻?剖??頦?嚙踝蕭嚙踝蕭嚙?
    ifstream fc_bfin(fc_bias_file.c_str());
    if (!fc_bfin) {
         cout << "Error opening bias file: " << fc_bias_file << endl;
         sc_stop();
    }
    Vector1D fc_biases(num_neurons, 0.0f);
    for (int n = 0; n < num_neurons; n++){
         fc_bfin >> fc_biases[n];
    }
    fc_bfin.close();

    // ???頦???剔?曄??嚙賢?刻?剖??頦???哨蕭嚙踝蕭???嚙? weight 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚙? bias
    Vector1D fc_output(num_neurons, 0.0f);
    for (int n = 0; n < num_neurons; n++) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; i++) {
            sum += fc_input[i] * fc_weights[n][i];
        }
        sum += fc_biases[n];
        if(fc_weight_file == "../data/fc8_weight.txt") fc_output[n] = sum;  // 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙賢??嚙踝蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭???嚚???剝?佗蕭嚙踝蕭???嚙踝蕭???嚙? fc8 ???頦???剖??鞈ｇ蕭嚙賢??嚙? ???頦???剖??鞈ｇ蕭嚙賡????渲?剝?????嚙踝蕭嚙踝蕭嚙賡?佗蕭嚙踝蕭嚙? ReLU
        else fc_output[n] = (sum > 0) ? sum : 0;  // ReLU
    }
    return fc_output;
}

// Soft MAx
Vector1D Soft_Max(Vector1D fc_output){
    Vector1D softmax_output(fc_output.size(), 0.0f);
    float sum_exp = 0.0f;
    for (int i = 0; i < fc_output.size(); i++){
         softmax_output[i] = exp(fc_output[i]);
         sum_exp += softmax_output[i];
    }
    for (int i = 0; i < softmax_output.size(); i++){
         softmax_output[i] /= sum_exp;
    }
    return softmax_output;
}

// ???頦???剝?∴蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙賢??鞈迎蕭嚙踝蕭嚙賢??嚙踝蕭???嚙賢??鞈ｇ蕭嚙賢??鞈ｇ蕭嚙踝蕭嚙賢??嚙踝蕭???嚙?
bool comparePairs(const pair<float, int>& a, const pair<float, int>& b) {
     return a.first > b.first;
}

// AlexNet ???頦???剖??頦???剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙? run()???頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚙賢??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭嚙? AlexNet 嚙踝蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭???嚙?
void AlexNet::run() {
    // Input Layer
    Vector3D padded_image = Input(224, 224, 3, file_name);

    // Convolutional Layer 1 + ReLU
    Vector3D conv_output1 = Convolution(64, 11, 4, padded_image.size(), 0, padded_image, "../data/conv1_weight.txt", "../data/conv1_bias.txt");
    // Max Pooling 1
    Vector3D pool_output1 = Max_Pooling(64, 3, 2, conv_output1);
    // Convolutional Layer 2 + ReLU
    Vector3D conv_output2 = Convolution(192, 5, 1, pool_output1.size(), 2, pool_output1, "../data/conv2_weight.txt", "../data/conv2_bias.txt");
    // Max Pooling 2
    Vector3D pool_output2 = Max_Pooling(192, 3, 2, conv_output2);
    // Convolutional Layer 3 + ReLU
    Vector3D conv_output3 = Convolution(384, 3, 1, pool_output2.size(), 1, pool_output2, "../data/conv3_weight.txt", "../data/conv3_bias.txt");
    // Convolutional Layer 4 + ReLU
    Vector3D conv_output4 = Convolution(256, 3, 1, conv_output3.size(), 1, conv_output3, "../data/conv4_weight.txt", "../data/conv4_bias.txt");
    // Convolutional Layer 5 + ReLU
    Vector3D conv_output5 = Convolution(256, 3, 1, conv_output4.size(), 1, conv_output4, "../data/conv5_weight.txt", "../data/conv5_bias.txt");
    // Max Pooling 5
    Vector3D pool_output5 = Max_Pooling(256, 3, 2, conv_output5);

    // Flatten Layer
    Vector1D fc_input = Flatten(256, 6, 6, pool_output5);
    // Fully Connected Layer 6 + ReLU
    Vector1D fc_output6 = Fully_Connect(9216, 4096, "../data/fc6_weight.txt", "../data/fc6_bias.txt", fc_input);
    // Fully Connected Layer 7 + ReLU
    Vector1D fc_output7 = Fully_Connect(4096, 4096, "../data/fc7_weight.txt", "../data/fc7_bias.txt", fc_output6);
    // Fully Connected Layer 8
    Vector1D fc_output8 = Fully_Connect(4096, 1000, "../data/fc8_weight.txt", "../data/fc8_bias.txt", fc_output7);

    // Softmax Layer
    Vector1D softmax_output = Soft_Max(fc_output8);

    int num = softmax_output.size();
    num_out->write(num);

     // ???頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭??刻?剖??鞈迎蕭嚙踝蕭嚙賢?刻?剖??鞈迎蕭嚙踝蕭嚙賢??嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭?????渲?剖??鞈∴蕭???嚙踝蕭嚙踝蕭???嚙踝蕭???鞈ｇ蕭嚙賡?佗蕭嚙踝蕭嚙? output port
     for(int i = 0; i < num; i++){
          softmax_output_result[i]->write(softmax_output[i]);
          fc_output_result[i]->write(fc_output8[i]);
     }
    
}

// Monitor ???頦???剖??頦???剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙? monitor_output()???頦???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭???嚚???剖??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭嚙? Top 100 ???頦???哨蕭嚙踝蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙?
void Monitor::monitor_output() {
    // ???頦???剖??鞈ｇ蕭嚙賡??嚙賢??鞈?嚙踝蕭嚙踝蕭嚙?
    string imagenet_file = "../data/imagenet_classes.txt";
    ifstream file_image(imagenet_file.c_str());
    if (!file_image) {
         cout << "Error opening imagenet classes file: " << imagenet_file << endl;
         sc_stop();
    }

//     wait(SC_ZERO_TIME);

    int num = num_in->read();
    Vector1D value(num, 0.0f);
    vector<pair<float, int> > prob_idx;
    for (int i = 0; i < num; i++){
         prob_idx.push_back(make_pair(in_prob[i]->read(), i));
         value.push_back(in_value[i]->read());
    }

    // ???頦???剖??鞈ｇ蕭嚙賡??嚙賢??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙賢??鞈ｇ蕭嚙踝蕭嚙賢?刻?剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚚???剖??鞈∴蕭嚙賢??鞈ｇ蕭嚙賡?佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭??佗蕭嚙踝蕭??刻?剖??頦???哨蕭嚙踝蕭???嚙?
    sort(prob_idx.begin(), prob_idx.end(), comparePairs);
    sort(value.begin(), value.end(), greater<float>());

    // ???頦???剖??鞈∴蕭???嚙踝蕭嚙踝蕭???嚙踝蕭???鞈ｇ蕭嚙賡?佗蕭嚙踝蕭???嚙踝蕭??佗蕭嚙踝蕭???嚙踝蕭???嚙?
    cout << fixed << setprecision(2);
    cout << "Top 100 classes:" << endl;
    cout << "=================================================" << endl;
    cout << right << setw(5) << "idx"
         << " | " << setw(8) << "val"
         << " | " << setw(11) << "possibility"
         << " | " << "class name" << endl;
    cout << "-------------------------------------------------" << endl;
    for (int i = 0; i < 100 && i < prob_idx.size(); i++){
         file_image.clear();
         file_image.seekg(0, ios::beg);
         int index = prob_idx[i].second;
         string line;
         for (int j = 0; j <= index; j++){
             getline(file_image, line);
         }
         
         cout << right << setw(5) << index
              << " | " << setw(8) << value[i]
              << " | " << setw(11) << prob_idx[i].first * 100
              << " | " << line << endl;
    }
    cout << "=================================================" << endl;
}