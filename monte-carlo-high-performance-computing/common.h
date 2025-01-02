#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <string.h>
#include <getopt.h>


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
    unsigned int seed = getpid() ^ (unsigned int)time(NULL) ^ (unsigned int)clock();
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

typedef struct
{
   double starting_price;
   double mu;
   double sigma;
   double DeltaT;
} MonteCarloConfigs;



// scaling down
#define HOURS_SCALAR 0.0248   // sqrt(1/1638) 252 days * 6.5 hours
#define MINUTES_SCALAR 0.0032 // sqrt(1/98280) 252 days * 6.5 hours * 60 minutes

double generateRandomVariable()
{

    double u1, u2, w, mult;
    double x1, x2;
    do
    {
        u1 = 2.0 * rand() / RAND_MAX - 1.0;
        u2 = 2.0 * rand() / RAND_MAX - 1.0;
        w = u1 * u1 + u2 * u2;
    } while (w >= 1.0 || w == 0);
    mult = sqrt((-2.0 * log(w)) / w);
    x1 = u1 * mult;
    x2 = u2 * mult;
    return w > 0.5 ? x1 : x2;
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
    fprintf(file, "%d,%.2f,%.2f,%.2f,%.2f,%2f\n", day, stats.mean, stats.min, stats.max, stats.std_dev, stats.last_price);
}

/*
    ###################################################
    ###################################################
    this section is to handle statistics of monte-carlo
    ###################################################
    ###################################################
*/



FILE *create_statistcs_file(int save_stats)
{
    FILE *file = (save_stats) ? fopen("./data/stats.csv", "w") : NULL;
    if (file)
        fprintf(file, "Day,Mean,Min,Max,Std Dev,End Price\n");
    return file;
}

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
    int parallel;
    double strike_price;
    int iterations;
    int save_stats;
    int Days;
    int Hours;
    int Minutes;
    char call_put;
    double startin_price;
    double deltaT;
    double mu;
    double sigma;
} InputArgs;


typedef struct {

    double startin_price;
    double deltaT;
    double mu;
    double sigma;
} Parameters;

// Function to print help information
void print_help() {
    printf("Usage: program [options]\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message and exit\n");
    printf("  -i, --iterations NUM    Number of iterations (default: 1000)\n");
    printf("  -p, --parallel NUM      Number of parallel executions (default: 1)\n");
    printf("  -s, --save-stats NUM    Save stats (default: 0)\n");
    printf("  -m, --minutes DOUBLE    minutes in each  (default: 0.2)\n");
    printf("  -k, --strike-price NUM  Strike price (default: 105.0)\n");
    printf("  -d, --days NUM          Number of days (default: 252)\n");
    printf("  -H, --hours NUM         Number of hours (default: 6)\n");
    printf("  -c, --call-put CHAR     Call or put (default: 'c')\n");
    printf("  -e, --expected-return DOUBLE     Expected Return of monte-carlo  (default: 0.1)\n");
    printf("  -x, --initial-price DOUBLE     Starting price (default: 100.0)\n");
    printf("  -v, --mu DOUBLE     Exprcted Volatility (default: 0.2)\n");
}

InputArgs process_input(int argc, char **argv) {
    InputArgs inputs = {1, 105.0, 1000, 0, 252, 6, 60, 'c',100.0,(double)1.0/252.0,0.1,0.2};
    
    // Define the options
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"iterations", required_argument, 0, 'i'},
        {"parallel", required_argument, 0, 'p'},
        {"save-stats", required_argument, 0, 's'},
        {"strike-price", required_argument, 0, 'k'},
        {"days", required_argument, 0, 'd'},
        {"hours", required_argument, 0, 'H'},
        {"minutes", required_argument, 0, 'm'},
        {"call-put", required_argument, 0, 'c'},
        {"mu", required_argument, 0, 'm'},
        {"starting-pirce", required_argument, 0, 'x'},
        {"sigma", required_argument, 0, 'e'},

        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int opt;
    
    while ((opt = getopt_long(argc, argv, "hi:p:s:k:d:H:c:e:x:m:v:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                exit(0);
            case 'i':
                inputs.iterations = atoi(optarg);
                break;
            case 'p':
                inputs.parallel = atoi(optarg);
                break;
            case 's':
                inputs.save_stats = atoi(optarg);
                break;
            case 'k':
                inputs.strike_price = atof(optarg);
                break;
            case 'd':
                inputs.Days = atoi(optarg);
                break;
            case 'H':
                inputs.Hours = atoi(optarg);
                break;
            case 'c':
                inputs.call_put = optarg[0];
                break;
            case 'm':
                inputs.Minutes = atoi(optarg);
                break;
            case 'v':
                inputs.sigma = atof(optarg);
                break;
            case 'x':
                inputs.startin_price = atof(optarg);
                break;
            case 'e':
                inputs.mu=atof(optarg);
                break;
            default:
                print_help();
                exit(1);
        }
    }

    inputs.deltaT = (double) 1.0/inputs.Days;
    
    return inputs;
}