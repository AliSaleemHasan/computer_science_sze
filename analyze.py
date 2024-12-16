import pandas as pd 
import matplotlib.pyplot as plt
import subprocess
import re
import os
import argparse

def save_data(file_name, iterations, command_args, regex_pattern, key_names,target=0):
    try:
        df_rows = []
        for iteration in iterations:
            if  'Thread' in key_names:
                os.environ["OMP_NUM_THREADS"] = str(iteration)
            print(f"Running iteration ${iteration}...")
            ans = subprocess.check_output(command_args + [str(iteration)], text=True)
            numericals = re.findall(regex_pattern, ans)
            df_rows.append({key_names[0]: iteration, key_names[1]: float(numericals[target])})
            print(f"{key_names[1]}:{numericals[target]}")
        df = pd.DataFrame(df_rows)
        df.to_csv(file_name, index=False)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")

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
    parser.add_argument('--iterations-avg', action='store_true', help="Save iterations average data")
    parser.add_argument('--threads-stats', action='store_true', help="Save threads statistics data")
    parser.add_argument('--analyze', action='store', type=str, help="Analyze data from CSV file")
    parser.add_argument('--plot-type', type=str, choices=[ 'box', 'scatter'], help="Type of plot to generate")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    iterations_avg_file = "./data/iterations-avg.csv"
    threads_stats_file = "./data/threads.csv"

    if args.iterations_avg:
        save_data(iterations_avg_file, range(200, 10000, 200), ["./monte-carlo.exe"], r"\d+\.\d+", ['Iteration', 'Payoff Avg'])

    if args.threads_stats:
        save_data(threads_stats_file, range(1, 21), ["./monte-carlo.exe", "1500"], r"\d+\.\d+", ['Thread', 'Time'],target=1)

    if args.analyze:
        df = pd.read_csv(args.analyze)
        if 'Iteration' in df.columns:
            analyze_data(df, 'Iteration', 'Payoff Avg', 'Average Payoff', 'Convergence of Average Payoff', './plots/iteration_avg.png')
        elif 'Thread' in df.columns:
            analyze_data(df, 'Thread', 'Time', 'Time', 'Time elapsed', './plots/threads_stats.png', 'Min')
        elif 'Day' in df.columns:
            if args.plot_type == 'box':
                plot_box_statistics(df)
            elif args.plot_type == 'scatter':
                plot_end_price_over_days(df)
