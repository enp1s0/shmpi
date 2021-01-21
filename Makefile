CXX=g++
CXXFLAGS=-std=c++11 -I./include -pthread
OBJDIR=objs
SRCDIR=src
TARGETDIR=lib
TESTDIR=test
TARGET=libshmpi.a

all: $(TARGETDIR)/$(TARGET) test

MODULES=send_recv.cpp allreduce.cpp alltoall.cpp
OBJECTS=$(MODULES:%.cpp=$(OBJDIR)/%.o)

$(TARGETDIR)/$(TARGET):$(OBJECTS)
	[ -d "$(TARGETDIR)" ] || mkdir $(TARGETDIR)
	ar rcs $@ $+

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	[ -d "$(OBJDIR)" ] || mkdir $(OBJDIR)
	$(CXX) $+ $(CXXFLAGS) -o $@ -c

TESTSRCS=send_recv.cpp allreduce.cpp
# The tests below need nvcc to compile.
#TESTSRCS+=opencl_buffer.cpp
TESTS=$(TESTSRCS:%.cpp=$(TESTDIR)/%.test)
test: $(TESTS)

$(TESTDIR)/%.test: $(TESTDIR)/%.cpp $(TARGETDIR)/$(TARGET)
	$(CXX) $+ $(CXXFLAGS) -o $@ -L./$(TARGETDIR) -lshmpi -lmpi

NVCC=nvcc
NVCCFLAGS=-std=c++11 -I./include -lOpenCL
$(TESTDIR)/opencl_buffer.test: $(TESTDIR)/opencl_buffer.cpp $(TARGETDIR)/$(TARGET)
	$(NVCC) $+ $(NVCCFLAGS) -o $@ -L./$(TARGETDIR) -lshmpi -lmpi

clean:
	rm -rf $(OBJDIR)
	rm -rf $(TARGETDIR)
	rm -f $(TESTDIR)/*.test
