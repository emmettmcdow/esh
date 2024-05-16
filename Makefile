
dev = docker exec -it dev-machine
CFLAGS := -static -g -Wall
CC := musl-gcc
CWD := $(shell pwd)

SOURCEDIR = $(CWD)/src
TESTDIR = $(CWD)/test
BUILDDIR = $(CWD)/output

SOURCES = $(wildcard $(SOURCEDIR)/*.c)
OBJECTS = $(patsubst $(SOURCEDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%.o,$(TEST_SOURCES))


all: $(BUILDDIR) $(BUILDDIR)/esh

.PHONY: dev-machine
dev-machine:
	-docker rm -f dev-machine
	docker build --target=dev -t rocky:dev .
	docker run -itd --name dev-machine -v $(CWD)/:/root/esh rocky:dev

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	-mkdir output/

$(OBJECTS): $(BUILDDIR)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJECTS): $(BUILDDIR)/%.o: $(TESTDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/esh: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILDDIR)/test: $(TEST_OBJECTS) $(filter-out $(BUILDDIR)/esh.o, $(OBJECTS))
	$(CC) $(CFLAGS) $^ -o $@

unit-test: $(BUILDDIR)/esh $(BUILDDIR)/test
	$(BUILDDIR)/test 

.PHONY: valgrind
valgrind: $(BUILDDIR)/esh $(BUILDDIR)/test
	echo "Valgrinding"
	valgrind $(BUILDDIR)/test
	valgrind $(BUILDDIR)/esh

.PHONY: format
format: $(SOURCEDIR)/* $(TESTDIR)/*
	clang-format -i $^
