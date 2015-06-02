#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define NUMBER_OF_ROWS 2048 // Output image number of rows
#define NUMBER_OF_COLUMNS 2048 // Output image number of columns                   
#define X_WINDOWS     -0.5 // Space's bottom left corner
#define Y_WINDOWS     -0.5 // Space's top right corner
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
	float zr, zi;
	float cr, ci;
	float zi_backup;
	float zr_square, zi_square;
	int column, row;
	char color;
	double start_time, end_time;
	double total_time;

	// Variables needed for parallel version
	int number_of_processes, process_id, remainder;
	char *local_image;
	char *julia_set_image;
	int pi_first_row, pi_last_row, local_image_size;
	int rows_per_process;

	cr = -0.4;
	ci = 0.6;

	MPI_Init(NULL, NULL); // Initialize MPI environment
	MPI_Comm_rank(MPI_COMM_WORLD, &process_id); // Get process id
	MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes); // Get number of processes

	// This program allows to create a julia set even though the number of rows in the image
	// is not divisible by the number of processes, for doing that it scatters the number of
	// rows between processes giving extra rows to the last process
	remainder = NUMBER_OF_ROWS % number_of_processes;

	if (process_id != number_of_processes - 1) {
		rows_per_process = NUMBER_OF_ROWS / number_of_processes;
		pi_first_row = process_id * rows_per_process;
		local_image_size = (rows_per_process + remainder) * NUMBER_OF_COLUMNS;
	} else {
		rows_per_process = NUMBER_OF_ROWS / number_of_processes;
		pi_first_row = process_id * rows_per_process;
		rows_per_process += remainder;
		local_image_size = rows_per_process * NUMBER_OF_COLUMNS;
	}

	local_image = (char*) malloc(sizeof(char) * local_image_size);
	pi_last_row = pi_first_row + rows_per_process;

	// Process 0 stores the whole image and measures time
	if (process_id == 0) {
		julia_set_image = (char*) malloc(
		                      sizeof(char) * ((rows_per_process + remainder) * number_of_processes * NUMBER_OF_COLUMNS));

		start_time = MPI_Wtime();
	}

	for (row = pi_first_row; row < pi_last_row; ++row) {
		zi_backup = (float) Y_WINDOWS + (float) row * Y_INCREMENT;
		for (column = 0; column < NUMBER_OF_COLUMNS; ++column) {
			zi = zi_backup;
			zr = (float) X_WINDOWS + (float) column * X_INCREMENT;
			zr_square = zi_square = (float) 0;
			color = 0;
			while (zr_square + zi_square < (float) 4 && color < 256) {
				zr_square = zr * zr;
				zi_square = zi * zi;
				zi = 2 * zr * zi + ci;
				zr = zr_square - zi_square + cr;
				color++;

			}
			local_image[(row - pi_first_row) * NUMBER_OF_COLUMNS + column] = color - 1;
		}
	}

	// Build compound image with partial images
	MPI_Gather(local_image, local_image_size, MPI_CHAR, julia_set_image, local_image_size, MPI_CHAR, 0, MPI_COMM_WORLD);

	if (process_id == 0) {
		end_time = MPI_Wtime();

		FILE *output_file;

		output_file = fopen("./output/parallel2_output.raw", "wb");

		if (!output_file) {
			printf("Cannot open output file\n");
			exit(EXIT_FAILURE);
		}

		for (row = 0; row < number_of_processes; ++row) {
			if (row != number_of_processes - 1) {
				for (column = row * (rows_per_process + remainder); column < row * (rows_per_process + remainder) + rows_per_process; column++) {
					fwrite(&julia_set_image[column * NUMBER_OF_COLUMNS], sizeof(char), NUMBER_OF_COLUMNS, output_file);
				}
			} else {
				for (column = row * (rows_per_process + remainder); column < row * (rows_per_process + remainder) + rows_per_process + remainder; column++) {
					fwrite(&julia_set_image[column * NUMBER_OF_COLUMNS], sizeof(char), NUMBER_OF_COLUMNS, output_file);
				}
			}
		}

		fclose(output_file);

		free(julia_set_image);

		total_time = (end_time - start_time);

		fprintf(stderr, "Time spent by Julia set of %dx%d=%.16g milliseconds\n",
		        NUMBER_OF_ROWS, NUMBER_OF_COLUMNS, total_time);

	}

	free(local_image);
	MPI_Finalize();

	return EXIT_SUCCESS;
}
