# service_supervisor
A compact `launchd`/`systemd`-style supervisor MVP written in C. It launches a child process, captures stdout/stderr through a pipe, watches termination via `waitpid`, restarts failed processes with exponential backoff, and shuts down the child process group on SIGINT/SIGTERM.
