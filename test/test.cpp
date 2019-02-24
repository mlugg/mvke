#include "../mvke.hpp"

#include <string>

int main(int argc, char **argv) {
  MVKE::Instance mvke("Test Application", 1, 0, 0);
  mvke.mainLoop();
  return 0;
}
