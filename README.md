# Jaeger C Client

## Install

By default the client build will fetch all dependencies from the Hunter
build dependency system. Only cmake must be installed.

This may be disabled with

    cmake -DHUNTER_ENABLED=0 -DJAEGERTRACINGC_USE_PCG=0

in which case local library dependencies must be installed.

On Debian, the following command should work.

```
sudo apt install libhttp-parser-dev libjansson-dev protobuf-c-compiler \
                 libprotobuf-c-dev
```

or on Fedora

```
sudo dnf install jansson-devel http-parser-devel protobuf-c-devel \
		protobuf-c-compiler
```

Run `git submodule update --init` to fetch the submodules.
