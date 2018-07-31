# Jaeger C Client

## Install

By default the client build will fetch all dependencies from the Hunter
build dependency system. Only cmake must be installed.

This builds a static library which must be linked into the application
along with all its dependencies. The simplest way to do that is to use
Hunter to find it in your own application's CMakeLists.txt.

    git submodule update --init
    mkdir -p build
    cd build
    cmake ..
    make -s
    make -s test
    make -s install

## Install (manual dependencies)

If you don't want to use Hunter or you can't you'll want to use local
dependencies.  This will be the case if you need offline builds, are building
for packages that must be reproducible, or aren't using cmake in your own
application's builds.

Hunter may be disabled with

    cmake -DHUNTER_ENABLED=0 -DJAEGERTRACINGC_USE_PCG=0

in which case local library dependencies must be installed:

* protobuf
  https://developers.google.com/protocol-buffers/
* protobuf-c >= 1.3.0
  https://github.com/protobuf-c/protobuf-c
* libhttp-parser
  https://github.com/nodejs/http-parser
* libjansson-dev
  https://github.com/akheron/jansson
* OpenTracing C API
  https://github.com/opentracing/opentracing-c
  (in `third_party/opentracing-c/`)

Run `git submodule update --init` to fetch submodules for IDL and testing.

Most systems do not package protobuf-c 1.3.0, so a source install or backported
packages will be required.

### Dependencies (Debian/Ubuntu)

Install avilable dependencies with:

    sudo apt install libhttp-parser-dev libjansson-dev

    # For protocol buffers 3.3, Ubuntu only:
    sudo add-apt-repository ppa:maarten-fonville/protobuf
    sudo apt-get update
    sudo apt-get install libprotobuf-dev protobuf-compiler

The packages `protobuf-c-compiler` and `libprotobuf-c-dev`, if available, will
be too old and must be installed from source or a backports repository.

### Dependencies (Fedora)

On Fedora 28:

    sudo dnf install jansson-devel http-parser-devel \
        protobuf-c-devel protobuf-c-compiler

On older Fedora releases, RHEL, and CentOS, protobuf-c-devel
protobuf-c-compiler are too old and must be installed from source, custom
packages, or a backport repository instead.
