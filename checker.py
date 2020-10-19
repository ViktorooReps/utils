from pprint import pprint as pp
from subprocess import Popen, PIPE, STDOUT

import sys
import os

import argparse
import subprocess

#TODO progress bar

parser = argparse.ArgumentParser(description="Automatic tester")
parser.add_argument("x_file")
parser.add_argument("--tests", "-t", default="tests.txt")
parser.add_argument("--print", "-p", default="wrong", choices=["none", "all", "wrong"])
parser.add_argument("--ntests", "-nt", default=5, type=int)
parser.add_argument("--mode", "-m", choices=["fill", "test"], default="test")
parser.add_argument("--output_file", "-o", default="output.txt")

args = parser.parse_args()

if args.x_file == None:
    print("Executable is required")
    exit()

if args.mode == "fill":
    tests = []
    outputs = []
    ntest = 0
    total_tests = 0
    written = 0
    with open(args.tests) as tst_file:
        tests = list(tst_file.read().split("[INPUT]\n"))[1:]
        for test in tests:
            prog_output = subprocess.run(['./' + args.x_file], 
                                        stderr=subprocess.STDOUT, 
                                        stdout=PIPE,
                                        input=test, 
                                        encoding='ascii').stdout
            outputs.append(prog_output)

    with open(args.output_file, "w") as out_file:
        total_tests = len(tests)
        for test, output in zip(tests, outputs):
            ntest += 1
            out_file.writelines(["[INPUT]\n", 
                                 test, 
                                 "[OUTPUT]\n", 
                                 output])

            if args.print == "all":
                print(30 * "-")
                print("TEST ", ntest, ":\n", test, sep="")
                print("PROGRAM OUTPUT:\n", output, sep="")
                print(30 * "-")

            written += 1
            if written == args.ntests:
                break

    
    print("Written tests: ", written, "/", total_tests, sep="")

if args.mode == "test":
    with open(args.tests) as tst_file:
        ntest = 0
        passed = 0
        printed = 0

        tests = list(tst_file.read().split("[INPUT]\n"))[1:]
        for test in tests:
            if printed == int(args.ntests):
                break

            ntest += 1

            test_input, test_output = test.split("[OUTPUT]\n")

            prog_output = subprocess.run(['./' + args.x_file], 
                                        stderr=subprocess.STDOUT, 
                                        stdout=PIPE,
                                        input=test_input, 
                                        encoding='ascii').stdout
            
            if args.print == "all":
                print(30 * "-")
                print("TEST ", ntest, ":\n", test_input, sep="")
                print("PROGRAM OUTPUT:\n", prog_output, sep="")
                print("NEEDED OUTPUT:\n", test_output, sep="")
                print(30 * "-")
                printed += 1

            if prog_output == test_output:
                passed += 1
            else:
                if args.print == "wrong":
                    print(30 * "-")
                    print("TEST ", ntest, ":\n", test_input, sep="")
                    print("PROGRAM OUTPUT:\n", prog_output, sep="")
                    print("NEEDED OUTPUT:\n", test_output, sep="")
                    print(30 * "-")
                    printed += 1

        print("Passed tests: ", passed, "/", ntest, sep="")
    
