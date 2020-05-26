# About

Plugin for [https://github.com/blockspacer/flextool](https://github.com/blockspacer/flextool)

See for details [https://blockspacer.github.io/flex_docs/](https://blockspacer.github.io/flex_docs/)

Plugin allows to embed template engine into C++ code.

Default template engine syntax (CXTPL) described at [https://github.com/blockspacer/CXTPL](https://github.com/blockspacer/CXTPL)

Template engine output can be used to produce valid C++ code.

Approach similar to javascript template literals:

```js
// javascript template literals
const myVariable = 'test'
const mystring = `something ${myVariable}` //something test
```

Default template engine syntax (CXTPL) allows to rewrite code above as:

```cpp
// custom C++ template syntax using flex_squarets_plugin
// After code generation `mystring` will store text `something test`.
const std::string myVariable = "test";
_squaretsString(
  R"raw(
    something [[+ myVariable +]]
  )raw"
)
std::string mystring;
``

Where `_squaretsString` defined as `attribute annotate` (see `#define` below)

After code generation `mystring` will store text `something test`.

Note that generated code appends data to `mystring` via '+=', so `mystring` can not be const.

Do not forget about indentation and '\n' as newline:

```cpp
  // will add extra spaces and newlines
  R"raw(
    something [[+ myVariable +]]
  )raw"
  // no spaces or newlines
  R"raw(something [[+ myVariable +]])raw"
  // not raw string literal may require escaping (as usual)
  "something [[+ myVariable +]]"
```

## Before installation

Requires flextool

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
                -o flex_squarets_plugin:enable_clang_from_conan=False \
                -e flex_squarets_plugin:enable_tests=True \
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
cmake -E chdir build \
  cmake -E time cmake --build . \
  --config ${build_type} \
  --target flex_squarets_plugin_run_all_tests
```

## For contibutors: conan editable mode

With the editable packages, you can tell Conan where to find the headers and the artifacts ready for consumption in your local working directory.
There is no need to run `conan create` or `conan export-pkg`.

See for details [https://docs.conan.io/en/latest/developing_packages/editable_packages.html](https://docs.conan.io/en/latest/developing_packages/editable_packages.html)

Build locally:

```bash
CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
  cmake -E time \
    conan install . \
    --install-folder local_build \
    -s build_type=Debug -s cling_conan:build_type=Release \
    --profile clang \
      -o flex_squarets_plugin:enable_clang_from_conan=False \
      -e flex_squarets_plugin:enable_tests=True

CONAN_REVISIONS_ENABLED=1 \
CONAN_VERBOSE_TRACEBACK=1 \
CONAN_PRINT_RUN_COMMANDS=1 \
CONAN_LOGGING_LEVEL=10 \
GIT_SSL_NO_VERIFY=true \
  cmake -E time \
    conan source . --source-folder local_build

conan build . \
  --build-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir
```

Set package to editable mode:

```bash
conan editable add local_build/package_dir \
  flex_squarets_plugin/master@conan/stable
```

Note that `conanfile.py` modified to detect local builds via `self.in_local_cache`

After change source in folder local_build (run commands in source package folder):

```
conan build . \
  --build-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir
```

Build your test project

In order to revert the editable mode just remove the link using:

```bash
conan editable remove \
  flex_squarets_plugin/master@conan/stable
```

## Usage

Example before code generation:

```cpp
#include <string>

// provide template code in __VA_ARGS__
/// \note you can use \n to add newline
/// \note does not support #define, #include in __VA_ARGS__
#define _squarets(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))

// squaretsString
/// \note may use `#include` e.t.c.
// example:
//   _squaretsString("#include <cling/Interpreter/Interpreter.h>")
#define _squaretsString(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" __VA_ARGS__)))

// example:
//   _squaretsFile("file/path/here")
/// \note FILE_PATH can be defined by CMakeLists
/// and passed to flextool via
/// --extra-arg=-DFILE_PATH=...
#define _squaretsFile(...) \
  __attribute__((annotate("{gen};{squaretsFile};CXTPL;" __VA_ARGS__)))

// uses Cling to execute arbitrary code at compile-time
// and run squarets on result returned by executed code
#define _squaretsCodeAndReplace(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate("{gen};{squaretsCodeAndReplace};CXTPL;" #__VA_ARGS__)))

int main(int argc, char* argv[]) {

  {
    // squarets will generate code from template
    // and append it after annotated variable
    _squarets(
      int a;\n
      [[~]] int b;\n
      /// \note does not support #define, #include in __VA_ARGS__
      #define A 1
      int c = "123";\n
    )
    std::string out = "";
  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note you must use escape like \"
    /// to insert quote into "" (as usual)
    /// or you can use R"raw()raw" string
    _squaretsString(
      "int a;\n"
      "[[~]] int b;\n"
      "/// \note supports #define, #include in __VA_ARGS__"
      "#define A 1"
      "int c = \"123\";\n"
    )
    std::string out;
  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note uses R"raw()raw" string
    _squaretsString(
      R"raw(int a;
      [[~]] int b;
      /// \note supports #define, #include in __VA_ARGS__"
      #define A 1
      int c = 123;)raw"
    )
    std::string out{""};
  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note uses Cling C++ interpreter
    /// to return template code combined from multiple std::string
    /// at compile-time
    _squaretsCodeAndReplace(
      [&clangMatchResult, &clangRewriter, &clangDecl]() {
        std::string a = R"raw(int g = 123;)raw";
        std::string b = R"raw(int s = 354;)raw";
        std::string c =
          R"raw(int a;
          [[~]] int b;
          /// \note supports #define, #include in __VA_ARGS__
          #define A 1
          int c = 123;)raw";

        // you can call fopen to read template from file
        // or download template from network, etc.
        // ...

        return new llvm::Optional<std::string>{
          a + b + c
        };
      }();
    )
    std::string out{""};
  }

#if FILE_CONTENTS_COMMENT
int example1 = 1;
[[~]] std::cout << example1;
std::cout << [[* std::to_string(example1) *]];
[[~
std::string baar;
~]][[~]]/*no newline*/
std::cout << [[+ std::to_string(example1) +]];
#endif // FILE_CONTENTS_COMMENT
  {
    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    _squaretsFile(
      TEST_TEMPLATE_FILE_PATH
    )
    std::string out{""};
  }

  return 0;
}
```

Generated code:

```cpp
#include <string>

// provide template code in __VA_ARGS__
/// \note you can use \n to add newline
/// \note does not support #define, #include in __VA_ARGS__
#define _squarets(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))

// squaretsString
/// \note may use `#include` e.t.c.
// example:
//   _squaretsString("#include <cling/Interpreter/Interpreter.h>")
#define _squaretsString(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" __VA_ARGS__)))

// example:
//   _squaretsFile("file/path/here")
/// \note FILE_PATH can be defined by CMakeLists
/// and passed to flextool via
/// --extra-arg=-DFILE_PATH=...
#define _squaretsFile(...) \
  __attribute__((annotate("{gen};{squaretsFile};CXTPL;" __VA_ARGS__)))

// uses Cling to execute arbitrary code at compile-time
// and run squarets on result returned by executed code
#define _squaretsCodeAndReplace(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate("{gen};{squaretsCodeAndReplace};CXTPL;" #__VA_ARGS__)))

int main(int argc, char* argv[]) {

  {
    // squarets will generate code from template
    // and append it after annotated variable
    _squarets(
      int a;\n
      [[~]] int b;\n
      /// \note does not support #define, #include in __VA_ARGS__
      #define A 1
      int c = "123";\n
    )
    std::string out = "";
out
 +=
R"raw(int a;
 )raw"
 ;
 int b;
out
 +=
R"raw( int c = "123";
)raw"
 ;

  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note you must use escape like \"
    /// to insert quote into "" (as usual)
    /// or you can use R"raw()raw" string
    _squaretsString(
      "int a;\n"
      "[[~]] int b;\n"
      "/// \note supports #define, #include in __VA_ARGS__"
      "#define A 1"
      "int c = \"123\";\n"
    )
    std::string out;
out
 +=
R"raw(int a;
)raw"
 ;
 int b;
