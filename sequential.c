#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define NUMBER_OF_ROWS 2048 // Output image number of rows
#define NUMBER_OF_COLUMNS 2048 // Output image number of columns                   
#define X_WINDOWS     -0.5 // Space bottom left corner
#define Y_WINDOWS     -0.5 // Space top right corner
#define X_WINDOWS_SIZE      1 // Windows width
#define Y_WINDOWS_SIZE      1 // Windows height
#define X_INCREMENT      X_WINDOWS_SIZE/NUMBER_OF_COLUMNS
#define Y_INCREMENT      Y_WINDOWS_SIZE/NUMBER_OF_ROWS

/*****************************************
 To create a julia set we need to resolve:
 f_c(z) = z^2 + c
 Where c is a complex constant value,
 z a complex variable and
 z0 = point's coordinates
 ******************************************/
int main(void) {
	float zr, zi; // z real and imaginary part
	float cr, ci; // c real and imaginary part
	float zi_backup;
	float zr_square, zi_square;
	int column, row;
	char julia_set_image[NUMBER_OF_ROWS][NUMBER_OF_COLUMNS];
	char color;
	FILE *output_file;
	double start_time, end_time, total_time;

	output_file = fopen("output.raw", "wb");

	if (!output_file) {
		printf("Cannot open output file\n");
		exit(EXIT_FAILURE);
	}

	/* Set the value of c for this specific julia set, real and imaginary part */
	cr = -0.4;
	ci = 0.6;

	start_time = MPI_Wtime();

	for (row = 0; row < NUMBER_OF_ROWS; row++) {
		zi_backup = (float) Y_WINDOWS + (float) row * Y_INCREMENT; // z0=point's coordinates (imaginary part).
		for (column = 0; column < NUMBER_OF_COLUMNS; column++) {
			zi = zi_backup;
			zr = (float) X_WINDOWS + (float) column * X_INCREMENT; // z0=point's coordinates (real part).
			zr_square = zi_square = (float) 0; // Initialize z squares: square of real part is zr_square, square of imaginary part is zi_square
			color = 0; // Each point will be painted depending on the number of iterations to escape
			while (zr_square + zi_square < (float) 4 && color < 256) {
				/* Calculate z^2 + c as zr^2-zi^2+2*zr*zi+c */
				zr_square = zr * zr; // zr^2
				zi_square = zi * zi; // zi^2
				zi = 2 * zr * zi + ci; // z imaginary part --> 2*zr*zi+ci.
				zr = zr_square - zi_square + cr; // z real part --> x^2-y^2+cr.
				color++; // Each point will be painted depending on the number of iterations to escape. If a point cannot escape it will be painted with black color
			}
			julia_set_image[row][column] = color - 1; // Set the color.
		}
	}

	end_time = MPI_Wtime();

	/* Write julia_set_image to the ouput file */
	for (row = 0; row < NUMBER_OF_ROWS; row++){
		fwrite(julia_set_image[row], sizeof(char), NUMBER_OF_COLUMNS, output_file);
	}

	fclose(output_file);

	total_time = end_time - start_time;

	fprintf(stderr, "Time spent by Julia set of %dx%d=%.16g milliseconds\n",
			NUMBER_OF_ROWS, NUMBER_OF_COLUMNS, total_time);

	return EXIT_SUCCESS;
}
