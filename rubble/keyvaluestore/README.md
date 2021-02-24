For installation of grpc, see https://github.com/camelboat/EECS_6897_Distributed_Storage_System_Project_Scripts/blob/xz_branch/setup_scripts/grpc_setup.sh

And before installing grpc, make sure you have the right cmake version, put this to the same directory as the grpc_setup.sh : https://github.com/camelboat/EECS_6897_Distributed_Storage_System_Project_Scripts/blob/xz_branch/setup_scripts/cmake_install.sh
and then run `sudo bash grpc_setup.sh`

Before compiling this project, make sure the grpc install dir is exported to the PATH.

For example, I set `MY_INSTALL_DIR` to `$HOME/local` during compilation of the grpc source, so I exported my PATH as follows:

```bash
export PATH="$PATH:$MY_INSTALL_DIR/bin"
```

To compile, run: 

```bash
mkdir -p cmake/build
cd cmake/build
cmake ../..
make -j
```