# esh - Emmett's Shell
I wrote this shell as a part of a job interview. I spent so much time on this sucker
I felt I had to release it.

I didn't end up getting the job, this shell failed in a few areas. I attribute that
to the fact that I had a week to build it and drastically underestimated the
difficulty of certain sections of the program. As a result, half of this shell
is over-engineered, the other half is a mess.

I'll leave it to the reader to determine which half is the mess ;)

## Installation
Provided is a Makefile and a Dockerfile. You can build this project simply by running
```
make dev-machine
docker exec -it dev-machine make
```
This will produce a binary that should be able to run on *any* Linux/amd64 OS. This is
because I built against musl. I have beef with glibc.

## Usage
Again, this will run on any Linux machine, but for the example here is how you run it
within a container:
```
docker exec -it dev-machine output/esh
```
FYI, output should also be a volumed directory in the sourcedir, so you can run it locally
as well if you like:
```
./output/esh
```

## Testing
I wrote some tests for my tokenizer. Feel free to take a look under the `test/` directory.
Additionally you can run them using:
```
make unit-test
``
