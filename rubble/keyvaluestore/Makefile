SOURCES = $(wildcard $(subdir)*.cc)
SRCOBJS = $(patsubst %.cc,%.o,$(SOURCES))
GXX = g++

GXXFLAGS = -O0 -g2  \
		   -Wall  -std=c++11 -I./  \
		   -I$(PROTOBUF_PATH) -I$(GRPC_PATH)/include

GRPC_PATH= /root/local/bin/grpc
PROTOBUF_PATH= /home/zhang/grpc/third_party/protobuf/src

BUILDFLAGS = -static-libgcc -static-libstdc++ -std=c++11
DLIBS = -pthread -ldl

SLIBS = $(PROTOBUF_PATH)/.libs/libprotobuf.a \
		$(GRPC_PATH)/libs/opt/libares.a \
        $(GRPC_PATH)/libs/opt/libboringssl.a \
        $(GRPC_PATH)/libs/opt/libgpr.a \
        $(GRPC_PATH)/libs/opt/libgrpc.a \
        $(GRPC_PATH)/libs/opt/libgrpc++.a \
        $(GRPC_PATH)/libs/opt/libgrpc++_core_stats.a \
        $(GRPC_PATH)/libs/opt/libgrpc++_error_details.a \
        $(GRPC_PATH)/libs/opt/libgrpc_plugin_support.a \
        $(GRPC_PATH)/libs/opt/libgrpc_unsecure.a \
        $(GRPC_PATH)/libs/opt/libgrpc++_unsecure.a \
        $(GRPC_PATH)/libs/opt/libz.a \
        -Wl,--no-as-needed $(GRPC_PATH)/libs/opt/libgrpc++_reflection.a -Wl,--as-needed

%.grpc.pb.cc: %.proto
	$(PROTOBUF_PATH)/protoc --grpc_out=./ --plugin=protoc-gen-grpc=$(GRPC_PATH)/bins/opt/grpc_cpp_plugin $^

%.pb.cc: %.proto
	$(PROTOBUF_PATH)/protoc --cpp_out=./ $^

%.pb.o: %.pb.cc
	$(GXX) $(GXXFLAGS) -c $^ -o $@

%.o:%.cc
	$(GXX) $(GXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(GXX) $(GXXFLAGS) -c $^ -o $@

all: client server

client: examples.pb.o examples.grpc.pb.o  examples_client.o
	$(GXX) $(BUILDFLAGS) $^ -o $@ $(SLIBS) $(DLIBS)

server: examples.pb.o examples.grpc.pb.o  examples_server.o
	$(GXX) $(BUILDFLAGS) $^ -o $@ $(SLIBS) $(DLIBS)

clean:
	rm *.o *.pb.cc *.pb.h server client