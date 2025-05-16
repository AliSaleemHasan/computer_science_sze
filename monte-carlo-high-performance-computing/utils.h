#pragma once
#include <float.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <omp.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <mpi.h>
// Structs
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

typedef struct
{
    uint64_t state;
    double spare;
    int has_spare;
} RNGState;

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

typedef struct
{
    double startin_price;
    double deltaT;
    double mu;
    double sigma;
} Parameters;

// Constants
#define HOURS_SCALAR 0.0248
#define MINUTES_SCALAR 0.0032

// Function declarations
double get_time();
void seed_random();
static void init_rng();
double generateRandomVariable();
double getPriceatDeltaT(double deltaT, double s0, double mu, double sigma, char *resolution);
void print_prices(int N, double *prices);
double calculatePayoff(double price, double strike_price, char action);
void write_to_csv(FILE *file, Stats stats, int day);
FILE *create_statistcs_file(int save_stats);
void update_stats(double price, double *total_price, double *sum_of_squares, double *min_price, double *max_price, int *count);
Stats calculate_final_stats(int iteration, double total_price, double sum_of_squares, double min_price, double max_price, int count, double last_price);
double get_price_for_day(int H, int M, double s0, double deltaT, double mu, double sigma);
void print_help();
InputArgs process_input(int argc, char **argv);

extern RNGState rng_state;
#pragma omp threadprivate(rng_state)

//**********************
// MPI helper functions
//**********************

int get_process_iterations(int iterations, int rank , int size);
void mpi_save_stats(char *filename, char *process_csv,int rank);


