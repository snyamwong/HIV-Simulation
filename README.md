# HIV_Simulation

To run the program, compile using gcc (or mpicc depending at the stage you're in).

## Instructions

The command will be ./ (your executable program name) (number of generation) (size of the petri dish)

The PPM files will be outputted into a directory called results. Therefore, before you run the code, make sure you make a directory called results.

## Example

mkdir results

./a.out 1 100

The program will run with 1 generation, and a size of 100 x 100 petri dish.

## Note

The print method (printPetriDishToPPM) will be automatically disabled, since PPM files are quite enourmous. Uncomment the method if you wish to see the output of the program.

For hiv_bcast, uncomment line 215.

For hiv_halo, uncomment line 195 - 197.
