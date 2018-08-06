#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hiv_halo.h"
#include "mpi.h"
#include <math.h>
#include <omp.h>

/*
 *  TODO: parallel
 *  TODO: better memory management (i.e convert petri dish to 1D array)
 */

int rank, numranks;

int main(int argc, char** argv)
{

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // setting a seed for the random number generator
    srand(time(NULL));

    int gen = atoi(argv[1]);
    int size = atoi(argv[2]);

    // allocate (dynamically) memory to the petriDish, and init all memory locations to be whitespace
    struct Pixel** petriDish;
    struct Pixel** checkBuffer;

    petriDish = allocatePetriDish(size);
    checkBuffer = allocatePetriDish(size);

    // populate the petri dish with cells or viruses randomly
    populatePetriDish(petriDish, size);
    populateBuffer(checkBuffer, size);

    double start, end;

    // incubatePetriDish
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    incubatePetriDish(petriDish, checkBuffer, size, gen);

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    //free petriDish and checkBuffer
    for(int i = 0; i < size + 2; i++){
        free(petriDish[i]);
    }
    free(petriDish);

    for(int j = 0; j < size; j++){
        free(checkBuffer[j]);
    }
    free(checkBuffer);

    if(rank == 0){
        printf("Number of Nodes: %d Total Time: %f\n",numranks,end - start);
    }

    MPI_Finalize();

    return 0;
}

struct Pixel** allocatePetriDish(int size)
{
    struct Pixel** petriDish = malloc((size + 2) * sizeof(struct Pixel*));

    for(int i = 0; i < size + 2; i++)
    {
        petriDish[i] = malloc(size * sizeof(struct Pixel));
    }

    return petriDish;
}

void populateBuffer(struct Pixel** buffer, int size)
{
    struct Pixel pixel = {255, 255, 255};

    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            buffer[i][j] = pixel;
        }
    }
}

void populatePetriDish(struct Pixel** petriDish, int size)
{
    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < size + 2; i++)
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
                // 98 means it's a virus (red)
                if(randomPixel == 98)
                {
                    pixel = (struct Pixel) {255, 0, 0};
                }
                    // 99 means it's a cell (green)
                else if(randomPixel == 99)
                {
                    pixel = (struct Pixel) {0, 255, 0};
                }
            }

            if(i == 0) pixel = (struct Pixel) {0, 0, 0};
            if(i == size + 1) pixel = (struct Pixel) {0, 0, 0};

            petriDish[i][j] = pixel;
        }
    }
}