out
 +=
R"raw(///
ote supports #define, #include in __VA_ARGS__#define A 1int c = "123";
)raw"
 ;

  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note uses R"raw()raw" string
    _squaretsString(
      R"raw(int a;
      [[~]] int b;
      /// \note supports #define, #include in __VA_ARGS__"
      #define A 1
      int c = 123;)raw"
    )
    std::string out{""};
out
 +=
R"raw(int a;
      )raw"
 ;
 int b;
out
 +=
R"raw(      /// \note supports #define, #include in __VA_ARGS__"
      #define A 1
      int c = 123;)raw"
 ;

  }

  {
    // squarets will generate code from template
    // and append it after annotated variable
    /// \note uses Cling C++ interpreter
    /// to return template code combined from multiple std::string
    /// at compile-time
    _squaretsCodeAndReplace(
      [&clangMatchResult, &clangRewriter, &clangDecl]() {
        std::string a = R"raw(int g = 123;)raw";
        std::string b = R"raw(int s = 354;)raw";
        std::string c =
          R"raw(int a;
          [[~]] int b;
          /// \note supports #define, #include in __VA_ARGS__
          #define A 1
          int c = 123;)raw";

        // you can call fopen to read template from file
        // or download template from network, etc.
        // ...

        return new llvm::Optional<std::string>{
          a + b + c
        };
      }();
    )
    std::string out{""};
out
 +=
R"raw(int g = 123;int s = 354;int a;
          )raw"
 ;
 int b;
out
 +=
R"raw(          /// \note supports #define, #include in __VA_ARGS__
          #define A 1
          int c = 123;)raw"
 ;

  }

#if FILE_CONTENTS_COMMENT
int example1 = 1;
[[~]] std::cout << example1;
std::cout << [[* std::to_string(example1) *]];
[[~
std::string baar;
~]][[~]]/*no newline*/
std::cout << [[+ std::to_string(example1) +]];
#endif // FILE_CONTENTS_COMMENT
  {
    // squarets will generate code from template file
    // and append it after annotated variable
    /// \note FILE_PATH defined by CMakeLists
    /// and passed to flextool via
    /// --extra-arg=-DFILE_PATH=...
    _squaretsFile(
      TEST_TEMPLATE_FILE_PATH
    )
    std::string out{""};
out
 +=
R"raw(int example1 = 1;
)raw"
 ;
 std::cout << example1;
out
 +=
R"raw(std::cout << )raw"
 ;

out
 +=
 std::to_string(  std::to_string(example1)  )
 ;

out
 +=
R"raw(;
)raw"
 ;

std::string baar;

/*no newline*/
out
 +=
R"raw(std::cout << )raw"
 ;

out
 +=  std::to_string(example1)  ;

out
 +=
R"raw(;

)raw"
 ;

  }

  return 0;
}
```
