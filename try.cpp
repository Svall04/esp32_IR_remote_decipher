#include <iostream>
using namespace std;

int main() {
  for (int i = 0; i < 10; i++) {
    cout << (1 << i) << endl;
  }
  int N = 0;
  for (int i = 0; i < 10; i++) {
    N |= 1 << i;
    cout << N << endl;
  }
}
