# Jaeger C Client

## Install

Install the project library dependencies. On Debian, the following command
should work.

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
