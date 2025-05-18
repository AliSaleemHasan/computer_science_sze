import pandas as pd 
import matplotlib.pyplot as plt
import subprocess
import re
import os
import argparse


numerical_regix = r"\d+\.\d+"
compiled_file= "./monte-carlo"
compiled_hypred_file = "./hypred"
def get_schedule_stats(filepath):
    try:
        df_rows=[]
        for schedule in ['static','dynamic','guided']:
          for chunk_size in [1,5,10,25,50,100,150,200,250,500]:
                os.environ["OMP_SCHEDULE"] = f'{schedule},{chunk_size}'
                ans = subprocess.check_output([compiled_file,'-i','3000'], text=True)
                time = re.findall(numerical_regix, ans)[1]
                print(f"time elapsed for 3000 iteration with '{schedule},{chunk_size}' schedule is ${time}\n")
                df_rows.append({'schedule':schedule,'chunk':chunk_size,'time':time})
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath,index=False)

    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")


# define the best number of processes and the best number of threads for 10000 iterations
def mpi_weak_scaling(filepath):
    try:
        df_rows=[]
        os.environ['OMP_SCHEDULE']='static,5'
        for c in range(2,17,2):
            os.environ['OMP_NUM_THREADS']=str(c)
            for n in range(1,17,2):
                ans_mpi = subprocess.check_output(['mpirun','-n',str(n),compiled_hypred_file,'-i','10000'],text=True)
                time_elapsed = re.findall(numerical_regix,ans_mpi)[1]
                df_rows.append({'c':c,'n':n,'time':time_elapsed})
                print(f"time elapsed for hypred monte-carlo with c={c} and n={n} is {time_elapsed}s")
        df= pd.DataFrame(df_rows)
        df.to_csv(filepath)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")
        




def get_speedup_stats(filepath):
    try:
        df_rows=[]
        os.environ['OMP_SCHEDULE']='static,5'
        os.environ["OMP_NUM_THREADS"] = '6'
        for iteration in range(400,10100,200):
            ans_serial = subprocess.check_output([compiled_file,'-i', str(iteration),'-p','0'], text=True)
            ans_openmp = subprocess.check_output([compiled_file,'-i',str(iteration)], text=True)
        # adding the speedup for mpi with multiple number of processes
            iterations_obj = {'iteration':iteration}
            for n_process in range(2,16,2):
                ans_mpi = subprocess.check_output(['mpirun' ,'-n',str(n_process), compiled_hypred_file,'-i',str(iteration)],text=True)
                print(f"time elapsed for mpi_{n_process} is {re.findall(numerical_regix,ans_mpi)[1]}")
                iterations_obj[f'time_mpi_{n_process}n']=re.findall(numerical_regix,ans_mpi)[1]
            time_serial = re.findall(numerical_regix, ans_serial)[1]
            iterations_obj['time_serial'] = time_serial
            time_openmp = re.findall(numerical_regix, ans_openmp)[1]
            iterations_obj['time_openmp'] = time_openmp
            print(f"time elapsed for {iteration} iterations for: serial is {time_serial} and for openmp is {time_openmp}\n")
            df_rows.append(iterations_obj)
            print(iterations_obj)
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath,index=False)

    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")





def plot_time_and_speedup(file_name,vs_info):
    df = pd.read_csv(file_name)
    for info in vs_info:
        df[info['label']] = df[info['base']] / df[info['name']]
        fig, ax1 = plt.subplots(figsize=(10, 5))
        ax1.bar(df['iteration'] +10, df[info['base']], width=20, label='Time Serial', alpha=0.7, color='purple')
        ax1.bar(df['iteration']-10 , df[info['name']], width=20, label=info['label'], alpha=0.7, color='blue')
        ax1.set_xlabel('Iterations')
        ax1.set_ylabel('Time (seconds)')
        ax1.set_title(f'Time and Speedup for Different Iterations _{info["label"]}')
        ax1.legend(loc='upper left')
        ax1.grid(True)
        
        ax2 = ax1.twinx()
        ax2.plot(df['iteration'], df[info['label']], label={info['label']}, color='deeppink', marker='o', linestyle='-', linewidth=2)
        ax2.set_ylabel('Speedup')
        ax2.legend(loc='upper right')

        plt.savefig(info['image'])

    
  

