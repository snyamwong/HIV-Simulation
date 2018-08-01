#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "project.h"

int isNotBorder(int i, int j, int size);
void checkNeighbors(struct Pixel** petriDish, struct Pixel** buffer, struct Pixel pixel, int x, int y);
void initBuffer(struct Pixel** buffer, int size);

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
    struct Pixel** buffer;

    petriDish = allocatePetriDish(size);
    buffer = allocatePetriDish(size);

    // populate the petri dish with cells or viruses randomly
    populatePetriDish(petriDish, size);
    initBuffer(buffer, size);

    // Gen 0 Print
    petriDishToPPM(petriDish, size, 0);

    // incubatePetriDish
    incubatePetriDish(petriDish, buffer, size, gen);

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

void initBuffer(struct Pixel** buffer, int size)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            struct Pixel pixel = {255, 255, 255};

            buffer[i][j] = pixel;
        }
    }
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

            // 98 means it's a virus (red)
            if(isNotBorder(i, j, size) && randomPixel == 98)
            {
                pixel = (struct Pixel) {255, 0, 0};
            }
            // 99 means it's a cell (green)
            else if(isNotBorder(i, j, size) && randomPixel == 99)
            {
                pixel = (struct Pixel) {0, 255, 0};
            }

            petriDish[i][j] = pixel; 
        }
    }
}

int isNotBorder(int i, int j, int size)
{
    if (i != 0 && i != size - 1 && j != 0 && j != size - 1)
    {
        return 1;   
    }
 
    return 0;
}

void incubatePetriDish(struct Pixel** petriDish, struct Pixel** buffer, int size, int gen)
{
    // iterates from 1 to gen
    for(int i = 1; i <= gen; i++)
    {
        // for this experiment, the edges are ignored and used as borders
        for(int x = 1; x < size - 1; x++)
        {
            for(int y = 1; y < size - 1; y++)
            {
                struct Pixel centerPixel = petriDish[x][y];

                checkNeighbors(petriDish, buffer, centerPixel, x, y);

                // checkNeighbors, then move
            }
        }

        // switch pointers between petriDish and buffer
        struct Pixel** temp = petriDish;
        petriDish = buffer;
        buffer = temp;

        // print petri dish to ppm
        petriDishToPPM(petriDish, size, i);
    }
}

void checkNeighbors(struct Pixel** petriDish, struct Pixel** buffer, struct Pixel pixel, int x, int y)
{
    buffer[x][y] = pixel;

    struct Pixel neighbor;
    struct Pixel newPixel;

    for(int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++)
        {
            neighbor = petriDish[x + i][y + i];

            // cell to virus infection (40% chance)
            if(pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 255 && neighbor.blue == 0 && neighbor.green == 0)
            {
                int chanceOfInfection = rand() % 100;

                if(chanceOfInfection >= 40)
                {
                    // blue if it's a cell to virus infection
                    newPixel = (struct Pixel) {0, 255, 0};

                    buffer[x][y] = newPixel;
                }
            }
            // cell to cell infection (60% chance)
            else if(pixel.red == 0 && pixel.green == 255 && pixel.blue == 0 && neighbor.red == 255 && neighbor.blue == 0 && neighbor.green == 255)
            {
                int chanceOfInfection = rand() % 100;

                if(chanceOfInfection >= 60)
                {
                    // magneta if it's a cell to cell infection
                    newPixel = (struct Pixel) {255, 0, 255};

                    buffer[x][y] = newPixel;
                }
            }
        }
    }
}

void petriDishToPPM(struct Pixel** petriDish, int size, int gen)
{
    // TODO: make sure that each node/processor prints to a different ppm, this can be done by printf to a file based on the node's rank
    // format: P3 = rgb color in ASCII, width and height in pixels, 255 is the max value of each color

    FILE* file;
    char* filename = malloc(20 * sizeof(char));
    sprintf(filename, "gen_%d_test.ppm", gen);

    file = fopen(filename, "w");

    fprintf(file, "P3\n%d %d\n255\n", size, size);     

    for(int i = 0; i < size; i++)
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
    for(int i = 0; i < size; i++)
    {   
        for(int j = 0; j < size; j++)
        {
            printf("%d %d %d ", petriDish[i][j].red, petriDish[i][j].green, petriDish[i][j].blue);
        }

        printf("\n");
    }
}
