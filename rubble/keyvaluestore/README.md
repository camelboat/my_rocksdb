before compile, make sure the grpc install dir is exported to the PATH.

for example, I set `MY_INSTALL_DIR` to `$HOME/local` during compilation of the grpc source, so I exported my PATH as follows:

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