void incubatePetriDish(struct Pixel** petriDish, struct Pixel** checkBuffer, int size, int gen)
{
    //make struct Pixel a data type
    int nitems = 3;
    int blocklengths[3] = {1, 1, 1};
    MPI_Aint offsets[3];
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype MPI_PIXEL;

    offsets[0] = offsetof(struct Pixel, red);
    offsets[1] = offsetof(struct Pixel, green);
    offsets[2] = offsetof(struct Pixel, blue);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PIXEL);
    MPI_Type_commit(&MPI_PIXEL);

    // Gen 0 Print
    if(rank == 0){
        petriDishToPPM(petriDish, size, 0);
    }

    struct Pixel centerPixel;

    int next = rank + 1;
    int prev = rank - 1;
    if(next>numranks-1) next = MPI_PROC_NULL;
    if(prev<0) prev = MPI_PROC_NULL;

    // iterates from 1 to gen
    for(int i = 1; i <= gen; i++)
    {
        populateBuffer(checkBuffer, size);

        MPI_Sendrecv(petriDish[size],size,MPI_PIXEL,next,0,petriDish[0],size,MPI_PIXEL,prev,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Sendrecv(petriDish[1],size,MPI_PIXEL,prev,0,petriDish[size+1],size,MPI_PIXEL,next,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        // for this experiment, the edges are ignored and used as borders
        for(int x = 1; x < size - 1; x++)
        {
            for(int y = 1; y < size - 1; y++)
            {
                // set the center pixel using petri dish
                centerPixel = petriDish[x][y];

                // only check if centerPixel isn't free
                if(!isFree(centerPixel))
                {
                    // then give the center pixel to buffer
                    checkBuffer[x][y] = centerPixel;

                    // check the neighbors of the center pixel for infection
                    checkNeighbors(petriDish, checkBuffer, centerPixel, x, y);

                    // move pixel
                    movePixel(checkBuffer, size, x, y);
                }
            }
        }

        // switch pointers between petriDish and checkBuffer and moveBuffer
        struct Pixel** temp = petriDish;
        petriDish = checkBuffer;
        checkBuffer = temp;

        // print petri dish to ppm after each gen
        /*if(rank == 0){
           petriDishToPPM(petriDish, size, i);
      }*/
    }

}


void checkNeighbors(struct Pixel** petriDish, struct Pixel** checkBuffer, struct Pixel pixel, int x, int y)
{
    struct Pixel neighbor;
    struct Pixel newPixel;

    for(int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++)
        {
            neighbor = petriDish[x + i][y + j];

            // cell free infection (40% chance)
            if(pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 255 && neighbor.green == 0 && neighbor.blue == 0)
            {
                int chanceOfInfection = rand() % 100;

                // (40% chance)
                if(chanceOfInfection >= 40)
                {
                    //printf("cell free\n");

                    // blue if it's a cell to virus infection
                    newPixel = (struct Pixel) {0, 0, 255};

                    checkBuffer[x][y] = newPixel;
                }
            }
                // cell to cell infection (60% chance)
            else if(pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 0 && neighbor.green == 0 && neighbor.blue == 255)
            {
                int chanceOfInfection = rand() % 100;

                // 60% chance
                if(chanceOfInfection >= 60)
                {
                    //printf("cell to cell\n");

                    // magneta if it's a cell to cell infection
                    newPixel = (struct Pixel) {255, 0, 255};

                    checkBuffer[x][y] = newPixel;
                }
            }
        }
    }
}

void movePixel(struct Pixel** checkBuffer, int size, int x, int y)
{
    // generates from -1 ... 1, used for picking a random position for the cell to move to
    int i = rand() % 3 - 1;
    int j = rand() % 3 - 1;

    // check if the position the cell is moving to is the border
    if(isNotBorder(x + i, y + j, size) && i != 0 && j != 0)
    {
        struct Pixel neighbor = checkBuffer[x + i][y + j];

        // then check if the neighbor is free
        if(isFree(neighbor))
        {
            struct Pixel emptyPixel = (struct Pixel) {255, 255, 255};

            checkBuffer[x + i][y + j] = checkBuffer[x][y];
            checkBuffer[x][y] = emptyPixel;
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

void petriDishToPPM(struct Pixel** petriDish, int size, int gen)
{
    // TODO: make sure that each node/processor prints to a different ppm, this can be done by printf to a file based on the node's rank
    // format: P3 = rgb color in ASCII, width and height in pixels, 255 is the max value of each color

    // setting up filename, which will be in the directory results
    FILE* file;
    char* filename = malloc(45 * sizeof(char));
    sprintf(filename, "results/gen_%d_test.ppm",gen);

    // creates file or clears existing file
    file = fopen(filename, "w");

    fprintf(file, "P3\n%d %d\n255\n", size, size);

    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < size + 2; i++)
    {
        for(int j = 0; j < size; j++)
        {
            fprintf(file, "%d %d %d ", petriDish[i][j].red, petriDish[i][j].green, petriDish[i][j].blue);
        }

        fprintf(file, "\n");
    }

    fclose(file);
}

void printPetriDish(struct Pixel** petriDish, int size)
{
    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            printf("%d %d %d ", petriDish[i][j].red, petriDish[i][j].green, petriDish[i][j].blue);
        }

        printf("\n");
    }
}
