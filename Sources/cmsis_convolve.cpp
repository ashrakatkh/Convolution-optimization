#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
#define NNOM_TRUNCATE
#define q7_t 	int8_t
#define q15_t 	int16_t
#define q31_t 	int32_t
#define q63_t 	int64_t
#ifndef NNOM_TRUNCATE 
    #define NN_ROUND(out_shift) ((0x1 << out_shift) >> 1 )
#else
    #define NN_ROUND(out_shift) 0
#endif

#ifndef __SSAT
static inline int __SSAT(int32_t value, int32_t bit) {
    int32_t min = -(1<<(bit-1));
    int32_t max = (1<<(bit-1)) - 1;
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}
#endif


q7_t __SMLAD(q7_t in1, q7_t in2, q7_t sum)
{
    q7_t out = in1 * in2;
    sum += out;
    return sum;
}
q7_t nn_read_q7_ia(const q7_t **in_q7)
{
    q7_t val;

    memcpy(&val, *in_q7, 1);
    *in_q7 += 1;

    return (val);
}

q7_t *nn_mat_mult_kernel_q7_q15(const q7_t *pA,
                                    const q7_t *pInBuffer,
                                    const uint16_t ch_im_out,
                                    const uint16_t numCol_A,
                                    const uint16_t bias_shift,
                                    const uint16_t out_shift,
                                    const q7_t *bias,
                                    q7_t *pOut)
{

    /* set up the second output pointers */
    q7_t *pOut2 = pOut + ch_im_out;
    const q7_t *pBias = bias;

    uint16_t rowCnt = ch_im_out >> 1;
    /* this loop over rows in A */
    for (int i =0 ; i< 27; i++)
     cout << "pA[ " <<i << "} = " << signed(pA [i]) << endl;
    while (rowCnt)
    {
        /* setup pointers for B */
        const q7_t *pB = pInBuffer;
        const q7_t *pB2 = pB + numCol_A;
          for  (int i=0; i<8; i++)
        {
            cout << "pB[ " << i << "] = "<< signed (pB[i]) << endl;
            cout << "pB2[ " << i << "] = "<< signed (pB2[i]) << endl;
        }
        /* align the second pointer for A */
        const q7_t *pA2 = pA + numCol_A;

        /* init the sum with bias */
        q31_t sum = ((q31_t)(*pBias) << bias_shift) + NN_ROUND(out_shift);
        q31_t sum2 = ((q31_t)(*pBias++) << bias_shift) + NN_ROUND(out_shift);
        q31_t sum3 = ((q31_t)(*pBias) << bias_shift) + NN_ROUND(out_shift);
        q31_t sum4 = ((q31_t)(*pBias++) << bias_shift) + NN_ROUND(out_shift);
       
        uint16_t colCnt = numCol_A >> 1;
        /* accumulate over the vector */
      
        while (colCnt)
        {
            q7_t inA11, inA12, inA21, inA22, inB1, inB2;
             inB1 =nn_read_q7_ia(&pB);
             inB2 =nn_read_q7_ia(&pB2);
            inA11 =nn_read_q7_ia(&pA);
            inA12 =nn_read_q7_ia(&pA);
            inA21 =nn_read_q7_ia(&pA2);
            inA22 =nn_read_q7_ia(&pA2);

             cout << "sum = sum +  inA11 * inB1 = "<< sum ;
            sum = __SMLAD(inA11, inB1, sum);
            sum2 = __SMLAD(inA11, inB2, sum2);
            sum3 = __SMLAD(inA21, inB1, sum3);
            sum4 = __SMLAD(inA21, inB2, sum4);
            cout << " + " << signed(inA11) << " * " << signed(inB1) << " = " << sum << endl;
        
            inB1 =nn_read_q7_ia(&pB);
             inB2 =nn_read_q7_ia(&pB2);

             cout << "sum = sum +  inA12 * inB1 = "<< sum ;
            sum = __SMLAD(inA12, inB1, sum);
            sum2 = __SMLAD(inA12, inB2, sum2);
            sum3 = __SMLAD(inA22, inB1, sum3);
            sum4 = __SMLAD(inA22, inB2, sum4);
          cout << " + " << signed(inA12) << " * " << signed(inB1) << " = " << sum << endl;
            colCnt--;
        } /* while over colCnt */
        colCnt = numCol_A & 0x1;
        while (colCnt)
        {
            q7_t inA1 = *pA++;
            q15_t inB1 = *pB++;
            q7_t inA2 = *pA2++;
            q15_t inB2 = *pB2++;
        cout << "sum = sum +  inA1* inB1 = "<< sum ;
            sum = __SMLAD(inA1, inB1, sum);
            sum2 = __SMLAD(inA1, inB2, sum2);
            sum3 = __SMLAD(inA2, inB1, sum3);
            sum4 = __SMLAD(inA2, inB2, sum4);
    
            colCnt--;
            cout << " + " << signed(inA1) << " * " << signed(inB1) << " = " << sum << endl;
        } /* while over colCnt */

        cout << "sum = " << sum << endl;
        cout << "sum2 = " << sum2 << endl;
        cout << "sum3 = " << sum3 << endl;
        cout << "sum4 = " << sum4 << endl;

        *pOut++ = sum ;
        *pOut++ = sum3 ;
        *pOut2++ = sum2;
        *pOut2++ = sum4;
       // *pOut++ = (q7_t)__SSAT((sum >> out_shift), 8);
       // *pOut++ = (q7_t)__SSAT((sum3 >> out_shift), 8);
        //*pOut2++ = (q7_t)__SSAT((sum2 >> out_shift), 8);
        //*pOut2++ = (q7_t)__SSAT((sum4 >> out_shift), 8);

        /* skip the row computed with A2 */
        pA += numCol_A;
        rowCnt--;
    } /* for over ch_im_out */

    /* compute left-over row if any */
    if (ch_im_out & 0x1)
    {
        /* setup pointers for B */
        const q7_t *pB = pInBuffer;
        const q7_t *pB2 = pB + numCol_A;

        /* load the bias */
        q31_t sum = ((q31_t)(*pBias) << bias_shift) + NN_ROUND(out_shift);
        q31_t sum2 = ((q31_t)(*pBias++) << bias_shift) + NN_ROUND(out_shift);

        uint16_t colCnt = numCol_A >> 2;
        while (colCnt)
        {
            q7_t inA11, inA12, inB1, inB2;

            inB1 =nn_read_q7_ia(&pB);
             inB2 =nn_read_q7_ia(&pB2);
            inA11 =nn_read_q7_ia(&pA);
            inA12 =nn_read_q7_ia(&pA);

            sum = __SMLAD(inA11, inB1, sum);
            sum2 = __SMLAD(inA11, inB2, sum2);

             inB1 =nn_read_q7_ia(&pB);
             inB2 =nn_read_q7_ia(&pB2);
            sum = __SMLAD(inA12, inB1, sum);
            sum2 = __SMLAD(inA12, inB2, sum2);

            colCnt--;
        }
        colCnt = numCol_A & 0x3;
        while (colCnt)
        {
            q7_t inA1 = *pA++;
            q15_t inB1 = *pB++;
            q15_t inB2 = *pB2++;

            sum += inA1 * inB1;
            sum2 += inA1 * inB2;
            colCnt--;
        }
        cout << "sum = "<<sum << endl;
        cout << "sum2 = "<<sum2 << endl;
       *pOut++ = sum;
       *pOut2++ = sum2;
        //*pOut++ = (q7_t)__SSAT((sum >> out_shift), 8);
        //*pOut2++ = (q7_t)__SSAT((sum2 >> out_shift), 8);
    }

    pOut += ch_im_out;

    /* return the new output pointer with offset */
    return pOut;

}



