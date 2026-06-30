
try:

```
make setup
make multikernel
make run-multikernel
```

and it will produce the output of shared server on MK.

`make setup` is required only at the first run, the remaining targets are just for **build** and **execute** without changing the codebase (if developing is necessary).

if different system files are what you wanted, change the
`SYSTEM_FILE` of the root Makefile to your new SDF, currently `passive_server-two-cores.system`.

if you want to run the code on a different platform,
you need to change both the `MICROKIT_BOARD` in Makefile or your environment, and the config files in `multikernel-passive-server`,
including (1) print.c for different UART configuration, and (2) SDF for different setup.
