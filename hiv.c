#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "project.h"

/*
 *  TODO: parallel
 *  TODO: better memory management (i.e convert petri dish to 1D array)
 *  TODO: incubatePetriDish
 */
int main(int argc, char** argv)
{
    // setting a seed for the random number generator
    srand(time(NULL));

    int gen = atoi(argv[1]);
    int size = atoi(argv[2]);

    // allocate (dynamically) memory to the petriDish, and init all memory locations to be whitespace
    struct Pixel** petriDish;
   
    petriDish = allocatePetriDish(size);

    // populate the petri dish with cells or viruses randomly
    populatePetriDish(petriDish, size);

    // incubatePetriDish
    // incubatePetriDish(petriDish, size, gen);

    // print petri dish to ppm
    petriDishToPPM(petriDish, size);

    return 0;
}

struct Pixel** allocatePetriDish(int size)
{
    struct Pixel** petriDish = malloc(size * sizeof(struct Pixel*)); 

    for(int i = 0; i < size; i++)
    {
        petriDish[i] = malloc(size * sizeof(struct Pixel));
    }

    return petriDish;
}

void populatePetriDish(struct Pixel** petriDish, int size)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            // assume virus and cell both have 1% of spawning respectively, and 98% of empty area
            int randomPixel = rand() % 100;

            struct Pixel pixel = {255, 255, 255};

            // 8 means it's a virus (red)
            if(randomPixel == 98)
            {
                pixel = (struct Pixel) {255, 0, 0};
            }
            // 9 means it's a cell
            else if(randomPixel == 99)
            {
                pixel = (struct Pixel) {0, 0, 255};
            }

            petriDish[i][j] = pixel; 
        }
    }
}

void incubatePetriDish(struct Pixel** petriDish, int size, int gen)
{
    // iterates from 0 to gen - 1
    for(int i = 0; i < gen; i++)
    {
        // for this experiment, the edges are ignored and used as borders
        for(int x = 1; x < size - 1; x++)
        {
            for(int y = 1; y < size - 1; y++)
            {
                struct Pixel pixel = petriDish[x][y];
            }
        }           
    }
}

void petriDishToPPM(struct Pixel** petriDish, int size)
{
    // TODO: make sure that each node/processor prints to a different ppm, this can be done by printf to a file based on the node's rank

    // format: P3 = rgb color in ASCII, width and height in pixels, 255 is the max value of each color
    printf("P3\n%d %d\n255\n", size, size);     

    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            printf("%d %d %d ", petriDish[i][j].red, petriDish[i][j].blue, petriDish[i][j].green);
        }
    }
}

void printPetriDish(struct Pixel** petriDish, int size)
{
    for(int i = 0; i < size; i++)
    {   
        for(int j = 0; j < size; j++)
        {
            printf("%d %d %d ", petriDish[i][j].red, petriDish[i][j].blue, petriDish[i][j].green);
        }

        printf("\n");
    }
}
