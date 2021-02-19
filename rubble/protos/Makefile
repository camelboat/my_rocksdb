
GRPC_INSTALL_DIR=/root/local/bin
LA_OUT_DIR=../logAndApply
KV_OUT_DIR=../keyvaluestore

GRPC_CPP_PLUGIN=${GRPC_INSTALL_DIR}/grpc_cpp_plugin

all: logAndApply keyvaluestore

logAndApply: 
	protoc  --grpc_out=${LA_OUT_DIR} --cpp_out=${LA_OUT_DIR} --experimental_allow_proto3_optional=true --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} logAndApply.proto 
.PHONY:logAndApply

keyvaluestore:
	protoc  --grpc_out=${KV_OUT_DIR} --cpp_out=${KV_OUT_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} keyvaluestore.proto 
.PHONY:keyvaluestore

clean:
	rm ${LA_OUT_DIR}/logAndApply.*
	rm ${KV_OUT_DIR}/keyvaluestore.*
.PHONY:clean
