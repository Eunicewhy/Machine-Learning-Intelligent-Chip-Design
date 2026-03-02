#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>
using namespace std;

typedef vector<float> Vector1D;

typedef vector<float> Flattened3D;

Vector1D zero_pad_flat(const Vector1D& input, int c, int h, int w,
                       int pad_top, int pad_left, int pad_bottom, int pad_right) {
    int new_h = h + pad_top + pad_bottom;
    int new_w = w + pad_left + pad_right;
    Vector1D output(c * new_h * new_w, 0.0f);
    for (int ch = 0; ch < c; ++ch) {
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                output[ch * new_h * new_w + (i + pad_top) * new_w + (j + pad_left)] =
                    input[ch * h * w + i * w + j];
            }
        }
    }
    return output;
}

Vector1D conv_layer(const Vector1D& input, const Vector1D& weight, const Vector1D& bias,
                   int in_c, int in_h, int in_w, int f_size, int out_c, int stride, int pad) {
    int out_h = (in_h - f_size + 2 * pad) / stride + 1;
    int out_w = (in_w - f_size + 2 * pad) / stride + 1;
    Vector1D output(out_c * out_h * out_w, 0.0f);

    for (int oc = 0; oc < out_c; ++oc) {
        for (int oh = 0; oh < out_h; ++oh) {
            for (int ow = 0; ow < out_w; ++ow) {
                float sum = 0.0f;
                for (int ic = 0; ic < in_c; ++ic) {
                    for (int i = 0; i < f_size; ++i) {
                        for (int j = 0; j < f_size; ++j) {
                            int in_i = oh * stride + i - pad;
                            int in_j = ow * stride + j - pad;
                            if (in_i >= 0 && in_j >= 0 && in_i < in_h && in_j < in_w) {
                                int input_idx = ic * in_h * in_w + in_i * in_w + in_j;
                                int filter_idx = oc * in_c * f_size * f_size + ic * f_size * f_size + i * f_size + j;
                                sum += input[input_idx] * weight[filter_idx];
                            }
                        }
                    }
                }
                float val = sum + bias[oc];
                val = (val > 0) ? val : 0; // ReLU
                output[oc * out_h * out_w + oh * out_w + ow] = val;
            }
        }
    }
    return output;
}

Vector1D max_pool(const Vector1D& input, int c, int h, int w,
                  int pool_size, int stride) {
    int out_h = (h - pool_size) / stride + 1;
    int out_w = (w - pool_size) / stride + 1;
    Vector1D output(c * out_h * out_w);

    for (int ch = 0; ch < c; ++ch) {
        for (int i = 0; i < out_h; ++i) {
            for (int j = 0; j < out_w; ++j) {
                float max_val = -1e9f;
                for (int m = 0; m < pool_size; ++m) {
                    for (int n = 0; n < pool_size; ++n) {
                        int r = i * stride + m;
                        int cidx = j * stride + n;
                        int idx = ch * h * w + r * w + cidx;
                        if (r < h && cidx < w) {
                            max_val = max(max_val, input[idx]);
                        }
                    }
                }
                output[ch * out_h * out_w + i * out_w + j] = max_val;
            }
        }
    }
    return output;
}

Vector1D fully_connect(const Vector1D& input, const Vector1D& weight, const Vector1D& bias,
                       int input_size, int num_neurons) {
    Vector1D output(num_neurons);
    for (int n = 0; n < num_neurons; ++n) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; ++i)
            sum += input[i] * weight[n * input_size + i];
        float val = sum + bias[n];
        val = (val > 0) ? val : 0;
        output[n] = val;
    }
    return output;
}