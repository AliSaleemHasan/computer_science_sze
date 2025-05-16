#include "utils.c"

MPI_Datatype stats_type;
RNGState rng_state = {0}; 
double starting_price = 100.0;
double deltaT = 1.0/252.0;
double mu = 0.1;
double sigma = 0.2;

void create_stats_type(){
	int lengths[6] = {1, 1, 1, 1, 1, 1};
	MPI_Aint displacements[6];

	// Use a dummy instance to compute displacements
	Stats dummy;
	MPI_Aint base_address;
	MPI_Get_address(&dummy, &base_address);
	MPI_Get_address(&dummy.Day,       &displacements[0]);
	MPI_Get_address(&dummy.mean,      &displacements[1]);
	MPI_Get_address(&dummy.min,       &displacements[2]);
	MPI_Get_address(&dummy.max,       &displacements[3]);
	MPI_Get_address(&dummy.std_dev,   &displacements[4]);
	MPI_Get_address(&dummy.last_price,&displacements[5]);

	// Adjust displacements relative to the base address of the structure
	for (int i = 0; i < 6; i++)
		displacements[i] = MPI_Aint_diff(displacements[i], base_address);

	MPI_Datatype types[6] = { MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };
	MPI_Type_create_struct(6, lengths, displacements, types, &stats_type);
	MPI_Type_commit(&stats_type);


}

Stats NStepFuturePrice(int D, int H, int M, int iteration)
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

	return s;
}

double monteCarlo(InputArgs inputs,int rank,int max_record_size,char *filename)
{
	char *process_csv; 
	if(inputs.save_stats) process_csv = malloc(inputs.iterations * max_record_size); // allocate memore for stats string
	double avg = 0.0, payoff;
	int i, counter = 0;
	Stats s;
#pragma omp parallel if (inputs.parallel)
#pragma omp for schedule(runtime) firstprivate(inputs) private(payoff, s) reduction(+ : avg, counter)
	for (i = 0; i < inputs.iterations; i++)
	{
		s = NStepFuturePrice(inputs.Days, inputs.Hours, inputs.Minutes, i);
		payoff = calculatePayoff(s.last_price, inputs.strike_price, inputs.call_put);

		if(inputs.save_stats){
			char record[max_record_size + 25]; // more than max_record_size because \n and others might be added
			sprintf(record,"%d,%.2f,%.2f,%.2f,%.2f,%2f\n",i,s.mean, s.min, s.max, s.std_dev, s.last_price);
			strcat(process_csv, record);
		}

		if (isfinite(payoff))
		{
			avg += payoff;
			counter++;
		}
	}

	if(inputs.save_stats) mpi_save_stats(filename,process_csv,rank);
		return avg / counter;
}

int main(int argc, char **argv)
{
	MPI_Init(&argc,&argv);
	int size, rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
    char *stats_filename = "./data/hypred_stats.csv";
 	double start_time = MPI_Wtime();

	create_stats_type();

	seed_random();
    int max_record_size = 100;

	InputArgs inputs = process_input(argc, argv);

	//first, all processes has to work with a chunk of iterations
	int processIterations = inputs.iterations / size;

	if(rank + 1== size) processIterations += (inputs.iterations%size);
	inputs.iterations = processIterations;

	// after defining the required iterations, the processes should send the results back to rank 0 (reduction)
	double processPayoff = monteCarlo(inputs,rank,max_record_size,stats_filename),AllProcessesPayoff;
	MPI_Reduce(&processPayoff,&AllProcessesPayoff,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);


	double end_time = MPI_Wtime();
	if(rank ==0){
		printf("avg payoff is %f \n", AllProcessesPayoff/size);
		printf("time elapsed : %f  \n", end_time - start_time);
	}


	MPI_Finalize();

	return 0;
}
