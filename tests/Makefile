CPP := clang++
CPPFLAGS := -g -Wall -I.. -std=c++11

SRCS := $(wildcard *.cpp)
BINS := $(basename $(SRCS))

.PHONY: all
all: $(BINS)

%: %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

.PHONY: check
check: $(BINS)
	@failed=0; \
	for t in $(BINS); do \
	  if ! ./$$t > /dev/null 2>&1; then \
	    echo "FAIL: $$t"; \
	    failed=1; \
	  fi; \
	done; \
	if [ $$failed -eq 0 ]; then \
	  echo "all $$(echo $(BINS) | wc -w | tr -d ' ') test suites passed"; \
	fi; \
	exit $$failed

.PHONY: clean
clean:
	@rm -rf $(BINS) $(addsuffix .dSYM, $(BINS))
