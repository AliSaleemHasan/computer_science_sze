import pandas as pd 
import matplotlib.pyplot as plt



def analyze_paths(file_name):
    df = pd.read_csv(file_name)
    print(df.info())
    return df

def plot_paths_data(df):
    plt.figure(figsize=(30,15))
    plt.plot(df['Day'],df['Mean'],label="Mean")
    plt.plot(df['Day'],df['Min'],label="Min")
    plt.plot(df['Day'],df['Max'],label="Max")
    plt.legend()
    plt.grid(True)
    plt.savefig('line.png')

def box_stats(df):
    plt.figure(figsize=(12,6))
    plt.boxplot([df['Mean'], df['Min'], df['Max'], df['Std Dev']], labels=['Mean', 'Min', 'Max', 'Std Dev'])
    plt.title('Box Plot of Statistics Over Iterations') 
    plt.ylabel('Values')
    plt.savefig('box.png')

if __name__ == "__main__":
    df = analyze_paths('stats.csv')
    plot_paths_data(df)
    box_stats(df)
