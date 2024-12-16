#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <string.h>

/*
    ###################################################
    ###################################################
                        Ensure randomness
    ###################################################
    ###################################################
*/
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_usec / 1000000.0 + (double)tv.tv_sec;
}

void seed_random()
{
    unsigned int seed = getpid() ^ (unsigned int)time(NULL);
    srand(seed);
}

/*
    ###################################################
    ###################################################
                help calculate monte-carlo
    ###################################################
    ###################################################
*/

typedef struct
{
    int Day;
    double mean;
    double min;
    double max;
    double std_dev;
    double last_price;
} Stats;

// scaling down
#define HOURS_SCALAR 0.0248   // sqrt(1/1638) 252 days * 6.5 hours
#define MINUTES_SCALAR 0.0032 // sqrt(1/98280) 252 days * 6.5 hours * 60 minutes

// look at the period of creating random numbers
// calculate epsilon using standard normal distribuiton (epsilon) Marsaglia polar method
// double generateRandomVariable()
// {
//     double u1 = (double)rand() / RAND_MAX, u2 = (double)rand() / RAND_MAX;
//     double value = sqrtf((-2.0 * logf(u1))) * sinf(2 * M_PI * u2);
//     return value;
// }

double generateRandomVariable()
{
    double u1, u2, w, mult;
    static double x1, x2;
    static int call = 0;
    if (call == 1)
    {
        call = !call;
        return x2;
    }
    do
    {
        u1 = 2.0 * rand() / RAND_MAX - 1.0;
        u2 = 2.0 * rand() / RAND_MAX - 1.0;
        w = u1 * u1 + u2 * u2;
    } while (w >= 1.0 || w == 0);
    mult = sqrt((-2.0 * log(w)) / w);
    x1 = u1 * mult;
    x2 = u2 * mult;
    call = !call;
    return x1;
}
int counter = 0;
double getPriceatDeltaT(double deltaT, double s0, double mu, double sigma, char *resolution)
{

    double ep = generateRandomVariable();
    if (strcmp(resolution, "h") == 0)
    {
        mu *= HOURS_SCALAR;
        sigma *= HOURS_SCALAR;
        deltaT /= 6.5;
    }
    if (strcmp(resolution, "m") == 0)
    {
        mu *= MINUTES_SCALAR;
        sigma *= MINUTES_SCALAR;
        deltaT /= 6.5 * 60.0;
    }

    double expo = expf((mu - (sigma * sigma) / 2) * deltaT + sigma * sqrtf(deltaT) * ep);
    expo = fmin(fmax(expo, 0.2), 5.0);

    return s0 * expo;
}

void print_prices(int N, double *prices)
{
    int i;
    for (i = 0; i < N; i++)
    {
        printf("%f ", prices[i]);
        if ((i + 1) % 5 == 0)
            printf("\n");
    }
    printf("\n");
}

double calculatePayoff(double price, double strike_price, char action)
{

    if (action != 'c' && action != 'p')
    {
        printf("action should be 'c' or 'p' as in call or put!!");
        exit(-1);
    };

    if (action != 'c')
        // return fmax(strike_price - price, 0.0);
        return price - strike_price;
    // return fmax(price - strike_price, 0.0);
    return strike_price - price;
}

void write_to_csv(FILE *file, Stats stats, int day)
{
    fprintf(file, "%d,%.2f,%.2f,%.2f,%.2f\n", day, stats.mean, stats.min, stats.max, stats.std_dev);
}

/*
    ###################################################
    ###################################################
    this section is to handle statistics of monte-carlo
    ###################################################
    ###################################################
*/

void update_stats(double price, double *total_price, double *sum_of_squares, double *min_price, double *max_price, int *count)
{
    *total_price += price;
    *sum_of_squares += price * price;
    if (price < *min_price)
        *min_price = price;
    if (price > *max_price)
        *max_price = price;
    (*count)++;
}

// Calculates the final statistics
Stats calculate_final_stats(int iteration, double total_price, double sum_of_squares, double min_price, double max_price, int count, double last_price)
{
    double mean = total_price / count;
    double variance = (sum_of_squares / count) - (mean * mean);
    double std_dev = sqrt(variance);
    Stats stats = {iteration + 1, mean, min_price, max_price, std_dev, last_price};
    return stats;
}

double get_price_for_day(int H, int M, double s0, double deltaT, double mu, double sigma)
{

    for (int j = 0; j < H; j++)
    {
        for (int k = 0; k < M; k++)
            s0 = getPriceatDeltaT(deltaT, s0, mu, sigma, "m");
        s0 = getPriceatDeltaT(deltaT, s0, mu, sigma, "h");
    }

    s0 = getPriceatDeltaT(deltaT, s0, mu, sigma, "d");
    return s0;
}

/*
    ###################################################
    ###################################################
                        handle input
    ###################################################
    ###################################################
*/

typedef struct
{
    double strike_price;
    int iterations;
    int save_stats;
    int Days;
    int Hours;
    int Minutes;
    char call_put;
} InputArgs;

InputArgs process_input(int argc, char **argv)
{
    InputArgs inputs = {105.0, 100, 0, 252, 6, 60, 'c'};

    if (argc > 1 && argv[1] != NULL)
        inputs.iterations = atoi(argv[1]);
    if (argc > 2 && argv[2] != NULL)
        inputs.save_stats = atoi(argv[3]);
    if (argc > 3 && argv[3] != NULL)
        inputs.strike_price = atof(argv[3]);
    if (argc > 4 && argv[4] != NULL)
        inputs.Days = atoi(argv[4]);
    if (argc > 5 && argv[5] != NULL)
        inputs.Hours = atoi(argv[5]);
    if (argc > 6 && argv[6] != NULL)
        inputs.call_put = argv[6][0];

    return inputs;
}
