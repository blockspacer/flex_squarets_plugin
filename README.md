# About

Plugin for [https://github.com/blockspacer/flextool](https://github.com/blockspacer/flextool)

Plugin provides support for template engine.

See for details [https://blockspacer.github.io/flex_docs/](https://blockspacer.github.io/flex_docs/)

## Before installation

- [installation guide](https://blockspacer.github.io/flex_docs/download/)

## Installation

```bash
export CXX=clang++-6.0
export CC=clang-6.0

# NOTE: change `build_type=Debug` to `build_type=Release` in production
# NOTE: use --build=missing if you got error `ERROR: Missing prebuilt package`
CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
    cmake -E time \
      conan create . conan/stable \
      -s build_type=Debug -s cling_conan:build_type=Release \
      --profile clang \
          -o flex_squarets_plugin:enable_clang_from_conan=False \
          -e flex_squarets_plugin:enable_tests=True
```

## Development flow (for contributors)

Commands below may be used to build project locally, without system-wide installation.

```bash
export CXX=clang++-6.0
export CC=clang-6.0

cmake -E remove_directory build

cmake -E make_directory build

# NOTE: change `build_type=Debug` to `build_type=Release` in production
build_type=Debug

# install conan requirements
CONAN_REVISIONS_ENABLED=1 \
    CONAN_VERBOSE_TRACEBACK=1 \
    CONAN_PRINT_RUN_COMMANDS=1 \
    CONAN_LOGGING_LEVEL=10 \
    GIT_SSL_NO_VERIFY=true \
        cmake -E chdir build cmake -E time \
            conan install \
            -s build_type=${build_type} -s cling_conan:build_type=Release \
            --build=missing \
            --profile clang \
                -e enable_tests=True \
                ..

# configure via cmake
cmake -E chdir build \
  cmake -E time cmake .. \
  -DENABLE_TESTS=TRUE \
  -DCONAN_AUTO_INSTALL=OFF \
  -DCMAKE_BUILD_TYPE=${build_type}

# build code
cmake -E chdir build \
  cmake -E time cmake --build . \
  --config ${build_type} \
  -- -j8

# run unit tests
cmake -E chdir build cmake -E time cmake --build . --config ${build_type} --target run_all_tests
```