def plot_weak_scaling(filepath,imagepath):
    """
    Reads benchmark data from a CSV file and plots time vs. number of processes (n)
    for different thread counts (c).

    Parameters:
    filepath (str): Path to the input CSV file.
    imagepath (str): Path to save the output plot.
    """
    # Load the data
    df = pd.read_csv(filepath)

    # Create a plot
    plt.figure(figsize=(12, 6))

    # Get unique values of 'c' (number of threads)
    unique_threads = sorted(df['c'].unique())

    # Plot each thread count separately
    for thread in unique_threads:
        subset = df[df['c'] == thread]  # Filter data for a specific thread count
        plt.plot(subset['n'], subset['time'], marker='o', linestyle='-', label=f'Threads {thread}')

    # Set labels and title
    plt.xlabel("# Processes (n)")
    plt.ylabel("Time Elapsed (s)")
    plt.yscale('log')
    plt.title("Weak Scaling: Time vs. # Processes for Different Thread Counts")
    plt.legend()
    plt.grid(True, which="both", linestyle="--")

    # Save and show the plot
    plt.savefig(imagepath)
    



def get_threads_stats(filepath,iterations):
    try:
        df_rows = []
        os.environ['OMP_SCHEDULE']='static,5'
        for iteration in iterations:  
            os.environ["OMP_NUM_THREADS"] = str(iteration)
            print(f"Running with {iteration} threads...")
            ans = subprocess.check_output([compiled_file,'-i','3000'], text=True)
            time = re.findall(numerical_regix, ans)[1]
            df_rows.append({'Thread': iteration, 'Time': float(time)})
            print(f"time:{time}")
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath, index=False)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")

def get_iterations_stats(filepath, iterations):
    try:
        df_rows = []
        os.environ['OMP_SCHEDULE']='static,5'
        os.environ["OMP_NUM_THREADS"] = '6'
        for iteration in iterations:  
            print(f"Running {iteration} iterations...")
            ans = subprocess.check_output([compiled_file,'-i',str(iteration)], text=True)
            numericals = re.findall(numerical_regix, ans)
            df_rows.append({'Iteration': iteration, 'Payoff Avg': float(numericals[0])})
            print(f"avg:{numericals[0]} , time:{numericals[1]}")
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath, index=False)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")


def analyze_shcdules(data_path,image):
    df = pd.read_csv(data_path)
    print(df.info())
    guided = df[df['schedule']=='guided']
    static = df[df['schedule']=='static']
    dynamic = df[df['schedule']=='dynamic']
    plt.figure(figsize=(12, 6))
    plt.semilogy( guided['chunk'],guided['time'],  marker='o', color='purple',label='guided')
    plt.semilogy( dynamic['chunk'],dynamic['time'], marker='o', color='blue',label='dynamic')
    plt.semilogy( static['chunk'],static['time'], marker='o', color='deeppink',label='static')
    plt.xlabel('Chunk Size')
    plt.xticks(df['chunk'], rotation=90)
    plt.ylabel('Time Elapsed')
    plt.title("Schdules Performance")
    plt.grid(True)
    plt.legend()
    plt.savefig(image)
    plt.show()

def analyze_data(df, x, y, ylabel, xlabel, image, horizontal_func='mean'):
    fun = df[y].mean() if horizontal_func == 'mean' else df[y].min()
    print(df.info())
    plt.figure(figsize=(10, 6))
    plt.plot(df[x], df[y], marker='o', linestyle='-')
    plt.xlabel(xlabel)
    plt.xticks(df[x], rotation=60)
    plt.ylabel(ylabel)
    plt.title(xlabel)
    plt.grid(True)
    plt.axhline(y=fun, color='r', linestyle='--', label=f'{horizontal_func} Value')
    plt.legend()
    plt.savefig(image)
    plt.show()


