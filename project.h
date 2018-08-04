/*
 *  struct for Pixel, contains rgb value for each pixel on a PPM
 *
 *  TODO have hold a value (like char or something) to represent if it's a cell, virus, infected cell (cell free), infected cell (cell to cell)
 *  for easier programming experience
 */
struct Pixel
{
    int red;
    int green;
    int blue;
};

/**
 *  allocates memory location for a petri dish
 */
struct Pixel** allocatePetriDish(int size);

/**
 *  populates the petri dish with healthy cells and viruses
 */
void populatePetriDish(struct Pixel** petriDish, int size);

/**
 *  populates the buffer with empty
 */
void populateBuffer(struct Pixel** buffer, int size);

/**
 *  incubates the petri dish given the petri dish and number of generations
 */
void incubatePetriDish(struct Pixel** petridish, struct Pixel** buffer, int size, int gen);

/**
 *  checks if the neighbors of the cell are either infected or a virus
 */
void checkNeighbors(struct Pixel** petriDish, struct Pixel** buffer, struct Pixel pixel, int x, int y);

/*
 *  moves the pixel to a free neighbor
 */
void movePixel(struct Pixel** buffer, int size, int x, int y);

/*
 *  converts petri dish to PPM file
 */
void petriDishToPPM(struct Pixel** petriDish, int size, int gen);

/**
 *  prints petri dish
 */
void printPetriDish(struct Pixel** petriDish, int size);

/**
 *  checks if the pixel is free
 */
int isFree(struct Pixel pixel);

/*
 *  checks if the given indices is part of the border
 */
int isNotBorder(int i, int j, int size);



