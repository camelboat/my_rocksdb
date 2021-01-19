CXX = g++
CXXFLAGS = -g -Wall

.PHONY: clean 

all: sst_shipping util_test

sst_shipping : sst_shipping.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@  -std=c++11 -lstdc++fs -lpthread -lboost_system -lboost_thread

# util_test :: util_test.cc
# 	$(CXX) $(CXXFLAGS) $@.cc -o$@  -std=c++11 -lstdc++fs 

 clean:
	rm -f sst_shipping
