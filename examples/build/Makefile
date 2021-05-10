
CXXFLAGS=-std=c++11 -pedantic -g -O2\
	-Wall \
	-Wcast-align \
	-Wcast-qual \
	-Wctor-dtor-privacy \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Werror \
	-Wextra \
	-Wformat=2 \
	-Winit-self \
	-Wmissing-include-dirs \
	-Wno-unused\
	-Wold-style-cast \
	-Woverloaded-virtual \
	-Wredundant-decls \
	-Wshadow \
	-Wsign-conversion \
	-Wsign-promo \
	-Wstrict-overflow=5 \
	-Wswitch-default \
	-Wundef \

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S),Linux)
  LDFLAGS += -lpthread
  ifeq ($(config),tsan)
    LDFLAGS += -ltsan
    CXXFLAGS += -fsanitize=thread -pie -fPIE
  endif
endif

px_sched_examples = px_sched_example1 px_sched_example2 px_sched_example3 px_sched_example4 px_sched_example5 px_sched_example6 px_sched_example7 px_sched_example8
px_render_examples = px_render_example_imgui #WIP: px_render_example_rtt px_render_example_triangle

all: $(px_sched_examples) $(px_render_examples)

$(px_sched_examples): %: ../%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	$(CXX) -DPX_SCHED_CONFIG_SINGLE_THREAD $(CXXFLAGS) -o $@_noMT $< $(LDFLAGS)


$(px_render_examples): %: ../%.cpp
	$(CXX) -std=c++14 -fpermissive -D linux -g -O2 -I .. -o $@ $< $(LDFLAGS) -ldl -lX11 -lXi -lXcursor

.PHONY: clean tests
clean:
	rm -f $(px_sched_examples)

tests: $(px_sched_examples)
	./px_sched_example1 
	./px_sched_example2 
	./px_sched_example3 
	./px_sched_example4 
	./px_sched_example5 
	./px_sched_example6 
	./px_sched_example7
	./px_sched_example8
	@echo "ALL px_sched_examples executed"
	./px_sched_example1_noMT	
	./px_sched_example2_noMT 
	./px_sched_example3_noMT 
	./px_sched_example4_noMT 
	./px_sched_example5_noMT 
	./px_sched_example6_noMT 
	./px_sched_example7_noMT
	./px_sched_example8_noMT
	@echo "ALL px_sched_examples executed (no MT)"
