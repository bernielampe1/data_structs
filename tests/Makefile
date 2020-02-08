CPP := clang++
CPPFLAGS := -g -Wall -I..

SRCS := $(wildcard *.cpp)
BINS := $(basename $(SRCS))

.PHONY: all
all: $(BINS)

%: %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	@rm -rf $(BINS) $(addsuffix .dSYM, $(BINS))