/*
* @brief Basic Q7 convolution function (non-sqaure shape)
 * @param[in]       Im_in        pointer to input tensor
 * @param[in]       dim_im_in_x  input tensor dimention x
 * @param[in]       dim_im_in_y  input tensor dimention y
 * @param[in]       ch_im_in     number of input tensor channels
 * @param[in]       wt           pointer to kernel weights
 * @param[in]       ch_im_out    number of filters, i.e., output tensor channels
 * @param[in]       dim_kernel_x filter kernel size x
 * @param[in]       dim_kernel_y filter kernel size y
 * @param[in]       padding_x    padding size x
 * @param[in]       padding_y    padding size y
 * @param[in]       stride_x     convolution stride x
 * @param[in]       stride_y     convolution stride y
 * @param[in]       bias         pointer to bias
 * @param[in]       bias_shift   amount of left-shift for bias
 * @param[in]       out_shift    amount of right-shift for output
 * @param[in,out]   Im_out       pointer to output tensor
 * @param[in]       dim_im_out_x output tensor dimension x
 * @param[in]       dim_im_out_y output tensor dimension y
 * @param[in,out]   bufferA      pointer to buffer space for input
 * @param[in,out]   bufferB      pointer to buffer space for output
 * @return     The function returns <code>ARM_MATH_SUCCESS</code>
 */

