#include <stdio.h>

int this_is_a_global;

void foo() {
}

int main(int argc, char ** argv) {
  if (argc > 1) {
    foo();
  }
  else if (argc > 2) {
    foo();
  }
  else {
  }
  return 0;
}
