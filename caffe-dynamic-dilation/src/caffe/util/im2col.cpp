#include <vector>

#include "caffe/util/im2col.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

// Function uses casting from int to unsigned to compare if value of
// parameter a is greater or equal to zero and lower than value of
// parameter b. The b parameter is of type signed and is always positive,
// therefore its value is always lower than 0x800... where casting
// negative value of a parameter converts it to value higher than 0x800...
// The casting allows to use one condition instead of two.
inline bool is_a_ge_zero_and_a_lt_b(int a, int b) {
  return static_cast<unsigned>(a) < static_cast<unsigned>(b);
}

template <typename Dtype>
void im2col_cpu(const Dtype* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w,
    const int stride_h, const int stride_w,
    const int dilation_h, const int dilation_w,
    Dtype* data_col) {
  const int output_h = (height + 2 * pad_h -
    (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;
  const int output_w = (width + 2 * pad_w -
    (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;
  const int channel_size = height * width;
  for (int channel = channels; channel--; data_im += channel_size) {
    for (int kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
      for (int kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
        int input_row = -pad_h + kernel_row * dilation_h;
        for (int output_rows = output_h; output_rows; output_rows--) {
          if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {
            for (int output_cols = output_w; output_cols; output_cols--) {
              *(data_col++) = 0;
            }
          } else {
            int input_col = -pad_w + kernel_col * dilation_w;
            for (int output_col = output_w; output_col; output_col--) {
              if (is_a_ge_zero_and_a_lt_b(input_col, width)) {
                *(data_col++) = data_im[input_row * width + input_col];
              } else {
                *(data_col++) = 0;
              }
              input_col += stride_w;
            }
          }
          input_row += stride_h;
        }
      }
    }
  }
}

// Explicit instantiation
template void im2col_cpu<float>(const float* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, const int stride_h,
    const int stride_w, const int dilation_h, const int dilation_w,
    float* data_col);
template void im2col_cpu<double>(const double* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, const int stride_h,
    const int stride_w, const int dilation_h, const int dilation_w,
    double* data_col);
/*
template <typename Dtype>
void im2col_dynamic_cpu(const Dtype* data_im, const Dtype* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, Dtype* data_col) {
  // effective kernel if we expand the dilations (trous)
  const int kernel_h_eff = kernel_h + (kernel_h - 1) * (pad_h - 1);
  const int kernel_w_eff = kernel_w + (kernel_w - 1) * (pad_w - 1);

  int height_col = (height + 2 * pad_h - kernel_h_eff) / stride_h + 1;
  int width_col = (width + 2 * pad_w - kernel_w_eff) / stride_w + 1;

  int channels_col = channels * kernel_h * kernel_w;

  for (int n = 0; n < num; ++n) {
    for (int c = 0; c < channels_col; ++c) {

      int c_im = c / kernel_w / kernel_h;
      for (int h = 0; h < height_col; ++h) {
	for (int w = 0; w < width_col; ++w) {
          Dtype dilation = dilation_im[(n * height + h) * width + w];
          int iDilationFloor = static_cast<int>(dilation);
          int iDilationCeil = iDilationFloor + 1;
          if (iDilationFloor == pad_h) {
            iDilationFloor = iDilationFloor - 1;
            iDilationCeil = iDilationCeil - 1;
          }
          double dist = iDilationCeil - dilation;

          int w_offset_ceil = (c % kernel_w)  * iDilationCeil;				//w offset, for different position, w offset should be different
          int h_offset_ceil = ((c / kernel_w) % kernel_h) * iDilationCeil;
          int w_offset_floor = (c % kernel_w)  * iDilationFloor;			//w offset, for different position, w offset should be different
          int h_offset_floor = ((c / kernel_w) % kernel_h) * iDilationFloor;

	  const int h_im_ceil = h * stride_h + h_offset_ceil - iDilationCeil;
	  const int w_im_ceil = w * stride_w + w_offset_ceil - iDilationCeil;
	  const int h_im_floor = h * stride_h + h_offset_floor - iDilationFloor;
	  const int w_im_floor = w * stride_w + w_offset_floor - iDilationFloor;

          if (h_im_ceil >= 0 && h_im_ceil < height && w_im_ceil >= 0 && w_im_ceil < width) {
            data_col[((n * channels_col + c) * height_col + h) * width_col + w] =
	    data_im[((n * channels + c_im) * height + h_im_ceil) * width + w_im_ceil] * (1-dist) * (1-dist) + 
	    data_im[((n * channels + c_im) * height + h_im_floor) * width + w_im_ceil] * (1-dist) * dist + 
	    data_im[((n * channels + c_im) * height + h_im_ceil) * width + w_im_floor] * (1-dist) * dist + 
	    data_im[((n * channels + c_im) * height + h_im_floor) * width + w_im_floor] * dist * dist;
          } else {
            data_col[((n * channels_col + c) * height_col + h) * width_col + w] = 0;
          }
	}
      }
    }
  }
}
*/
template <typename Dtype>
void im2col_dynamic_cpu(const Dtype* data_im, const Dtype* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, Dtype* data_col) {
  // effective kernel if we expand the dilations (trous)
  const int kernel_h_eff = kernel_h + (kernel_h - 1) * (pad_h - 1);
  const int kernel_w_eff = kernel_w + (kernel_w - 1) * (pad_w - 1);



  int height_col = (height + 2 * pad_h - kernel_h_eff) / stride_h + 1;
  int width_col = (width + 2 * pad_w - kernel_w_eff) / stride_w + 1;


  int channels_col = channels * kernel_h * kernel_w;


  for (int n = 0; n < num; ++n) {
    for (int c = 0; c < channels_col; ++c) {

      int c_im = c / kernel_w / kernel_h;
      for (int h = 0; h < height_col; ++h) {
	for (int w = 0; w < width_col; ++w) {
          Dtype dilation = dilation_im[(n * height + h) * width + w];
          int iDilationFloor = static_cast<int>(dilation);
          if (dilation - iDilationFloor > 0.5) {
            dilation = iDilationFloor + 1;
          } else {
            dilation = iDilationFloor;
          }

          int w_offset = (c % kernel_w)  * dilation;				//w offset, for different position, w offset should be different
          int h_offset = ((c / kernel_w) % kernel_h) * dilation;

	  const int h_im = h * stride_h + h_offset - dilation;
	  const int w_im = w * stride_w + w_offset - dilation;



	  data_col[((n * channels_col + c) * height_col + h) * width_col + w] =
	    (h_im >= 0 && h_im < height && w_im >= 0 && w_im < width) ?
	    data_im[((n * channels + c_im) * height + h_im) * width + w_im] : 
	    0.; // zero-pad
	}
      }
    }
  }
}

// Explicit instantiation
template void im2col_dynamic_cpu<float>(const float* data_im, const float* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, float* data_col);
template void im2col_dynamic_cpu<double>(const double* data_im, const double* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, double* data_col);

template <typename Dtype>
inline void im2col_nd_core_cpu(const Dtype* data_input, const bool im2col,
    const int num_spatial_axes, const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, Dtype* data_output) {
  if (!im2col) {
    int im_size = im_shape[0];
    for (int i = 0; i < num_spatial_axes; ++i) {
      im_size *= im_shape[1 + i];
    }
    caffe_set(im_size, Dtype(0), data_output);
  }
  int kernel_size = 1;
  for (int i = 0; i < num_spatial_axes; ++i) {
    kernel_size *= kernel_shape[i];
  }
  const int channels_col = col_shape[0];
  vector<int> d_offset(num_spatial_axes, 0);
  vector<int> d_iter(num_spatial_axes, 0);
  for (int c_col = 0; c_col < channels_col; ++c_col) {
    // Loop over spatial axes in reverse order to compute a per-axis offset.
    int offset = c_col;
    for (int d_i = num_spatial_axes - 1; d_i >= 0; --d_i) {
      if (d_i < num_spatial_axes - 1) {
        offset /= kernel_shape[d_i + 1];
      }
      d_offset[d_i] = offset % kernel_shape[d_i];
    }
    for (bool incremented = true; incremented; ) {
      // Loop over spatial axes in forward order to compute the indices in the
      // image and column, and whether the index lies in the padding.
      int index_col = c_col;
      int index_im = c_col / kernel_size;
      bool is_padding = false;
      for (int d_i = 0; d_i < num_spatial_axes; ++d_i) {
        const int d = d_iter[d_i];
        const int d_im = d * stride[d_i] - pad[d_i] +
            d_offset[d_i] * dilation[d_i];
        is_padding |= d_im < 0 || d_im >= im_shape[d_i + 1];
        index_col *= col_shape[d_i + 1];
        index_col += d;
        index_im *= im_shape[d_i + 1];
        index_im += d_im;
      }
      if (im2col) {
        if (is_padding) {
          data_output[index_col] = 0;
        } else {
          data_output[index_col] = data_input[index_im];
        }
      } else if (!is_padding) {  // col2im
        data_output[index_im] += data_input[index_col];
      }
      // Loop over spatial axes in reverse order to choose an index,
      // like counting.
      incremented = false;
      for (int d_i = num_spatial_axes - 1; d_i >= 0; --d_i) {
        const int d_max = col_shape[d_i + 1];
        DCHECK_LT(d_iter[d_i], d_max);
        if (d_iter[d_i] == d_max - 1) {
          d_iter[d_i] = 0;
        } else {  // d_iter[d_i] < d_max - 1
          ++d_iter[d_i];
          incremented = true;
          break;
        }
      }
    }  // while(incremented) {
  }  // for (int c = 0; c < channels_col; ++c) {
}

template <typename Dtype>
void im2col_nd_cpu(const Dtype* data_im, const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, Dtype* data_col) {
  const bool kIm2Col = true;
  im2col_nd_core_cpu(data_im, kIm2Col, num_spatial_axes, im_shape, col_shape,
                  kernel_shape, pad, stride, dilation, data_col);
}

// Explicit instantiation
template void im2col_nd_cpu<float>(const float* data_im,
    const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, float* data_col);
template void im2col_nd_cpu<double>(const double* data_im,
    const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, double* data_col);

template <typename Dtype>
void col2im_cpu(const Dtype* data_col, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w,
    const int stride_h, const int stride_w,
    const int dilation_h, const int dilation_w,
    Dtype* data_im) {
  caffe_set(height * width * channels, Dtype(0), data_im);
  const int output_h = (height + 2 * pad_h -
    (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;
  const int output_w = (width + 2 * pad_w -
    (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;
  const int channel_size = height * width;
  for (int channel = channels; channel--; data_im += channel_size) {
    for (int kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
      for (int kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
        int input_row = -pad_h + kernel_row * dilation_h;
        for (int output_rows = output_h; output_rows; output_rows--) {
          if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {
            data_col += output_w;
          } else {
            int input_col = -pad_w + kernel_col * dilation_w;
            for (int output_col = output_w; output_col; output_col--) {
              if (is_a_ge_zero_and_a_lt_b(input_col, width)) {
                data_im[input_row * width + input_col] += *data_col;
              }
              data_col++;
              input_col += stride_w;
            }
          }
          input_row += stride_h;
        }
      }
    }
  }
}

// Explicit instantiation
template void col2im_cpu<float>(const float* data_col, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, const int stride_h,
    const int stride_w, const int dilation_h, const int dilation_w,
    float* data_im);
template void col2im_cpu<double>(const double* data_col, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, const int stride_h,
    const int stride_w, const int dilation_h, const int dilation_w,
    double* data_im);

template <typename Dtype>
void col2im_dynamic_cpu(const Dtype* data_col, const Dtype* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, Dtype* data_im) {
  caffe_set(num * channels * height * width, Dtype(0), data_im);

  const int kernel_h_eff = kernel_h + (kernel_h - 1) * (pad_h - 1);
  const int kernel_w_eff = kernel_w + (kernel_w - 1) * (pad_w - 1);

  int height_col = (height + 2 * pad_h - kernel_h_eff) / stride_h + 1;
  int width_col = (width + 2 * pad_w - kernel_w_eff) / stride_w + 1;

  int channels_col = channels * kernel_h * kernel_w;

  for (int n = 0; n < num; ++n) {
    for (int c = 0; c < channels_col; ++c) {
      int c_im = c / kernel_w / kernel_h;
      for (int h = 0; h < height_col; ++h) {
	for (int w = 0; w < width_col; ++w) {
          Dtype dilation = dilation_im[(n * height + h) * width + w];
          int iDilationFloor = static_cast<int>(dilation);
          if (dilation - iDilationFloor > 0.5) {
            dilation = iDilationFloor + 1;
          } else {
            dilation = iDilationFloor;
          }

          int w_offset = (c % kernel_w)  * dilation;
          int h_offset = ((c / kernel_w) % kernel_h) * dilation;
          
          const int h_im = h * stride_h + h_offset - dilation;
	  const int w_im = w * stride_w + w_offset - dilation;


	  if (h_im >= 0 && h_im < height && w_im >= 0 && w_im < width) {
	    data_im[((n * channels + c_im) * height + h_im) * width + w_im] += 
	      data_col[((n * channels_col + c) * height_col + h) * width_col + w];
	  }
	}
      }
    }
  }
}

// Explicit instantiation
template void col2im_dynamic_cpu<float>(const float* data_col, const float* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, float* data_im);
template void col2im_dynamic_cpu<double>(const double* data_col, const double* dilation_im,
    const int num, const int channels, const int height, const int width,
    const int kernel_h, const int kernel_w, const int pad_h, const int pad_w,
    const int stride_h, const int stride_w, double* data_im);

template <typename Dtype>
void col2im_nd_cpu(const Dtype* data_col, const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, Dtype* data_im) {
  const bool kIm2Col = false;
  im2col_nd_core_cpu(data_col, kIm2Col, num_spatial_axes, im_shape, col_shape,
                     kernel_shape, pad, stride, dilation, data_im);
}

// Explicit instantiation
template void col2im_nd_cpu<float>(const float* data_col,
    const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, float* data_im);
template void col2im_nd_cpu<double>(const double* data_col,
    const int num_spatial_axes,
    const int* im_shape, const int* col_shape,
    const int* kernel_shape, const int* pad, const int* stride,
    const int* dilation, double* data_im);


}  // namespace caffe
