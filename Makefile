CXX=g++
CXXFLAGS=-std=c++11 -I./include
OBJDIR=objs
SRCDIR=src
TARGETDIR=lib
TESTDIR=test
TARGET=libshmpi.a

all: $(TARGETDIR)/$(TARGET)

MODULES=send_recv.cpp
OBJECTS=$(MODULES:%.cpp=$(OBJDIR)/%.o)

$(TARGETDIR)/$(TARGET):$(OBJECTS)
	[ -d "$(TARGETDIR)" ] || mkdir $(TARGETDIR)
	ar rcs $@ $+

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	[ -d "$(OBJDIR)" ] || mkdir $(OBJDIR)
	$(CXX) $+ $(CXXFLAGS) -o $@ -c

TESTSRCS=send_recv.cpp
TESTS=$(TESTSRCS:%.cpp=$(TESTDIR)/%.test)
test: $(TARGETDIR)/$(TARGET) $(TESTS)

$(TESTDIR)/%.test: $(TESTDIR)/%.cpp
	$(CXX) $+ $(CXXFLAGS) -o $@ -L./$(TARGETDIR) -lshmpi -lmpi

clean:
	rm -rf $(OBJDIR)
	rm -rf $(TARGETDIR)
	rm -f $(TESTDIR)/*.test
