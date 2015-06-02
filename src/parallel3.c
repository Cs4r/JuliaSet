#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

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
	register float zr, zi;
	register float cr, ci;
	register float zi_backup;
	register float zr_square, zi_square;
	register int row, column;
	register char color;
	double start_time, end_time;
	double total_time;

	// Variables need for parallel version
	int number_of_processes, process_id, remainder;
	register float x_increment_accumulated, y_increment_accumulated;
	register float x_increment_as_float = (float) X_INCREMENT,
			x_windows_as_float = (float) X_WINDOWS;
	register float y_increment_as_float = (float) Y_INCREMENT,
			y_windows_as_float = (float) Y_WINDOWS;
	char *local_image;
	char *julia_set_image;
	int pi_first_row, pi_last_row, local_image_size, current_pixel;
	int rows_per_process;
	int received_flag = 0, processes_which_finished_calculation = 0,
			received_elements;
	int maximum_number_of_rows, offset, maximum_offset;
	MPI_Request request;
	MPI_Status status;

	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
	MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);

	remainder = NUMBER_OF_ROWS % number_of_processes;
	rows_per_process = NUMBER_OF_ROWS / number_of_processes;
	pi_first_row = process_id * rows_per_process;

	if (process_id != number_of_processes - 1) {
		local_image_size = (rows_per_process + remainder) * NUMBER_OF_COLUMNS;
	} else {
		rows_per_process += remainder;
		local_image_size = rows_per_process * NUMBER_OF_COLUMNS;
	}

	local_image = (char*) malloc(sizeof(char) * local_image_size);
	pi_last_row = pi_first_row + rows_per_process;

	cr = -0.4;
	ci = 0.6;

	// Process 0 stores the whole image and measures time
	if (process_id == 0) {
		maximum_number_of_rows = (rows_per_process + remainder);
		julia_set_image = (char*) malloc(
				sizeof(char)
						* (maximum_number_of_rows * number_of_processes
								* NUMBER_OF_COLUMNS));

		start_time = MPI_Wtime();
	}

	current_pixel = 0;

	for (row = pi_first_row, y_increment_accumulated = row
			* y_increment_as_float; row < pi_last_row;
			++row, y_increment_accumulated += y_increment_as_float) {
		zi_backup = y_windows_as_float + y_increment_accumulated;
		for (column = 0, x_increment_accumulated = 0.0;
				column < NUMBER_OF_COLUMNS; ++column, x_increment_accumulated +=
						x_increment_as_float) {
			zr = x_windows_as_float + x_increment_accumulated;
			zr_square = zi_square = color = 0;
			zi = zi_backup;
			while (zr_square + zi_square < 4.0 && color < 256) {
				zr_square = zr * zr;
				zi_square = zi * zi;
				zi = 2 * zr * zi + ci;
				zr = zr_square - zi_square + cr;
				color++;
			}
			local_image[current_pixel++] = color - 1;
		}

		if (process_id == 0) {

			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
					&received_flag, &status);

			if (received_flag != 0) {

				MPI_Get_count(&status, MPI_CHAR, &received_elements);

				MPI_Irecv(
						(julia_set_image + status.MPI_SOURCE * received_elements),
						received_elements, MPI_CHAR, status.MPI_SOURCE,
						status.MPI_TAG, MPI_COMM_WORLD, &request);

				if (MPI_Wait(&request, &status) == MPI_SUCCESS) {
					processes_which_finished_calculation++;
				}

			}
		}
	}

	if (process_id != 0) {
		// Send local_image to process with process_id equal to 0
		MPI_Isend(local_image, local_image_size, MPI_CHAR, 0, rows_per_process,
				MPI_COMM_WORLD, &request);

		MPI_Wait(&request, &status);
	} else {
		// Processs with process_id equal to 0 gathers rest of processes local images
		while (processes_which_finished_calculation < number_of_processes - 1) {
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
					&received_flag, &status);

			if (received_flag != 0) {

				MPI_Get_count(&status, MPI_CHAR, &received_elements);

				MPI_Irecv(
						(julia_set_image + status.MPI_SOURCE * received_elements),
						received_elements, MPI_CHAR, status.MPI_SOURCE,
						status.MPI_TAG, MPI_COMM_WORLD, &request);

				if (MPI_Wait(&request, &status) == MPI_SUCCESS) {
					processes_which_finished_calculation++;
				}

			}
		}

		// Finally, copy process zero local image to final image
		offset = 0;
		for (row = 0; row < rows_per_process; ++row) {
			for (column = 0; column < NUMBER_OF_COLUMNS; ++column) {
				julia_set_image[offset + column] = local_image[offset + column];
			}
			offset += NUMBER_OF_COLUMNS;
		}

	}

	// Process 0 stores the whole image and measures time
	if (process_id == 0) {
		end_time = MPI_Wtime();

		FILE *output_file;

		output_file = fopen("./output/parallel3_output.raw", "wb");

		if (!output_file) {
			printf("Cannot open output file\n");
			exit(EXIT_FAILURE);
		}

		offset = 0;

		// Write only useful rows to the file
		for (row = 0; row < number_of_processes; ++row) {

			if (row != number_of_processes - 1) {
				maximum_offset = offset + rows_per_process;
			} else {
				maximum_offset = offset + maximum_number_of_rows;
			}

			for (column = offset; column < maximum_offset; ++column) {
				fwrite(&julia_set_image[column * NUMBER_OF_COLUMNS],
						sizeof(char),
						NUMBER_OF_COLUMNS, output_file);
			}

			offset += maximum_number_of_rows;
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
