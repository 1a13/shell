A fully functional shell program that supports a multi-process pipeline and I/O file redirection using system calls.

This shell cannot run in the background using &, run special shell commands (such as cd, jobs, fg, bg), run text editors (emacs / vim), nor does it support I/O file redirection in the middle of a pipeline.

conduit.cc, sigsegv.cc, spin.cc, split.cc, fpe.cc, open_fds.py files are used to test the shell program.

Note: stsh.cc is the only modified file
