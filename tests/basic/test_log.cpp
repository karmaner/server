#include "basic/log.h"

void test_log() {
  LOG_INFO("Hello, %s\n", "World!");

}


int main() {
  
  test_log();


  return 0;
}