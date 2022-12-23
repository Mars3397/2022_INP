import os
from subprocess import PIPE, Popen
import subprocess
import difflib
from concurrent.futures import ThreadPoolExecutor
import argparse
import concurrent.futures

SERVER_PORT = 8888
SERVER_ADDRESS = 'x.x.x.x'
SERVER_ADDRESSES = ['X.X.X.X', 'X.X.X.X', 'X.X.X.X']

TESTCASES_SRC = "testcases_part3"
TESTCASES_YOUR = 'testcases_your'
TESTCASES_CORRECT = 'testcases_correct_part3'
TESTCASES_DIFF = 'testcases_diff'

def run_testcase(filename):
    print(f'Running {filename}')

    subprocess.run([
        'python3', 'client_part3.py', '-p', f'{SERVER_PORT}', '--server-ips', f'{SERVER_ADDRESSES[0]}', 
        f'{SERVER_ADDRESSES[1]}', f'{SERVER_ADDRESSES[2]}', '--filename', f'{filename}', '--dst', f'{TESTCASES_YOUR}'
    ])

def diff_file(filename):
    ta_answer = None
    your_answer = None

    src_file = os.path.join(TESTCASES_CORRECT, filename)
    with open(src_file, 'r') as f:
        ta_answer = f.readlines()

    dst_file = os.path.join(TESTCASES_YOUR, filename)
    with open(dst_file, 'r') as f:
        your_answer = f.readlines()

    assert ta_answer and your_answer
    dif = difflib.context_diff(ta_answer,
                               your_answer,
                               fromfile=src_file,
                               tofile=dst_file)
    dif_list = list(dif)
    if not dif_list:
        return 1

    diff_file = os.path.join(TESTCASES_DIFF, filename)
    with open(diff_file, 'w+') as f:
        f.writelines(iter(dif_list))
    return 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser('Demo script\n')

    parser.add_argument('-p', dest='port', type=int, default=8888)
    parser.add_argument('--server-ip', dest='server_ip', type=str, default=None)
    parser.add_argument('--server-ips', dest='server_ips', type=str, default=None, nargs='+')
    parser.add_argument('-t', '--testcase', dest='testcase', type=str, default='')
    args = parser.parse_args()

    SERVER_PORT = args.port
    SERVER_ADDRESS = args.server_ip
    SERVER_ADDRESSES = args.server_ips

    os.makedirs(TESTCASES_YOUR, exist_ok=True)
    os.makedirs(TESTCASES_DIFF, exist_ok=True)
    testcases_src = [args.testcase] if args.testcase else os.listdir(TESTCASES_SRC)

    for i in range(len(testcases_src)):
        run_testcase(testcases_src[i])
        print(f'{testcases_src[i]} finished')
        if i < len(testcases_src)-1:
            input("Press Enter if you manually restart 3 servers...")

    with ThreadPoolExecutor() as executor:
        future_to_filename = {
            executor.submit(diff_file, filename): filename
            for filename in testcases_src
        }
        total_pass_testcases = 0
        for future in concurrent.futures.as_completed(future_to_filename):
            filename = future_to_filename[future]
            try:
                score = future.result()
                if score == 0:
                    print(f'testcases/{filename} is not correct')
                else:
                    total_pass_testcases += score

            except Exception as e:
                print(e)

        print(f'PASS {total_pass_testcases}/{len(testcases_src)}')
