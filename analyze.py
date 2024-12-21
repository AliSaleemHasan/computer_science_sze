import pandas as pd 
import matplotlib.pyplot as plt
import subprocess
import re
import os
import argparse


numerical_regix = r"\d+\.\d+"

def get_schedule_stats(filepath):
    try:
        df_rows=[]
        for schedule in ['static','dynamic','guided']:
          for chunk_size in [1,5,10,25,50,100,150,200,250,500]:
                os.environ["OMP_SCHEDULE"] = f'{schedule},{chunk_size}'
                ans = subprocess.check_output(['./monte-carlo.exe','3000'], text=True)
                time = re.findall(numerical_regix, ans)[1]
                print(f"time elapsed for 3000 iteration with '{schedule},{chunk_size}' schedule is ${time}\n")
                df_rows.append({'schedule':schedule,'chunk':chunk_size,'time':time})
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath,index=False)



    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")


def get_speedup_stats(filepath):
    try:
        df_rows=[]
        os.environ['OMP_SCHEDULE']='static,5'
        os.environ["OMP_NUM_THREADS"] = '6'
        for iteration in range(100,2100,100):
            ans_serial = subprocess.check_output(['./monte-carlo.exe',str(iteration),'0'], text=True)
            ans_parallel = subprocess.check_output(['./monte-carlo.exe',str(iteration)], text=True)
            time_serial = re.findall(numerical_regix, ans_serial)[1]
            time_parallel = re.findall(numerical_regix, ans_parallel)[1]
            print(f"time elapsed for {iteration} iterations for: serial is {time_serial} and for parallel is {time_parallel}\n")
            df_rows.append({'iteration':iteration,'time_serial':time_serial,'time_parallel':time_parallel})
        df = pd.DataFrame(df_rows)
        df.to_csv(filepath,index=False)



    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")


def plot_time_and_speedup(file_name,image):
    df = pd.read_csv(file_name)
    df['Speedup'] = df['time_serial'] / df['time_parallel']
    
    fig, ax1 = plt.subplots(figsize=(10, 5))
    ax1.bar(df['iteration'] - 10, df['time_serial'], width=20, label='Time Serial', alpha=0.7, color='purple')
    ax1.bar(df['iteration'] + 10, df['time_parallel'], width=20, label='Time Parallel', alpha=0.7, color='blue')
    
    ax1.set_xlabel('Iterations')
    ax1.set_ylabel('Time (seconds)')
    ax1.set_title('Time and Speedup for Different Iterations')
    ax1.legend(loc='upper left')
    ax1.grid(True)
    
    ax2 = ax1.twinx()
    ax2.plot(df['iteration'], df['Speedup'], label='Speedup', color='deeppink', marker='o', linestyle='-', linewidth=2)
    ax2.set_ylabel('Speedup')
    ax2.legend(loc='upper right')
    
    plt.savefig(image)
    plt.show()


def get_threads_stats(filepath,iterations):
    try:
        df_rows = []
        os.environ['OMP_SCHEDULE']='static,5'
        for iteration in iterations:  
            os.environ["OMP_NUM_THREADS"] = str(iteration)
            print(f"Running with {iteration} threads...")
            ans = subprocess.check_output(['./monte-carlo.exe','3000'], text=True)
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
            ans = subprocess.check_output(['./monte-carlo.exe',str(iteration)], text=True)
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

def analyze_data(df, x, y, ylabel, xlabel, image, horizontal_func='Mean'):
    fun = df[y].mean() if horizontal_func == 'Mean' else df[y].min()
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
    plt.boxplot([df['Mean'], df['Min'], df['Max'], df['Std Dev']], labels=['Mean', 'Min', 'Max', 'Std Dev'])
    plt.title('Box Plot of Statistics')
    plt.ylabel('Values')
    plt.grid(True)
    plt.savefig(output_image)
    plt.show()

def plot_end_price_over_days(df, output_image='./plots/end_price_over_days.png'):
    plt.figure(figsize=(10, 6))
    plt.scatter(df['Day'], df['End Price'], marker='o')
    plt.xlabel('Day')
    plt.ylabel('End Price')
    plt.title('End Price over Days')
    plt.grid(True)
    plt.savefig(output_image)
    plt.show()

def parse_args():
    parser = argparse.ArgumentParser(description="Monte Carlo Analysis")
    parser.add_argument('--iterations-stats', action='store_true', help="Save iterations average data")
    parser.add_argument('--threads-stats', action='store_true', help="Save threads statistics data")
    parser.add_argument('--schedule-stats', action='store_true', help="Save schdules statistics data")
    parser.add_argument('--vs-stats', action='store_true', help="Save serial vs parallel statistics data")
    parser.add_argument('--analyze', action='store', type=str, help="Analyze data from CSV file")
    parser.add_argument('--plot-type', type=str, choices=[ 'box', 'scatter'], help="Type of plot to generate")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    schdules_plot_name='./plots/schdules.png'
    vs_plot_name='./plots/vs.png'
    iterations_avg_file = "./data/iterations-avg.csv"
    threads_stats_file = "./data/threads.csv"
    schedule_stats_file = "./data/schedule.csv"
    vs_stats_file = "./data/vs.csv"



    if args.schedule_stats:
        get_schedule_stats(schedule_stats_file)
    if args.iterations_stats:
        get_iterations_stats(iterations_avg_file, range(200, 10200, 200))
    if args.threads_stats:
        get_threads_stats(threads_stats_file, range(1, 21))
    if args.vs_stats:
        get_speedup_stats(vs_stats_file)


    if args.analyze:
        df = pd.read_csv(args.analyze)
        if 'Payoff Avg' in df.columns:
            analyze_data(df, 'Iteration', 'Payoff Avg', 'Average Payoff', 'Convergence of Average Payoff', './plots/iteration_avg.png')
        elif 'Thread' in df.columns:
            analyze_data(df, 'Thread', 'Time', 'Time', 'Time elapsed', './plots/threads_stats.png', 'Min')
        elif 'Day' in df.columns:
            if args.plot_type == 'box':
                plot_box_statistics(df)
            elif args.plot_type == 'scatter':
                plot_end_price_over_days(df)
        elif 'schedule' in df.columns:
            analyze_shcdules(schedule_stats_file,schdules_plot_name)
        elif 'time_serial' in df.columns:
            plot_time_and_speedup(vs_stats_file,vs_plot_name)
