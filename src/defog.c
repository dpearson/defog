/* Copyright 2014-2015 David Pearson.
 * All rights reserved.
 *
 * Compilation: gcc -o defog defog.c `pkg-config --libs --cflags opencv` -std=c99 -lm
 * Usage: ./defog RGB_IMAGE_FILE
 */

#include <stdio.h>

#include <cv.h>
#include <highgui.h>

// Color channel definitions for convenience
typedef enum {
	BLUE,
	GREEN,
	RED
} channel_t;

// Representation of a pixel in the dark channel used when computing the transmission map
typedef struct {
	int x;
	int y;
	double val;
	double intensity;
} channel_value_t;

// The width of the window used in finding the dark channel in the vicinity of a particular pixel
#define MAP_WIDTH 20

// Function definitions
int scalar_min(CvScalar sc, int num_vals);
double find_light_intensity(IplImage *img, IplImage *gray, int x1, int y1, int x2, int y2);
int find_dark_channel(IplImage *img, int x1, int y1, int x2, int y2);

/* Find the minimum value in a CvScalar
 *
 * sc - The scalar to find the minimum value of
 * num_vals - The number of values in the scalar, which must be >= 1
 *
 * Returns the index of the minimum value
 */
int scalar_min(CvScalar sc, int num_vals) {
	// Get all values
	double *vals = sc.val;

	// Initially assume that the first value is the minimum
	double min = vals[0];
	int min_in = 0;

	// Iterate through, swapping the minimum value and index if necessary
	for (int i = 1; i < num_vals; i++) {
		double val = vals[i];
		if (val < min) {
			min = val;
			min_in = i;
		}
	}

	return min_in;
}

/* Finds the light intensity of an area of an image
 *
 * img - The original BGR image
 * gray - The grayscaler version of the original BGR image
 * x1 - The left edge of the area, which will be searched
 * y1 - The upper edge of the area, which will be searched
 * x2 - The right edge of the area, which will *not* be searched
 * y2 - The lower edge of the area, which will *not* be searched
 *
 * Returns the intensity value for the atmospheric light in the image area
 */
double find_light_intensity(IplImage *img, IplImage *gray, int x1, int y1, int x2, int y2) {
	// Initialize an array for the top 0.1% of pixels
	int top_num = (x2 - x1) * (y2 - y1) * 0.001;
	channel_value_t *top = malloc(top_num * sizeof(channel_value_t));
	for (int i = 0; i < top_num; i++) {
		top[i].x = -1;
		top[i].y = -1;
		top[i].val = -1.0;
		top[i].intensity = -1.0;
	}

	// Get the dark channel of the area
	channel_t dark_channel = find_dark_channel(img, x1, y1, x2, y2);

	// Then iterate through the region
	for (int y = y1; y < y2; y++) {
		for (int x = x1; x < x2; x++) {
			// Find the dark channel value and the intensity
			double val = cvGet2D(img, y, x).val[dark_channel];
			double intensity = cvGet2D(gray, y, x).val[0];

			// Attempt to place the pixel in the array of the brightest pixels
			for (int i = 0; i < top_num; i++) {
				// Compare to the pixel in the current bucket, using intensity as a tiebreaker
				if (top[i].val < val || (top[i].val == val && top[i].intensity < intensity)) {
					// Set values
					top[i].x = x;
					top[i].y = y;
					top[i].val = val;
					top[i].intensity = intensity;

					// Then break out of the loop
					break;
				}
			}
		}
	}

	// Find the max intensity value in the array
	channel_value_t *max = malloc(sizeof(channel_value_t));
	max->intensity = -1.0;
	for (int i = 0; i < top_num; i++) {
		if (top[i].intensity > max->intensity) {
			memcpy(max, &top[i], sizeof(channel_value_t));
		}
	}

	// Grab the value to return
	double ret_val = max->intensity;

	// Clean up
	free(top);
	free(max);

	return ret_val;
}

/* Finds the dark channel in an area of an image
 *
 * img - The original BGR image
 * x1 - The left edge of the area, which will be searched
 * y1 - The upper edge of the area, which will be searched
 * x2 - The right edge of the area, which will *not* be searched
 * y2 - The lower edge of the area, which will *not* be searched
 *
 * Returns the scalar index of the dark channel in the image area
 */
int find_dark_channel(IplImage *img, int x1, int y1, int x2, int y2) {
	// Assume that the top left pixel has the minimum value temporarily
	CvScalar min_pixel = cvGet2D(img, y1, x1);

	// Get the minimum channel value for the top left pixel
	int min = scalar_min(min_pixel, 3);

	// Iterate through the area
	for (int y = y1; y < y2; y++) {
		for (int x = x1; x < x2; x++) {
			// Get a new pixel and minimum channel value
			CvScalar curr_pixel = cvGet2D(img, y, x);
			int curr_min = scalar_min(curr_pixel, 3);

			// Compare the current pixel to the minimum pixel
			if (curr_pixel.val[curr_min] < min_pixel.val[min]) {
				// Swap them if necessary
				min_pixel = curr_pixel;
				min = curr_min;
			}
		}
	}

	return min;
}

