
OUT_DIR=../logAndApply/
GRPC_CPP_PLUGIN=/root/local/bin/grpc_cpp_plugin

all:
	protoc  --grpc_out=${OUT_DIR} --cpp_out=${OUT_DIR} --experimental_allow_proto3_optional=true --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} logAndApply.proto 
.PHONY:all

clean:
	rm ${OUT_DIR}/logAndApply.*
.PHONY:clean
