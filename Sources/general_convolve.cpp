

#include <stdio.h>
#include <iostream>
using namespace std;

#define q7_t 	int8_t
#define q15_t 	int16_t
#define q31_t 	int32_t
#define q63_t 	int64_t

#define NNOM_TRUNCATE 
#ifndef NNOM_TRUNCATE 
    #define NN_ROUND(out_shift) ((0x1 << out_shift) >> 1 )
#else
    #define NN_ROUND(out_shift) 0
#endif

#ifndef __SSAT
static inline int __SSAT(int32_t value, int32_t bit) {
    cout << "value = "<< value << endl; 
         
    int32_t min = -(1<<(bit-1));
    int32_t max = (1<<(bit-1)) - 1;
    cout << "min = " <<min <<endl;
    cout << "max = " <<max << endl; 
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}
#endif

void convolve_HWC_q7_basic_nonsquare(const q7_t *Im_in,
                                               const uint16_t dim_im_in_x,
                                               const uint16_t dim_im_in_y,
                                               const uint16_t ch_im_in,
                                               const q7_t *wt,
                                               const uint16_t ch_im_out,
                                               const uint16_t dim_kernel_x,
                                               const uint16_t dim_kernel_y,
                                               const uint16_t padding_x,
                                               const uint16_t padding_y,
                                               const uint16_t stride_x,
                                               const uint16_t stride_y,
                                               const q7_t *bias,
                                               const uint16_t bias_shift,
                                               const uint16_t out_shift,
                                               q7_t *Im_out,
                                               const uint16_t dim_im_out_x,
                                               const uint16_t dim_im_out_y,
                                               q7_t *bufferA,
                                               q7_t *bufferB)
{
    (void)bufferB;
    (void)bufferA;
    int i, j, k, l, m, n;
    int conv_out;
    int in_row, in_col;

    for (i = 0; i < ch_im_out; i++)
    {
        for (j = 0; j < dim_im_out_y; j++)
        {
            for (k = 0; k < dim_im_out_x; k++)
            {
                //conv_out = ((q31_t)bias[i] << bias_shift) + NN_ROUND(out_shift);
                conv_out = bias[i];
                cout <<"convout = " << conv_out <<endl;
                //conv_out = 0;
                for (m = 0; m < dim_kernel_y; m++)
                {
                    for (n = 0; n < dim_kernel_x; n++)
                    {
                        // if-for implementation
                        
                        in_row = stride_y * j + m - padding_y;
                        in_col = stride_x * k + n - padding_x;
                        if (in_row >= 0 && in_col >= 0 && in_row < dim_im_in_y && in_col < dim_im_in_x)
                        {
                            for (l = 0; l < ch_im_in; l++)
                            {
                                cout<< "Im_in [ " << (in_row * dim_im_in_x + in_col) * ch_im_in + l <<"] * wt [ " << i * ch_im_in * dim_kernel_y * dim_kernel_x + (m * dim_kernel_x + n) * ch_im_in +
                                       l<<"] = " << signed(Im_in[(in_row * dim_im_in_x + in_col) * ch_im_in + l]) << " * " <<
                                signed (wt[i * ch_im_in * dim_kernel_y * dim_kernel_x + (m * dim_kernel_x + n) * ch_im_in +
                                       l])<< " = " << Im_in[(in_row * dim_im_in_x + in_col) * ch_im_in + l] *
                                    wt[i * ch_im_in * dim_kernel_y * dim_kernel_x + (m * dim_kernel_x + n) * ch_im_in +
                                       l]<< endl;

                                conv_out += Im_in[(in_row * dim_im_in_x + in_col) * ch_im_in + l] *
                                    wt[i * ch_im_in * dim_kernel_y * dim_kernel_x + (m * dim_kernel_x + n) * ch_im_in +
                                       l];
                            }
                        }
                    }
                }
                Im_out[i + (j * dim_im_out_x + k) * ch_im_out] = conv_out;
                cout << " Im_out[ " << i + (j * dim_im_out_x + k) * ch_im_out << "] = " << signed (conv_out) << endl;
                //int test = conv_out >> out_shift;
                //cout << " test value = "<< test << endl;
                //Im_out[i + (j * dim_im_out_x + k) * ch_im_out] = (q7_t)__SSAT((conv_out >> out_shift), 8);
            }
        }
    }
}


