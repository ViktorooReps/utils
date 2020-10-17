from pprint import pprint as pp
from subprocess import Popen, PIPE, STDOUT

import sys
import os

import argparse
import subprocess

parser = argparse.ArgumentParser(description="Automatic tester")
parser.add_argument("x_file")
parser.add_argument("--tests", "-t", default="tests.txt")
parser.add_argument("--print", "-p", default="wrong", choices=["none", "all", "wrong"])
parser.add_argument("--ntests", "-nt", default=5, type=int)
parser.add_argument("--step", "-s", type=int)

args = parser.parse_args()

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
    