#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include "common.h"
#include "float.h"

double deltaT = 1.0 / 252.0; // one day
double mu = 0.03;
double starting_price = 100.0;
double sigma = 0.5;

Stats NStepFuturePrice(int D, int H, int M, int interation)
{
    double s0 = 100.0, total_price = 0.0, sum_of_squares = 0.0;
    double deltaT = 1.0 / 252.0; // one day
    double mu = 0.03;
    double sigma = 0.5;
    double min_price = DBL_MAX, max_price = -DBL_MAX, last_price = 0.0;
    int count = 0;

    for (int i = 0; i < D; i++)
    {
        s0 = get_price_for_day(H, M, s0, deltaT, mu, sigma);
        update_stats(s0, &total_price, &sum_of_squares, &min_price, &max_price, &count);
        if (i + 1 == D)
            last_price = s0;
    }

    // printf("thread number %d just finished it's execution in %f \n", omp_get_thread_num(), end - start);
    // Final stats after all iterations
    Stats s = calculate_final_stats(0, total_price, sum_of_squares, min_price, max_price, count, last_price);

    return s;
}

double monteCarloParallized(InputArgs inputs)
{
    double all_avg = 0.0;
    int all_counter = 0;

    omp_set_num_threads(500);
    double payoff;
    Stats s;
#pragma omp parallel for schedule(static, 100) firstprivate(inputs) private(payoff, s) reduction(+ : all_avg, all_counter)
    for (int i = 0; i < inputs.iterations; i++)
    {

        s = NStepFuturePrice(inputs.Days, inputs.Hours, inputs.Minutes, i);

        // printf("Thread working on i = {%d} iteration is %d \n", i, omp_get_thread_num());

        payoff = calculatePayoff(s.last_price, inputs.strike_price, inputs.call_put);
        if (isfinite(payoff))
        {
            all_avg += payoff;
            all_counter++;
        }
    }

    return all_avg / all_counter;
}

int main(int argc, char **argv)
{
    seed_random();
    InputArgs inputs = process_input(argc, argv);
    double start = omp_get_wtime();
    printf("avg payoff is %f \n", monteCarloParallized(inputs));
    double end = omp_get_wtime();

    printf("time elapsed : %f  \n", end - start);
    return 0;
}
