//==============================================================
// DPC++ Example
//
// Image Rotation with DPC++
//
// Author: Pat Hoey
//
// Copyright ©  2022-
//
// MIT License
//
#include <CL/sycl.hpp>
#include <array>
#include <iostream>
#include "dpc_common.hpp"
#if FPGA || FPGA_EMULATOR || FPGA_PROFILE
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

using namespace sycl;

// useful header files for image rotation
#include "utils.h"
#include "bmp-utils.h"
#include "gold.h"

#define PI 3.14159265


using Duration = std::chrono::duration<double>;
class Timer {
 public:
  Timer() : start(std::chrono::steady_clock::now()) {}

  Duration elapsed() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<Duration>(now - start);
  }

 private:
  std::chrono::steady_clock::time_point start;
};

static const char* inputImagePath = "./Images/cat.bmp";
static const char* outputImagePath = "./cat-rot.bmp";

#define IMAGE_SIZE (720*1080)
constexpr size_t array_size = IMAGE_SIZE;
typedef std::array<float, array_size> FloatArray;

//************************************
// Image Rotation in DPC++ on device:
//************************************
void ImageRot_v1(queue &q, float *image_in, float *image_out, float cos_value,
                 float sin_value, const size_t ImageRows, const size_t ImageCols)
{

    // We create buffers for the input and output data.
    //
    buffer<float, 1> image_in_buf(image_in, range<1>(ImageRows*ImageCols));
    buffer<float, 1> image_out_buf(image_out, range<1>(ImageRows*ImageCols));


    // Create the range object for the pixel data.
    range<2> num_items{ImageRows, ImageCols};

    // Submit a command group to the queue by a lambda function that contains the
    // data access permission and device computation (kernel).
    q.submit([&](handler &h) {
      // Create an accessor to buffers with access permission: read, write or
      // read/write. The accessor is a way to access the memory in the buffer.
      accessor srcPtr(image_in_buf, h, read_only);

      // Another way to get access is to call get_access() member function 
      auto dstPtr = image_out_buf.get_access<access::mode::write>(h);

      // Use parallel_for to run image rotation in parallel on device. This
      // executes the kernel.
      //    1st parameter is the number of work items.
      //    2nd parameter is the kernel, a lambda that specifies what to do per
      //    work item. The parameter of the lambda is the work item id.
      // DPC++ supports unnamed lambda kernel by default.
      h.parallel_for(num_items, [=](id<2> item)
      {

        // get row and col of the pixel assigned to this work item
        int row = item[0];
        int col = item[1];


        // Declare center point of the image to rotate about
        int x0, y0;

        //x0 = (int)ImageRows / 2;
        //y0 = (int)ImageCols / 2;
	x0 = 0;
	y0 = 0;

        // Declare initial points and new points
        int x1, x2, y1, y2;
	// Declare the intermediete values
	float x_rotated, y_rotated;

        // Get relative point for pixel this work item is handling
        x1 = row;
        y1 = col;

        // Calculate new values
        x_rotated = cos_value * (float)(x1 - x0) + sin_value * (float)(y1 - y0);
        y_rotated = (-1.0) * sin_value * (float)(x1 - x0) + cos_value * (float)(y1 - y0);

	// Cast the floats to ints
	x2 = (int)x_rotated;
	y2 = (int)y_rotated;

        // If the new rotated pixel is within bounds copy the pixel from src to new spot in dest
        if ((x2 >= 0) &&
            (x2 < ImageCols) &&
            (y2 >= 0) &&
            (y2 < ImageRows)){
          /* Write the new pixel value */
          dstPtr[ImageCols * y2 + x2] = srcPtr[ImageCols * row + col];
        }
      }
    );
  });
}


int main() {
  // Create device selector for the device of your interest.
#if FPGA_EMULATOR
  // DPC++ extension: FPGA emulator selector on systems without FPGA card.
  ext::intel::fpga_emulator_selector d_selector;
#elif FPGA || FPGA_PROFILE
  // DPC++ extension: FPGA selector on systems with FPGA card.
  ext::intel::fpga_selector d_selector;
#else
  // The default device selector will select the most performant device.
  default_selector d_selector;
#endif

  float *hInputImage;
  float *hOutputImage;

  int imageRows;
  int imageCols;
  int i;

  // Calculations for rotations
  // https://www.cplusplus.com/reference/cmath/cos/ 
  int theta = -45;
  float cos_value, sin_value;

  cos_value = cos(theta * PI / 180.0);
  sin_value = sin(theta * PI / 180.0);


  /* Read in the BMP image */
  hInputImage = readBmpFloat(inputImagePath, &imageRows, &imageCols);
  printf("imageRows=%d, imageCols=%d\n", imageRows, imageCols);
  /* Allocate space for the output image */
  hOutputImage = (float *)malloc( imageRows*imageCols * sizeof(float) );
  for(i=0; i<imageRows*imageCols; i++)
    hOutputImage[i] = 1234.0;


  Timer t;

  try {
    queue q(d_selector, dpc_common::exception_handler);

    // Print out the device information used for the kernel code.
    std::cout << "Running on device: "
              << q.get_device().get_info<info::device::name>() << "\n";

    // Image rotation in DPC++
    ImageRot_v1(q, hInputImage, hOutputImage, cos_value, sin_value, imageRows, imageCols);
  } catch (exception const &e) {
    std::cout << "An exception is caught for image convolution.\n";
    std::terminate();
  }

  std::cout << t.elapsed().count() << " seconds\n";

  /* Save the output bmp */
  printf("Output image saved as: cat-rot.bmp\n");
  writeBmpFloat(hOutputImage, "cat-rot.bmp", imageRows, imageCols,
          inputImagePath);

  return 0;
}
