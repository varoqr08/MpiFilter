/*
	Basado en el código de Phil Gabardo.
	Se reutiliza la estructura, pero se cambia la funcionalidad de varias secciones
	tomado de: https://github.com/PhilGabardo/PPM-Filtering
*/

#include <stdlib.h>
#include "lib.h"
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>


int main(int argc, char** argv)
{
	// parsed image, and filtered image
	RGB *image, *outputImage;

	// process information
	int my_rank, p;

	// loop variables
	int dest, source;
	int offset;

	// image attributes
	int global_width, global_height;
	int width, height, max;
	Dimension *dimensions;


	// MPI boilerplate
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	int tag = 0;
	int windowSize = atoi(argv[3]);

	char convert_in[100];
	char convert_out[100];


	// Process 0 parses the PPM file and distributes size attributes
	if (my_rank == 0) {

		sprintf(convert_in, "convert %s tmp_in.ppm", argv[1]);
		system(convert_in);
		system("convert tmp_in.ppm -compress none tmp_in.ppm");

		image = readPPM("tmp_in.ppm", &global_width, &global_height, &max);
		offset = 0;

		for (dest = 1; dest < p; dest++) {
			int rows = global_height/p;
			if (dest < global_height % p)
				rows++;
			offset += rows;

			// Need windowSize/2 (or 2*windowSize/2) slack to make sure that the borders are computed correctly
			if (dest == p-1) {
				rows += (windowSize/2);
			}
			else {
				rows += (windowSize/2)*2;
			}
			Dimension *dim = (Dimension*)malloc(sizeof(Dimension));
			dim->width = global_width;
			dim->height = rows;
			MPI_Send(dim, 2, MPI_INT, dest, tag, MPI_COMM_WORLD);
		}

		// Set own size attributes
		width = global_width;
		height = global_height / p;
		if (global_height % p != 0) {
			height += 1;
		}
		height += (windowSize/2);
	}

	// Receive size attributes
	else {
		dimensions = (Dimension*)malloc(sizeof(Dimension));
		MPI_Recv(dimensions, 2, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
		width = dimensions->width;
		height = dimensions->height;
		image = (RGB*)malloc(height*width*sizeof(RGB));
	}


	// Process 0 distributes rows
	if (my_rank == 0) {
		offset = global_height / p;
		if (global_height % p != 0) {
			offset += 1;
		}
		offset += (windowSize/2);
		for (dest = 1; dest < p; dest++) {
			int rows = global_height/p;
			if (dest < global_height % p)
				rows++;

			if (dest == p-1) {
				rows += (windowSize/2);
			}
			else {
				rows += (windowSize/2)*2;
			}

			if (dest == p-1) {
				offset -= (windowSize/2)*2;
			}
			else {
				offset -= (windowSize/2)*2;
			}


			MPI_Send(image + offset*width, 3*rows*width, MPI_UNSIGNED_CHAR, dest, tag, MPI_COMM_WORLD);
			offset += rows;
		}
	}

	// Receive rows
	else {

		MPI_Recv(image, height*width*3, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &status);
	}


	// Process image	
	outputImage = processImage(width, height, image, windowSize, argv[4]);


	// Send processed data back to P0
	if (my_rank != 0 && my_rank != p-1) {
		MPI_Send(image + (windowSize/2)*width, (height-2*(windowSize/2))*width*3, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
	}
	else if (my_rank == p-1) {
		MPI_Send(image + (windowSize/2)*width, (height-(windowSize/2))*width*3, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
	}

	else {
		offset = global_height / p;
		if (global_height % p != 0) {
			offset += 1;
		}
		for (source = 1; source < p; source++) {
			int rows = global_height/p;
			if (source < global_height % p)
				rows++;

			MPI_Recv(image+offset*width, rows*width*3, MPI_UNSIGNED_CHAR, source, tag, MPI_COMM_WORLD, &status);

			offset += rows;
		}

	}

	// Process 0 writes processed PPM file
	if (my_rank == 0) {
		writePPM("tmp_out.ppm", global_width, global_height, max, outputImage);
		free(image);

		sprintf(convert_out, "convert tmp_out.ppm %s", argv[2]);
		system(convert_out);
		system("rm tmp_*");

	}

	MPI_Finalize();
	return(0);
}
