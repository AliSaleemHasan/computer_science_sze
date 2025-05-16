#include "utils.c"

RNGState rng_state = {0}; 
double starting_price = 100.0;
double deltaT = 1.0/252.0;
double mu = 0.1;
double sigma = 0.2;

Stats NStepFuturePrice(int D, int H, int M, FILE *file, int iteration)
{
	double s0 = starting_price, total_price = 0.0, sum_of_squares = 0.0;
	double min_price = DBL_MAX, max_price = -DBL_MAX, last_price=0;
	int count = 0;

	for (int i = 0; i < D; i++)
	{
		s0 = get_price_for_day(H, M, s0, deltaT, mu, sigma);
		update_stats(s0, &total_price, &sum_of_squares, &min_price, &max_price, &count);
		if (i + 1 == D)
			last_price = s0;
	}

	// Final stats for one path (full trading year)
	Stats s = calculate_final_stats(iteration, total_price, sum_of_squares, min_price, max_price, count, last_price);
	if (file)
#pragma omp critical
		write_to_csv(file, s, iteration + 1);

	return s;
}

double monteCarlo(InputArgs inputs)
{
	FILE *file = create_statistcs_file(inputs.save_stats);
	double avg = 0.0, payoff;
	int i, counter = 0;
	Stats s;
#pragma omp parallel if (inputs.parallel)
#pragma omp for schedule(runtime) firstprivate(inputs) private(payoff, s) reduction(+ : avg, counter)
	for (i = 0; i < inputs.iterations; i++)
	{
		s = NStepFuturePrice(inputs.Days, inputs.Hours, inputs.Minutes, file, i);
		payoff = calculatePayoff(s.last_price, inputs.strike_price, inputs.call_put);

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
	InputArgs inputs = process_input(argc, argv);
	double start = get_time();
	printf("avg payoff is %f \n", monteCarlo(inputs));
	double end = get_time();
	printf("time elapsed : %f  \n", end - start);
	return 0;
}
