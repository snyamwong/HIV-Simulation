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
 * incubates the petri dish given the petri dish and number of generations
 */
void incubatePetriDish(struct Pixel** petridish, struct Pixel** buffer, int size, int gen);

/*
 *  converts petri dish to PPM file
 */
void petriDishToPPM(struct Pixel** petriDish, int size, int gen);

/**
 *  prints petri dish
 */
void printPetriDish(struct Pixel** petriDish, int size);
