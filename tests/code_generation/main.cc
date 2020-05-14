#include <string>

// provide template code in __VA_ARGS__
/// \note you can use \n to add newline
/// \note does not support #define, #include in __VA_ARGS__
#define $squarets(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))

// squaretsString
/// \note may use `#include` e.t.c.
// example:
//   $squaretsString("#include <cling/Interpreter/Interpreter.h>")
#define $squaretsString(...) \
  __attribute__((annotate("{gen};{squarets};CXTPL;" __VA_ARGS__)))

// example:
//   $squaretsFile("file/path/here")
/// \note FILE_PATH can be defined by CMakeLists
/// and passed to flextool via
/// --extra-arg=-DFILE_PATH=...
#define $squaretsFile(...) \
  __attribute__((annotate("{gen};{squaretsFile};CXTPL;" __VA_ARGS__)))

// uses Cling to execute arbitrary code at compile-time
// and run squarets on result returned by executed code
#define $squaretsCodeAndReplace(...) \
  /* generate definition required to use __attribute__ */ \
  __attribute__((annotate("{gen};{squaretsCodeAndReplace};CXTPL;" #__VA_ARGS__)))

int main(int argc, char* argv[]) {

  {
    // squarets will generate code from template
    // and append it after annotated variable
    $squarets(
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
    $squaretsString(
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
    $squaretsString(
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
    $squaretsCodeAndReplace(
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
    $squaretsFile(
      TEST_TEMPLATE_FILE_PATH
    )
    std::string out{""};
  }

  return 0;
}