/* Calculates the number of high-frequency pixels in an image, which was used
 * as an evaluation metric for the defogging process.
 * In theory, a higher number of high-intensity pixels should correlate to a reduced
 * amount of fog.
 *
 * img - The BGR image to evaluate
 *
 * Returns the number of pixels in the real component of the
 * DFT result that are greater than 127
 */
int evaluate(IplImage *img) {
	// Get the size of the image
	CvSize size = cvGetSize(img);

	// Convert the color image to grayscale
	IplImage *gray_tmp = cvCreateImage(size, img->depth, 1);
	cvCvtColor(img, gray_tmp, CV_RGB2GRAY);

	// Create a new image at the required depth for the DFT (IPL_DEPTH_32F)
	IplImage *gray = cvCreateImage(size, IPL_DEPTH_32F, 1);

	// Then copy all pixel values
	for (int y = 0; y < size.height; y++) {
		for (int x = 0; x < size.width; x++) {
			cvSet2D(gray, y, x, cvGet2D(gray_tmp, y, x));
		}
	}

	// Free the original grayscale image
	cvReleaseImage(&gray_tmp);

	// Create a new image to hold the results of the DFT
	IplImage *real = cvCreateImage(size, IPL_DEPTH_32F, 1);
	//IplImage *cmpx = cvCreateImage(size, IPL_DEPTH_32F, 1);

	// And perform the transoformation
	cvDFT(gray, real, 32, 0);
	//cvDFT(gray, cmpx, 16, 0);

	// Perform thresholding to get high-intensity pixels from the DFT real results
	cvThreshold(real, real, 127, 255, CV_THRESH_BINARY);

	// Count nonzero pixels
	int nonzero = cvCountNonZero(real);

	// Clean up after ourselves
	cvReleaseImage(&gray);
	cvReleaseImage(&real);
	//cvReleaseImage(&cmpx);

	return nonzero;
}

int main(int argc, const char * argv[]) {
	// Read in the image to defog
	IplImage *img = (IplImage *)cvLoadImage(argv[1], CV_LOAD_IMAGE_COLOR);

	// Run an initial high-frequency pixel count
	printf("Number of high-frequency pixels in the original image: %d\n", evaluate(img));

	// Create a window for displaying input, output, and intermediary steps
	cvNamedWindow("disp", CV_WINDOW_AUTOSIZE);

	// Display the original image
	cvShowImage("disp", img);
	cvWaitKey(0);

	// Convert the input image to grayscale
	CvSize size = cvGetSize(img);
	IplImage *gray = cvCreateImage(size, img->depth, 1);
	cvCvtColor(img, gray, CV_RGB2GRAY);

	// Calculate the light intensity for the image
	double light_intensity = find_light_intensity(img, gray, 0, 0, size.width, size.height);

	// Create empty images for the transmission map and the output (defogged) image
	IplImage *map = cvCreateImage(size, img->depth, 1);
	IplImage *out = cvCreateImage(size, img->depth, img->nChannels);

	// Iterate through the input image
	for (int y = 0; y < size.height; y++) {
		for (int x = 0; x < size.width; x++) {
			// Calculate the bounds of the window used to determine the dark channel, falling
			// back on simply using the image bounds if the window can't fit
			int x_low = x - (MAP_WIDTH / 2) > 0 ? x - (MAP_WIDTH / 2) : 0;
			int y_low = y - (MAP_WIDTH / 2) > 0 ? y - (MAP_WIDTH / 2) : 0;
			int x_high = x + (MAP_WIDTH / 2) <= size.width ? x + (MAP_WIDTH / 2) : size.width;
			int y_high = y + (MAP_WIDTH / 2) <= size.height ? y + (MAP_WIDTH / 2) : size.height;

			// Find the dark channel for the window
			channel_t dark_channel = find_dark_channel(img, x_low, y_low, x_high, y_high);

			// Estimate t(x)
			double t = 1 - (cvGet2D(img, y, x).val[dark_channel] / light_intensity);

			// Then store it in the transmission map image
			cvSet2D(map, y, x, cvScalar(t * 255.0, 0.0, 0.0, 0.0));

			// Use the transmission map and light intensity to calculate channel values for the pixel in
			// the output image
			CvScalar out_pixel = cvScalarAll(0.0);
			for (int i = 0; i < out->nChannels; i++) {
				// Calculate the actual pixel value
				// The constant value was derived by attempting to maximize the evaluation metric
				out_pixel.val[i] = (cvGet2D(img, y, x).val[i] - light_intensity) / fmax(t, 0.54) + light_intensity;
			}

			// Set the calculated pixel value in the output image
			cvSet2D(out, y, x, out_pixel);
		}
	}

	// Show the map image
	cvSaveImage("map.png", map, 0);
	cvShowImage("disp", map);
	cvWaitKey(0);

	// Count the number of high-frequency pixels in the output image
	printf("Number of high-frequency pixels in the defogged image: %d\n", evaluate(out));

	// Then show the output image
	cvSaveImage("out.png", out, 0);
	cvShowImage("disp", out);
	cvWaitKey(0);

	// Release all images
	cvReleaseImage(&img);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);
	cvReleaseImage(&out);

	// Destroy the window used to display images
	cvDestroyAllWindows();

	return 0;
}