def plot_box_statistics(df, output_image='./plots/box_plot_statistics.png'):
    plt.figure(figsize=(12, 6))
    plt.boxplot([df['mean'], df['min'], df['max'], df['std_dev']], labels=['Mean', 'min', 'max', 'std_dev'])
    plt.title('Box Plot of Statistics')
    plt.ylabel('Values')
    plt.grid(True)
    plt.savefig(output_image)
    plt.show()

def plot_end_price_over_days(df, output_image='./plots/end_price_over_days.png'):
    plt.figure(figsize=(10, 6))
    plt.scatter(df['Day'], df['last_price'], marker='o')
    plt.xlabel('Day')
    plt.ylabel('End Price')
    plt.title('End Price over Days')
    plt.grid(True)
    plt.savefig(output_image)
    plt.show()

def parse_args():
    parser = argparse.ArgumentParser(description="Monte Carlo Analysis")
    parser.add_argument('--iterations-stats', action='store_true', help="Save iterations average data")
    parser.add_argument("--hypred-stats",action='store_true',help="Save Stats for hypred (mpi with openmp) for 10000 iterations with different cpus and processes")
    parser.add_argument('--threads-stats', action='store_true', help="Save threads statistics data")
    parser.add_argument('--schedule-stats', action='store_true', help="Save schdules statistics data")
    parser.add_argument('--vs-stats', action='store_true', help="Save serial vs parallel statistics data")
    parser.add_argument('--analyze', action='store', type=str, help="Analyze data from CSV file")
    parser.add_argument('--plot-type', type=str, choices=[ 'box', 'scatter'], help="Type of plot to generate")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    schdules_plot_name='./plots/schdules.png'
    vs_plot_name='./plots/speedup/vs.png'
    iterations_avg_file = "./data/iterations-avg.csv"
    threads_stats_file = "./data/threads.csv"
    schedule_stats_file = "./data/schedule.csv"
    vs_stats_file = "./data/vs.csv"
    hypred_stats_file="./data/hypred_stats.csv"
    weak_scaling_image = "./plots/weak_scaling.png"
    vs_info =[{'base':"time_serial",'name':"time_openmp",'image':"./plots/speedup/openmp_speedup.png",'label':"openmp speedup"},{'base':"time_serial",'label':"hypred speedup 8 n",'name':"time_mpi_8n",'image':"./plots/speedup/mpi_8n_speedup.png"},{'base':"time_openmp",'label':"hypred_speedup_over_openmp",'name':"time_mpi_8n",'image':"./plots/speedup/mpi_8n_speedup_over_openmp.png"}]


    if args.schedule_stats:
        get_schedule_stats(schedule_stats_file)
    if args.iterations_stats:
        get_iterations_stats(iterations_avg_file, range(200, 10200, 200))
    if args.threads_stats:
        get_threads_stats(threads_stats_file, range(1, 21))
    if args.vs_stats:
        get_speedup_stats(vs_stats_file)
    if args.hypred_stats:
        mpi_weak_scaling(hypred_stats_file)

    if args.analyze:
        df = pd.read_csv(args.analyze)
        if 'Payoff Avg' in df.columns:
            analyze_data(df, 'Iteration', 'Payoff Avg', 'Average Payoff', 'Convergence of Average Payoff', './plots/iteration_avg.png')
        elif 'Thread' in df.columns:
            analyze_data(df, 'Thread', 'Time', 'Time', 'Time elapsed', './plots/threads_stats.png', 'min')
        elif 'Day' in df.columns:
            if args.plot_type == 'box':
                plot_box_statistics(df)
            elif args.plot_type == 'scatter':
                plot_end_price_over_days(df)
        elif 'schedule' in df.columns:
            analyze_shcdules(schedule_stats_file,schdules_plot_name)
        elif 'time_serial' in df.columns:
            plot_time_and_speedup(vs_stats_file,vs_info)
        if 'c' in df.columns:
            plot_weak_scaling(hypred_stats_file,weak_scaling_image)

