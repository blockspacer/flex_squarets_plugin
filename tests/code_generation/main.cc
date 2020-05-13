#include <string>

int main(int argc, char* argv[]) {

  __attribute__((annotate("{gen};{funccall};squarets;")))
  std::string out = R"rawdata(
int a;
[[~]] int b;
)rawdata";

  return 0;
}
