CPP := clang++
CPPFLAGS := -g -Wall -I.. -std=c++11

SRCS := $(wildcard *.cpp)
BINS := $(basename $(SRCS))

.PHONY: all
all: $(BINS)

%: %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	@rm -rf $(BINS) $(addsuffix .dSYM, $(BINS))

