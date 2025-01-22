#include <iostream>

class xorshiftrplus {
  
public:
  long long int seeds[2];

  // default seed values
  xorshiftrplus() {
  }

  long long int generate(){
    long long int x = this->seeds[0];
    const long long int y = this->seeds[1]; 

    this->seeds[0] = y;
    x ^= x << 23;
    x ^= x >> 17;
    x ^= y;
    this->seeds[1] = x + y;

    return x;
  }

};

