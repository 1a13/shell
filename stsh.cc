/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include <fcntl.h>
#include <sys/wait.h>  // for waitpid
#include <unistd.h>    // for fork
#include <iostream>
#include "stsh-exception.h"
#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "fork-utils.h"  // this needs to be the last #include in the list

using namespace std;

/* Check if there is an input redirection in pipeline p.
   If so, input is read in from the file.
 */
void inputRedirect(const pipeline &p) {
    std::string inputFile = p.input;
    if (!inputFile.empty()) {
        int fd = open(inputFile.c_str(), O_RDONLY, 0644);
        if (fd == -1) {
            throw STSHException("Could not open " + inputFile);
        }
        
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
}

/* Check if there is an output redirection in pipeline p
   If so, output is written into the file, which needs
   to be truncated if it exists or created if it doesn't.
 */
void outputRedirect(const pipeline &p) {
    std::string outputFile = p.output;
    if (!outputFile.empty()) {
        int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            throw STSHException("Could not open " + outputFile);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

/* Wait for child processes to complete and check for segmentation
   faults, which should only print once per pipeline.
 */
void waitForChildren(size_t numCmds, pid_t pids[]) {
    bool segfault = false;
    for (int i = 0; i < numCmds; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        // check for segfault, only print once
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV && !segfault) {
            segfault = true;
            cerr << "Segmentation fault" << endl;
        }
    }
}

/* Create new process for the provided pipeline. This function 
   can only run pipelines which have one command.
*/
void runSingleCommand(const pipeline &p, pid_t pids[]) {
    command cmd = p.commands[0];
    pids[0] = fork();

    // Child process
    if (pids[0] == 0) {
        // check for I/O redirection
        inputRedirect(p);
        outputRedirect(p);

        execvp(cmd.argv[0], cmd.argv);

        // terminate child process if execvp returns
        throw STSHException(string(cmd.argv[0]) + ": Command not found.");
    }

    waitForChildren(1, pids);
}

/**
 * Create new process(es) for the provided pipeline. Spawns child processes with
 * input/output redirected to the appropriate pipes and/or files.
 */
void runPipeline(const pipeline& p) {
    size_t numCmds = (p.commands).size();
    pid_t pids[numCmds];

    // edge case: 1 command in pipeline
    if (numCmds == 1) {
        runSingleCommand(p, pids);
        return;
    }

    int prev_fds[2];
    pipe2(prev_fds, O_CLOEXEC);

    command cmd = p.commands[0];
    pids[0] = fork();

    // first process
    if (pids[0] == 0) {
        // check for input redirection
        inputRedirect(p);

        dup2(prev_fds[1], STDOUT_FILENO);
        execvp(cmd.argv[0], cmd.argv);
        throw STSHException(string(cmd.argv[0]) + ": Command not found.");
    }
    close(prev_fds[1]);

    // repeat proccesses, connect to two pipes
    for (int i = 1; i < numCmds - 1; i++) {
        int curr_fds[2];
        pipe2(curr_fds, O_CLOEXEC);

        cmd = p.commands[i];
        pids[i] = fork();

        if (pids[i] == 0) {
            dup2(prev_fds[0], STDIN_FILENO);
            dup2(curr_fds[1], STDOUT_FILENO);
            execvp(cmd.argv[0], cmd.argv);
            throw STSHException(string(cmd.argv[0]) + ": Command not found.");
        }
        close(prev_fds[0]);
        close(curr_fds[1]);

        // save file desciptors of current pipe for next iteration
        for (int j = 0; j < 2; j++) {
            prev_fds[j] = curr_fds[j];
        }
    }

    int lastIndex = numCmds - 1;
    cmd = p.commands[lastIndex];
    pids[lastIndex] = fork();

    // intialize last process
    if (pids[lastIndex] == 0) {
        // check for output redirection
        outputRedirect(p);

        dup2(prev_fds[0], STDIN_FILENO);
        execvp(cmd.argv[0], cmd.argv);
        throw STSHException(string(cmd.argv[0]) + ": Command not found.");
    }
    close(prev_fds[0]);

    // wait for all processes to complete and check for seg faults
    waitForChildren(numCmds, pids);
}

/**
 * Define the entry point for a process running stsh.
 */
int main(int argc, char* argv[]) {
    pid_t stshpid = getpid();
    rlinit(argc, argv);
    while (true) {
        string line;
        if (!readline(line) || line == "quit" || line == "exit") break;
        if (line.empty()) continue;
        try {
            pipeline p(line);
            runPipeline(p);
        } catch (const STSHException& e) {
            cerr << e.what() << endl;
            if (getpid() != stshpid) exit(0);  // if exception is thrown from child process, kill it
        }
    }
    return 0;
}
