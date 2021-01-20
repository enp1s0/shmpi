# Small Host MPI

This is a framework for small host heterogeneous environment.

## Requirements
- MPI (We tests with Open MPI)
- C++ >= 11

## Supported functions
- Send/Recv
- Allreduce (also `MPI_IN_PLACE`(`shmpi::shmpi_in_place`))

## Installation and usage
1. Clone this repository
```
git clone https://github.com/enp1s0/shmpi
cd shmpi
```

2. Build
```
make
```

3. Link to your program
```
g++ -I/path/to/shmpi/include -L/path/to/shmpi/lib -lshmpi ...
```

## License
MIT
