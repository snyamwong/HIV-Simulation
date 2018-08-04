#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "project.h"
#include "mpi.h"

int rank, numRank;
int countCellToCellInfection = 0, countCellFreeInfection = 0;

/*
 *  TODO: parallel
 *  TODO: figure out how to send petriDish with ghost rows
 *  TODO: calculate the occurences of infections by both cell free and cell to cell
 */
int main(int argc, char** argv)
{
    // setting a seed for the random number generator
    srand(time(NULL));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numRank);

    // make struct Pixel a datatype
    int nitems = 3;
    int blocklengths[3] = {255, 255, 255};
    MPI_Aint offsets[3];
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype MPI_PIXEL;

    offsets[0] = offsetof(struct Pixel, red);
    offsets[1] = offsetof(struct Pixel, green);
    offsets[2] = offsetof(struct Pixel, blue);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PIXEL);
    MPI_Type_commit(&MPI_PIXEL);

    int gen = atoi(argv[1]);
    int size = atoi(argv[2]);

    // allocate (dynamically) memory to the petriDish, and init all memory locations to be whitespace
    struct Pixel* petriDish;
    struct Pixel* checkBuffer;

    // have only rank 0 allocate memory for petri dish, then broadcast it to everyone
    if(rank == 0)
    {
        petriDish = allocatePetriDish(size);
        checkBuffer = allocatePetriDish(size);
    }

    MPI_Bcast(petriDish, size * size, MPI_PIXEL, 0, MPI_COMM_WORLD); 
    MPI_Bcast(checkBuffer, size * size, MPI_PIXEL, 0, MPI_COMM_WORLD);

    // populate the petri dish with cells or viruses randomly
    populatePetriDish(petriDish, size);
    populateBuffer(checkBuffer, size);

    // incubatePetriDish
    incubatePetriDish(petriDish, checkBuffer, gen, size);

    // apparently it's good practice to free memory even when your program is closing - so...
    free(petriDish);
    free(checkBuffer);

    MPI_Finalize();

    return 0;
}

struct Pixel* allocatePetriDish(int size)
{
    struct Pixel* petriDish = malloc(size * size * sizeof(struct Pixel));

    return petriDish;
}

void allocateSendcountsDisplacement(int* sendcounts, int* disp, int size)
{
    int mySize = size / numRank * size;

    sendcounts = malloc(numRank * sizeof(int));

    disp = malloc(numRank * sizeof(int));
    disp[0] = 0;

    for(int i = 0; i < numRank; i++)
    {
        sendcounts[i] = mySize;
    }
    
    int remainder = size % numRank * size;
    sendcounts[numRank - 1] += remainder;

    for(int i = 1; i < numRank; i++)
    {
        disp[i] = disp[i - 1] + sendcounts[i - 1];
    }
}

void populateBuffer(struct Pixel* buffer, int size)
{
    struct Pixel pixel = {255, 255, 255};

    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            buffer[i * size + j] = pixel;
        }
    }
}

void populatePetriDish(struct Pixel* petriDish, int size)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            // assume virus and cell both have 1% of spawning respectively, and 98% of empty area
            int randomPixel = rand() % 100;

            // default value for pixel (white)
            struct Pixel pixel = {255, 255, 255};

            // first, check if it is the border or not - if so, ignore
            if(isNotBorder(i, j, size))
            {
                // 96 means it's an infected cell by cell free
                if(randomPixel == 96)
                {
                    pixel = (struct Pixel) {0, 0, 255};
                }
                // 97 means it's an infected cell by cell to cell
                else if(randomPixel == 97)
                {
                    pixel = (struct Pixel) {255, 0, 255};
                }
                // 98 means it's a virus (red)
                else if(randomPixel == 98)
                {
                    pixel = (struct Pixel) {255, 0, 0};
                }
                // 99 means it's a cell (green)
                else if(randomPixel == 99)
                {
                    pixel = (struct Pixel) {0, 255, 0};
                }
            }

            petriDish[i * size + j] = pixel;
        }
    }
}

