#include <iostream>
#include <array>
#include <fstream>
#include <cassert>
#include <iomanip>

using namespace std ;

constexpr int bytes_per_block = 11;
using block_t = std::array<char,bytes_per_block>;

std::ofstream accel("accel.dat");
std::ofstream velo("velo.dat");
std::ofstream angle("angle.dat");

struct AccelerationDecoder {
  void decode( block_t block ){
    auto decode_val = [&](int idx1, int idx2, std::string name ){ 
      char x_low = block[idx1]; 
      char x_high = block[idx2]; 
      short x = x_low | (x_high << 8);
      double xd = x / 32768.0 * 16.0 ; 
      std::cout << name << ": " << xd << std::endl;
      return xd;
    };
    auto v0 = decode_val( 2,3,"x");
    auto v1 = decode_val( 4,5,"y");
    auto v2 = decode_val( 6,7,"z");
    accel << v0 << " " << v1 << " " << v2 << endl;
  }
};

struct VelocityDecoder {
  void decode( block_t block ){
    auto decode_val = [&](int idx1, int idx2, std::string name ){ 
      char x_low = block[idx1]; 
      char x_high = block[idx2]; 
      short x = x_low | (x_high << 8);
      double xd = x / 32768.0 * 2000 ; 
      std::cout << name << ": " << xd << std::endl;
      return xd;
    };
    auto v0 = decode_val( 2,3,"x");
    auto v1 = decode_val( 4,5,"y");
    auto v2 = decode_val( 6,7,"z");
    velo << v0 << " " << v1 << " " << v2 << endl;
  }
};

struct AngleDecoder {
  void decode( block_t block ){
    auto decode_val = [&](int idx1, int idx2, std::string name ){ 
      char x_low = block[idx1]; 
      char x_high = block[idx2]; 
      short x = x_low | (x_high << 8);
      double xd = x / 32768.0 * 180 ; 
      std::cout << name << ": " << xd << std::endl;
      return xd;
    };
    auto v0 = decode_val( 2,3,"x");
    auto v1 = decode_val( 4,5,"y");
    auto v2 = decode_val( 6,7,"z");
    angle << v0 << " " << v1 << " " << v2 << endl;
  }
};

struct Decoder {

  enum {
    ACCELERATION = 0x51,
    VELOCITY = 0x52,
    ANGLE = 0x53
  };

  void decode( block_t block ){
    std::cout << "entering " << __PRETTY_FUNCTION__ << std::endl;
    if ( (block[0] ^ 0x55) == 0 ){
      std::cout << "block starts with 0x55" << std::endl;
    }

    if ( (block[1] ^ ACCELERATION) == 0x00 ) {
      cout << "acceleration" << endl;
      AccelerationDecoder ad;
      ad.decode ( block );
    }
    if ( (block[1] ^ VELOCITY) == 0x00 ) {
      cout << "velo" << endl;
      VelocityDecoder vd;
      vd.decode( block );
    }
    if ( (block[1] ^ ANGLE) == 0x00 ) {
      cout << "angle" << endl;
      AngleDecoder ad;
      ad.decode( block );
    }
    printf("block %x\n", block[1]);
    printf("accel %x\n", ACCELERATION);
    std::cout << "leaving " << __PRETTY_FUNCTION__ << std::endl;
  }
};

int main(int argc, char** argv){

  std::ifstream in(argv[1], std::ios::binary);
  assert( in.good() );

  char value;
  bool once = true;
  int block_idx = 0;
  block_t block;
  while ( in >> value ){
    //printf("value: %x\n", value);
    if ( (value ^ 0x55) == 0) {
      block_idx = 0;
      Decoder d;
      d.decode( block );
    }
#if 0
    if ( once ){ 
      if ( !((value ^ 0x55) == 0) ) { 
	cout << "is 0x55" << std::endl;
	continue;
      }else{
	once = false;
      }
    }
#endif
    block[block_idx++] = value;
#if 0
    if ( block_idx == bytes_per_block ) {
      block_idx = 0;
      Decoder d;
      d.decode( block );
    } 
#endif
  }

  return 0;
}
