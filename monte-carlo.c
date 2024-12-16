#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "string.h"
#include "common.h"
#include "float.h"
#include "math.h"

double deltaT = 1.0 / 252.0; // one day
double mu = 0.03;
double starting_price = 100.0;
double sigma = 0.5;

Stats NStepFuturePrice(int D, int H, int M, FILE *file, int iteration, int save_stats)
{
    double s0 = starting_price, total_price = 0.0, sum_of_squares = 0.0;
    double min_price = DBL_MAX, max_price = -DBL_MAX, last_price;
    int count = 0;

    double start = get_time();
    for (int i = 0; i < D; i++)
    {
        s0 = get_price_for_day(H, M, s0, deltaT, mu, sigma);
        update_stats(s0, &total_price, &sum_of_squares, &min_price, &max_price, &count);
        if (i + 1 == D)
            last_price = s0;
    }
    double end = get_time();

    printf("it took %f time for this iteration to finish \n", end - start);

    // Final stats after all iterations
    Stats s = calculate_final_stats(iteration, total_price, sum_of_squares, min_price, max_price, count, last_price);
    if (save_stats == 1)
        write_to_csv(file, s, iteration + 1);

    return s;
}

double monteCarlo(int D, int H, int M, int iterations, char action, int strike_price, int save_stats)
{
    FILE *file = fopen("stats.csv", "w");
    if (!file)
    {
        perror("Failed to open file");
        return 1;
    }

    fprintf(file, "Day,Mean,Min,Max,Std Dev,End Price\n");

    double avg = 0.0, payoff = 0.0, counter = 0.0;
    int i;
    Stats s;
    for (i = 0; i < iterations; i++)
    {
        printf("iteration number %d\n", i);
        s = NStepFuturePrice(D, H, M, file, i, save_stats);
        payoff = calculatePayoff(s.last_price, strike_price, action);

        if (isfinite(payoff))
        {
            avg += payoff;
            counter++;
        }
    }

    return avg / counter;
}

int main(int argc, char **argv)
{

    seed_random();
    InputArgs args = process_input(argc, argv);

    double start = get_time();
    printf("avg payoff is %f \n", monteCarlo(args.Days, args.Hours, args.Minutes, args.iterations, args.call_put, args.strike_price, 1));
    double end = get_time();

    printf("time elapsed : %f  \n", end - start);

    return 0;
}