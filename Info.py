import os
import subprocess
import psutil
import time

def run_executable(executable_path, args):
    start_time = time.time()

    process = subprocess.Popen([executable_path] + args, shell=True)
    process.wait()

    end_time = time.time()

    return process.returncode, end_time - start_time

def log_system_info():
    print("System Information:")
    print(f"CPU Cores: {psutil.cpu_count(logical=False)}")
    print(f"Logical CPUs: {psutil.cpu_count(logical=True)}")
    print(f"Total Memory: {psutil.virtual_memory().total / (1024 ** 3):.2f} GB")

if __name__ == "__main__":
    executable_name = "Project1.exe"
    command_args = ["bible.txt", "16"]

    current_directory = os.path.dirname(os.path.abspath(__file__))
    executable_path = os.path.join(current_directory, executable_name)

    log_system_info()

    return_code, execution_time = run_executable(executable_path, command_args)

    print(f"\nExecution Time: {execution_time:.2f} seconds")
    print(f"Exit Code: {return_code}")

    process = psutil.Process(os.getpid())
    memory_info = process.memory_info()

    print(f"User CPU Time: {process.cpu_times().user:.2f} seconds")
    print(f"System CPU Time: {process.cpu_times().system:.2f} seconds")
    print(f"Memory Usage: {memory_info.rss / (1024 ** 2):.2f} MB")
