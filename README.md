Trasher - POC for reading/writing shared memory from TEE

Shared memory is the type of memory used for exchanging data between normal
world and secure world in a GP defined TEE solution.

It's rather easy to figure out the the physical address(es) normally used for
parameter passing and hence you can map this area in normal world user space (as
root). By doing so you can both read and write to the shared memory even though
the Trusted Application currently is running and using this value.