void incubatePetriDish(struct Pixel* petriDish, struct Pixel* checkBuffer, int gen, int size)
{
    // Gen 0 Print
    petriDishToPPM(petriDish, size, 0);

    // the portion sent to every processor
    int mySize = size / numRank * size;

    // range should always start at the beginning of a row and end at the end of a row
    // it should never stop midway, thus you wouldn't do (size * size) / numRank
    // +1 to mystart and -1 to myend is to ignore borders
    int range = (size / numRank);
    int mystart = rank * range + 1;
    int myend = mystart + range - 1;

    if(myend > size)
    {
        myend = size;
    }

    // Debug messages for range, mystart, myend
    printf("Range: %d\n", range);
    printf("mystart: %d\n", mystart);
    printf("myend: %d\n", myend);

    struct Pixel centerPixel;

    // iterates from 1 to gen
    for(int i = 1; i <= gen; i++)
    {
        // reset buffer to its original state
        populateBuffer(checkBuffer, size);

        // for this experiment, the edges are ignored and used as borders
        for(int x = mystart; x < myend; x++)
        {
            for(int y = 0; y < size; y++)
            {
                // set the center pixel using petri dish
                centerPixel = petriDish[x * size + y];

                // only check if centerPixel isn't free
                if(!isFree(centerPixel))
                {
                    // then give the center pixel to buffer
                    checkBuffer[x * size + y] = centerPixel;

                    // check the neighbors of the center pixel for infection
                    checkNeighbors(petriDish, checkBuffer, centerPixel, x, y, size);

                    // move pixel
                    movePixel(checkBuffer, size, x, y);
                } 
            }
        }

        // reducing in place with checkBuffer, MPI_SUM is okay because we don't allow collosion and the default value of checkBuffer is 0
        MPI_Allreduce(MPI_IN_PLACE, checkBuffer, size * size, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);

        // switch pointers between petriDish and checkBuffer and moveBuffer
        struct Pixel* temp = petriDish;
        petriDish = checkBuffer;
        checkBuffer = temp;

        // print petri dish to ppm after each gen
        petriDishToPPM(petriDish, size, i);
    }
}

void checkNeighbors(struct Pixel* petriDish, struct Pixel* checkBuffer, struct Pixel pixel, int x, int y, int size)
{
    struct Pixel neighbor;
    struct Pixel newPixel;

    for(int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++)
        {
            //neighbor = petriDish[x + i][y + i];
            neighbor = petriDish[(x + i) * size + (y + i)];

            // cell free infection (40% chance)
            if(pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 255 && neighbor.green == 0 && neighbor.blue == 0)
            {
                int chanceOfInfection = rand() % 100;

                // (40% chance)
                if(chanceOfInfection >= 40)
                {
                    printf("cell free infection at index x: %d y: %d\n", x, y);

                    // blue if it's a cell to virus infection
                    newPixel = (struct Pixel) {0, 0, 255};

                    checkBuffer[x * size + y] = newPixel;


                    countCellFreeInfection++;
                }
            }
            // cell to cell infection (60% chance)
            // can be infected by infected cell through cell free or infected cell through cell to cell
            else if((pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 0 && neighbor.green == 0 && neighbor.blue == 255)
                    ||
                    (pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 255 && neighbor.green == 0 && neighbor.blue == 255))
            {
                int chanceOfInfection = rand() % 100;

                // 60% chance
                if(chanceOfInfection >= 60)
                {
                    printf("cell to cell infection at index x: %d y: %d\n", x, y);

                    // magneta if it's a cell to cell infection
                    newPixel = (struct Pixel) {255, 0, 255};

                    checkBuffer[x * size + y] = newPixel;

                    countCellToCellInfection++;
                }
            }
        }
    }
}

void movePixel(struct Pixel* checkBuffer, int size, int x, int y)
{
    // generates from -1 ... 1, used for picking a random position for the cell to move to
    int i = rand() % 3 - 1;
    int j = rand() % 3 - 1;

    // check if the position the cell is moving to is the border
    if(isNotBorder(x + i, y + j, size) && i != 0 && j != 0)
    {
        struct Pixel neighbor = checkBuffer[(x + i) * size + (y + j)];

        // then check if the neighbor is free
        if(isFree(neighbor))
        {
            struct Pixel emptyPixel = (struct Pixel) {255, 255, 255};

            checkBuffer[(x + i) * size + (y + j)] = checkBuffer[x * size + y];
            checkBuffer[x * size + y] = emptyPixel;
        }
    }
}

int isFree(struct Pixel pixel)
{
    if(pixel.red == 255 && pixel.green == 255 && pixel.blue == 255)
    {
        return 1;
    }

    return 0;
}

int isNotBorder(int i, int j, int size)
{
    if (i != 0 && i != size - 1 && j != 0 && j != size - 1)
    {
        return 1;   
    }
 
    return 0;
}

void petriDishToPPM(struct Pixel* petriDish, int size, int gen)
{
    // TODO: make sure that each node/processor prints to a different ppm, this can be done by printf to a file based on the node's rank
    // format: P3 = rgb color in ASCII, width and height in pixels, 255 is the max value of each color

    // setting up filename, which will be in the directory results
    FILE* file;
    char* filename = malloc(45 * sizeof(char));
    sprintf(filename, "results/gen_%d_test.ppm", gen);

    // creates file or clears existing file
    file = fopen(filename, "w");

    fprintf(file, "P3\n%d %d\n255\n", size, size);     

    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            fprintf(file, "%d %d %d ", petriDish[i * size + j].red, petriDish[i * size + j].green, petriDish[i * size + j].blue);
        }

        fprintf(file, "\n");
    }

    // freeing memory and closing file
    free(filename);
    fclose(file);
}

void printPetriDish(struct Pixel* petriDish, int size)
{
    for(int i = 0; i < size; i++)
    {   
        for(int j = 0; j < size; j++)
        {
            printf("%d %d %d ", petriDish[i * size + j].red, petriDish[i * size + j].green, petriDish[i * size + j].blue);
        }

        printf("\n");
    }
}
