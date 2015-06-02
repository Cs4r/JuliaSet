#!/usr/bin/env bash

readonly MPIRUN=/usr/bin/mpirun
readonly NUMBER_OF_PROCESSES=4
readonly BINDIR=./bin
readonly OUTPUTDIR=./output
readonly RAW2GIF=/usr/bin/raw2gif
readonly PALETTE=vgaPalette.txt
readonly IMAGE_WIDTH=2048
readonly IMAGE_HEIGHT=2048

#TODO: check if mpicc, mpirun and raw2gif are installed in the system

make

if [ $? -ne 0 ]; then 
	echo "Error compiling sources. Exiting.." >&2
	exit 1
fi

for program in `ls $BINDIR`;do
	echo "Running $BINDIR/$program..."
	if [ $program == "sequential" ]; then
		 $BINDIR/$program
	else
		$MPIRUN -n $NUMBER_OF_PROCESSES $BINDIR/$program
	fi
done

for raw_output in `ls $OUTPUTDIR`;do
	gif_output=${raw_output/raw/gif}
	echo "Creating $OUTPUTDIR/$gif_output..."
	
	$RAW2GIF -s $IMAGE_WIDTH $IMAGE_HEIGHT -p $PALETTE $OUTPUTDIR/$raw_output > $OUTPUTDIR/$gif_output
	
	echo "Deleting $OUTPUTDIR/$raw_output..."
	rm $OUTPUTDIR/$raw_output
done

echo -e "Output images are ready. You can take a look the directory $OUTPUTDIR"