int main()
{
    //printf("Hello World");
    const q7_t Im_in[75] = { 2,1,0,
                             1,0,0,
                             1,2,0,
                             2,2,2,
                             2,1,0,

                             1,2,1,
                             2,2,1,
                             2,1,2,
                             2,1,2,
                             1,0,1,

                             0,0,1,
                             2,2,1,
                             2,1,1,
                             1,0,2,
                             0,2,1,

                             1,0,1,
                             0,1,0,
                             1,2,1,
                             2,2,2,
                             0,2,0,

                             0,1,0,
                             1,1,2,
                             1,2,0,
                             0,2,2,
                             1,2,2
                           };


        
        
        
        /*
                         2,1,1,2,2,
                         1,2,2,2,1,
                         0,2,2,1,0,
                         1,0,1,2,0,
                         0,1,1,0,1,
        
                         1,0,2,2,1,
                         2,2,1,1,0,
                         0,2,1,0,2,
                         0,1,2,2,2,
                         1,1,2,2,2,
                         
                         0,0,0,2,0,
                         1,1,2,2,1,
                         1,1,1,2,1,
                         1,0,1,2,0,
                         0,2,0,2,2
    */
    const uint16_t dim_im_in_x = 5;
    const uint16_t dim_im_in_y = 5;
    const uint16_t ch_im_in = 3;
    const q7_t wt[54] = {
                          1,0,1,
                          0,0,-1,
                          -1,1,0,

                          0,0,-1,
                          0,-1,-1,
                          1,-1,-1,

                          1,1,1,
                          1,0,-1,
                          0,-1,-1,

                          0,-1,0,
                          0,-1,0,
                          1,-1,-1,

                          -1,0,-1,
                          -1,0,-1,
                          1,-1,1,

                          -1, -1, 0,
                          1, 1, -1,
                          -1, -1, 1
    };

        
     /*   
        
        
                        1,0,-1,
                         0,0,1,
                         1,1,0,
                      
                      0,0,1,
                      0,-1,-1,
                      1,0,-1,
                      
                      1,-1,0,
                      -1,-1,-1,
                      1,-1,-1,
                      
                      0,0,1,
                      -1,-1,1,
                      -1,1,-1,
                      
                      -1,-1,-1,
                      0,0,-1,
                      -1,1,-1,
                      
                      0,0,-1,
                      -1,-1,1,
                      0,-1,1
     */
                      
     const uint16_t ch_im_out = 2;
    const uint16_t dim_kernel_x = 3;
    const uint16_t dim_kernel_y =3;
    const uint16_t padding_x = 1;
    const uint16_t padding_y = 1;
    const uint16_t stride_x = 2;
    const uint16_t stride_y = 2;
    const q7_t bias[2] = {1,0};
    const uint16_t bias_shift = 0;
    const uint16_t out_shift =0;
   
    const uint16_t dim_im_out_x = 3;
    const uint16_t dim_im_out_y = 3;
    q7_t Im_out [ dim_im_out_x * dim_im_out_y *ch_im_out];
    q7_t *bufferA;
    q7_t *bufferB;

  int n = dim_im_out_x * dim_im_out_y *ch_im_out;
   
  convolve_HWC_q7_basic_nonsquare (Im_in,dim_im_in_x, dim_im_in_y, ch_im_in, wt, ch_im_out, dim_kernel_x,
    dim_kernel_y, padding_x, padding_y, stride_x, stride_y,bias, bias_shift, out_shift,Im_out, dim_im_out_x,
    dim_im_out_y, bufferA, bufferB);
    
    //int n = dim_im_out_x * dim_im_out_y *ch_im_out;
    for (int i =0; i < n ; i++)
    
    {
        cout << signed(Im_out [i])<<endl;
    }
    return 0;
}