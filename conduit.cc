/**
 * File: conduit.c
 * ---------------
 * This program reads one character from standard
 * input every second and (after a possible delay)
 * publishes one or more copies of that letter.
 * You can specify the optional delay with the --delay
 * flag, and optionally specify how many copies of 
 * each character to print with the --count flag.  For instance,
 * 
 * ./conduit --delay 2 --count 3
 * 
 * Will process 1 character from stdin every 2 seconds, 
 * and prints out 3 copies of each character.
 */
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
using namespace std;

static const int kIncorrectUsage = 1;
static void printUsage(const string& message, const string& executable) {
    cerr << "Error: " << message << endl;
    cerr << "Usage: ./" << executable << " [--delay m] [--count m]" << endl;
    exit(kIncorrectUsage);
}

static void extractArguments(int argc, char *argv[], size_t& delay, size_t& count) {
    struct option options[] = {
        {"delay", required_argument, NULL, 'd'},
        {"count", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0},
    };
    
    while (true) {
        int ch = getopt_long(argc, argv, "d:c:", options, NULL);
        if (ch == -1) break;
        switch (ch) {
        case 'd':
            delay = atoi(optarg);
            break;
        case 'c':
            count = atoi(optarg);
            break;
        default:
            printUsage("Unrecognized flag.", argv[0]);
        }
    }
    
    argc -= optind;
    if (argc > 0) printUsage("Too many arguments.", argv[0]);
}

int main(int argc, char *argv[]) {
    size_t delay = 0, count = 1; 
    extractArguments(argc, argv, delay, count);
    while (true) {
        int ch = fgetc(stdin);
        if (ch == -1) break; // break without delay
        if (delay > 0) sleep(delay);
        size_t repeat = ch == '\n' ? 1 : count;
        for (size_t i = 0; i < repeat; i++) printf("%c", ch);
        fflush(stdout);
    }

    return 0;
}
