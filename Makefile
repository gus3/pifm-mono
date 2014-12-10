CFLAGS += -fno-strict-aliasing -fwrapv -g -pg -O0 --coverage
CXXFLAGS := $(CFLAGS)

pifm: objs
	$(CXX) $(CXXFLAGS) -o $@ *.o

objs: main.o support.o sink.o

clean:
	rm -f pifm *.o *.gcno gmon.out
