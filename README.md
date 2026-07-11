# Mini Service Supervisor

A compact `launchd`/`systemd`-style supervisor MVP written in C. It launches a child process, captures stdout/stderr through a pipe, watches termination via `waitpid`, restarts failed processes with exponential backoff, and shuts down the child process group on SIGINT/SIGTERM.

## Build
```bash
make
```

## Test scenario
```bash
rm -f /tmp/flaky_count
./supervisor ./flaky.sh
```
Expected: the child fails twice, is restarted with increasing delay, then exits successfully.

Graceful shutdown test:
```bash
./supervisor /bin/sh -c 'while true; do echo alive; sleep 1; done'
# press Ctrl+C
```