void convolve_HWC_q7_basic_nonsquare(          const q7_t *Im_in,
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

    /* Run the following code for Cortex-M4 and Cortex-M7 */

    int16_t i_out_y, i_out_x, i_ker_y, i_ker_x;
    int x=0;
    /*
     *  Here we use bufferA as q15_t internally as computation are done with q15_t level
     *  im2col are done to output in q15_t format from q7_t input
     */
    q7_t *pBuffer = bufferA;
    q7_t *pOut = Im_out;
   
    int* v [3]={0,0,0};
    /* This part implements the im2col function */
    for (i_out_y = 0; i_out_y < dim_im_out_y; i_out_y++)
    {
        for (i_out_x = 0; i_out_x < dim_im_out_x; i_out_x++)
        {
            for (i_ker_y = i_out_y * stride_y - padding_y; i_ker_y < i_out_y * stride_y - padding_y + dim_kernel_y;
                 i_ker_y++)
            {
                for (i_ker_x = i_out_x * stride_x - padding_x; i_ker_x < i_out_x * stride_x - padding_x + dim_kernel_x;
                     i_ker_x++)
                {
                    if (i_ker_y < 0 || i_ker_y >= dim_im_in_y || i_ker_x < 0 || i_ker_x >= dim_im_in_x)
                    {
                        /* Filling 0 for out-of-bound paddings */
                     
                        memset(pBuffer + x , 0, sizeof(q7_t) * ch_im_in);
                             x+= ch_im_in;
                    }
                    else
                    {
                        /* Copying the pixel data to column */
                         cout <<" I am in else before memcpy and x = "<< x << endl;
                        for (int i =0; i< 27; i++)
                            cout << "wt[ " << i << "] = " << signed(wt[i])  << endl;
                         cout << "pBuffer [15] = " << signed(pBuffer[15]) << endl;
                         memcpy (  pBuffer + x , 
                         (q7_t *)Im_in + (i_ker_y * dim_im_in_x + i_ker_x) * ch_im_in,  sizeof(q7_t) * ch_im_in);
                        
                
                        cout <<" I am in else and x = "<< x << endl;
                        for (int i =0; i< 27; i++)
                            cout << "wt[ " << i << "] = " << signed(wt[i])  << endl;
                         
                         if ( x == 15)
                         {
                              for ( int i =0; i< x+3; i++)
                              cout << "pppBuffer ["<<i<<"]= "<<signed (pBuffer[i]) <<endl;
                              
 
                         }
                         x+= ch_im_in;
                       

                    }

                }
            }
            
            /* Computation is filed for every 2 columns */
            if (pBuffer + x  == bufferA + 2 * ch_im_in * dim_kernel_y * dim_kernel_x)
            {
    
                pOut = nn_mat_mult_kernel_q7_q15(
                    wt, bufferA, ch_im_out, ch_im_in * dim_kernel_y * dim_kernel_x, bias_shift, out_shift, bias, pOut);
                  cout << "hrllo"<<endl;
                /* counter reset */
                pBuffer = bufferA;
                x =0;
            }
        }
    }

    /* left-over because odd number of output pixels */
    if (pBuffer + x != bufferA)
    {
        const q7_t *pA = wt;
        int i;

        for (i = 0; i < ch_im_out; i++)
        {
            /* Load the accumulator with bias first */
            q31_t sum = ((q31_t)bias[i] << bias_shift) + NN_ROUND(out_shift);

            /* Point to the beging of the im2col buffer */
            const q7_t *pB = bufferA;

            /* Each time it process 4 entries */
            uint16_t colCnt = ch_im_in * dim_kernel_y * dim_kernel_x >> 1;

            while (colCnt)
            {
                q31_t inA1;
                q31_t inA2;
                q31_t inB1, inB2;

                //pA = read_and_pad(pA, &inA1, &inA2);

                inA1 =nn_read_q7_ia(&pA);
                inA2 =nn_read_q7_ia(&pA);
  

                inB1 =nn_read_q7_ia(&pB);
                inB2 =nn_read_q7_ia(&pB);
                sum = __SMLAD(inA1, inB1, sum);
                sum = __SMLAD(inA2, inB2, sum);
                colCnt--;
            }
            colCnt = ch_im_in * dim_kernel_y * dim_kernel_x & 0x1;
            while (colCnt)
            {
                q7_t inA1 = *pA++;
                q15_t inB1 = *pB++;
                sum += inA1 * inB1;
                colCnt--;
            }
            //*pOut++ = (q7_t)__SSAT((sum >> out_shift), 8);
            cout << "sum  = "<< sum << endl;
            *pOut++ = sum;
        }
    }

}
/**
 * @} end of NNConv group
 */

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
    const uint16_t out_shift =24;
   
    const uint16_t dim_im_out_x = 3;
    const uint16_t dim_im_out_y = 3;
    q7_t Im_out [ dim_im_out_x * dim_im_out_y *ch_im_out];
    q7_t *bufferA;
    q7_t *bufferB;
    bufferA = new q7_t [2* ch_im_out * dim_kernel_x * dim_kernel_y];
  int n = dim_im_out_x * dim_im_out_y *ch_im_out;

  //for (int i =0; i< 27; i++)
   // cout << "wt[ " << i << "] = " << signed(wt[i])  << endl;
  convolve_HWC_q7_basic_nonsquare (Im_in,dim_im_in_x, dim_im_in_y, ch_im_in, wt, ch_im_out, dim_kernel_x,
    dim_kernel_y, padding_x, padding_y, stride_x, stride_y,bias, bias_shift, out_shift,Im_out, dim_im_out_x,
    dim_im_out_y, bufferA, bufferB);
    
    
    for (int i =0; i < n ; i++)
    
    {
        cout << signed(Im_out [i])<<endl;  
       
    }
    return 0;
}