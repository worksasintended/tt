#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <vector>

using namespace std;
constexpr bool checkConsistency = false;
constexpr int blockSize = 11;
constexpr double nan = numeric_limits<double>::quiet_NaN();
// value stored in two bytes, low and hight part
auto bytesToVal(char low, char high) {
  short val = low | (high << 8);
  return val;
}

struct Angle {
  double x;
  double y;
  double z;
  double temp;
  bool consistent; // check sum correct?

  void decode(char data[]) {
    x = bytesToVal(data[2], data[3]) / 32760.0 * 180;
    y = bytesToVal(data[4], data[5]) / 32760.0 * 180;
    z = bytesToVal(data[6], data[7]) / 32760.0 * 180;
    temp = bytesToVal(data[8], data[9]) / 340. + 36.53;
    (accumulate(data, data + 10, (char)0) == data[10]) ? consistent = true
                                                       : consistent = false;
  }

  ~Angle() {}
};

struct RotationVelocity {
  double x;
  double y;
  double z;
  double temp;
  bool consistent;

  void decode(char data[]) {
    x = bytesToVal(data[2], data[3]) / 32760.0 * 2000;
    y = bytesToVal(data[4], data[5]) / 32760.0 * 2000;
    z = bytesToVal(data[6], data[7]) / 32760.0 * 2000;
    temp = bytesToVal(data[8], data[9]) / 340. + 36.53;
    (accumulate(data, data + 10, (char)0) == data[10]) ? consistent = true
                                                       : consistent = false;
  }

  ~RotationVelocity() {}
};

struct Acceleration {
  double x;
  double y;
  double z;
  double temp;
  bool consistent;

  void decode(char data[]) {
    x = bytesToVal(data[2], data[3]) / 32760.0 * 16;
    y = bytesToVal(data[4], data[5]) / 32760.0 * 16;
    z = bytesToVal(data[6], data[7]) / 32760.0 * 16;
    temp = bytesToVal(data[8], data[9]) / 340. + 36.53;
    (accumulate(data, data + 10, (char)0) == data[10]) ? consistent = true
                                                       : consistent = false;
  }

  ~Acceleration() {}
};

struct DataPoint {
  int t;
  Acceleration *acceleration;
  Angle *angle;
  RotationVelocity *rotationVelocity;

  DataPoint(int t_, Angle *angle_, Acceleration *acceleration_,
            RotationVelocity *rotationVelocity_)
      : t(t_), acceleration(acceleration_), angle(angle_),
        rotationVelocity(rotationVelocity_) {}
  ~DataPoint() {
    delete acceleration;
    delete angle;
    delete rotationVelocity;
  }
};

vector<DataPoint *> readFile(ifstream &input) {
  // read data Blocks
  vector<DataPoint *> dataPoints = {};
  int blockIdx = 0;
  int timestep = 0;
  char data[blockSize] = {0};
  char val;
  while (input >> val) {
    // hit header
    if ((val ^ 0x55) == 0) {
      Angle *angle = new Angle();
      RotationVelocity *rotationVelocity = new RotationVelocity();
      Acceleration *acceleration = new Acceleration();

      blockIdx = 0;
      data[blockIdx++] = val;
      // the demanded order  is 0x51, 0x52, 0x52
      // read 0x51
      if (((input >> val) && val ^ 0x51) == 0) {
        data[blockIdx++] = val;
        while (blockIdx < blockSize) {
          if (input >> val) { // handle file end
            data[blockIdx++] = val;
          } else {
            return dataPoints;
          }
          acceleration->decode(data);
        }
        blockIdx = 0;
      } else { // not first element of datapoint(time)
        // TODO if not acceleration restart while
        angle->x = nan;
        angle->y = nan;
        angle->z = nan;
        angle->temp = nan;
        angle->consistent = false;
      }
      // read 0x52
      if (input >> val && (val ^ 0x55) == 0 &&
          ((input >> val) && val ^ 0x52) == 0) {
        data[blockIdx++] = 0x55;
        data[blockIdx++] = val;
        while (blockIdx < blockSize) {
          if (input >> val) { // handle file end
            data[blockIdx++] = val;
          } else {
            return dataPoints;
          }
          rotationVelocity->decode(data);
        }
        blockIdx = 0;
      } else {
        rotationVelocity->x = nan;
        rotationVelocity->y = nan;
        rotationVelocity->z = nan;
        rotationVelocity->temp = nan;
        rotationVelocity->consistent = false;
      }

      // read 0x53
      if (input >> val && (val ^ 0x55) == 0 &&
          ((input >> val) && val ^ 0x53) == 0) {
        data[blockIdx++] = 0x55;
        data[blockIdx++] = val;
        while (blockIdx < blockSize) {
          if (input >> val) { // handle file end
            data[blockIdx++] = val;
          } else {
            return dataPoints;
          }
          angle->decode(data);
        }
        blockIdx = 0;
      } else {
        angle->x = nan;
        angle->y = nan;
        angle->z = nan;
        angle->temp = nan;
        angle->consistent = nan;
      }

      dataPoints.push_back(
          new DataPoint(timestep++, angle, acceleration, rotationVelocity));
    }
  }
  return dataPoints;
}

void write(vector<DataPoint *> data, ofstream &output) {
  output << "#t,xAcceleration,yAcceleration,zAcceleration,xAngle,yAngle,zAngle,"
            "xAngleVelocity,yAngleVelocity,zAngleVelocity"
         << endl;
  // print only data with valid consistency check
  if (checkConsistency) {
    for (auto date : data) {
      output << date->t << "\t";
      if (date->acceleration->consistent) {
        output << date->acceleration->x << "\t" << date->acceleration->y << "\t"
               << date->acceleration->z << "\t";
      } else {
        output << "nan"
               << "\t"
               << "nan"
               << "\t"
               << "nan"
               << "\t";
      }
      if (date->angle->consistent) {
        output << date->angle->x << "\t" << date->angle->y << "\t"
               << date->angle->z << "\t";
      } else {
        output << "nan"
               << "\t"
               << "nan"
               << "\t"
               << "nan"
               << "\t";
      }
      if (date->rotationVelocity->consistent) {
        output << date->rotationVelocity->x << "\t" << date->rotationVelocity->y
               << "\t" << date->rotationVelocity->z << endl;
      } else {
        output << "nan"
               << "\t"
               << "nan"
               << "\t"
               << "nan" << endl;
      }
    }
  } else {
    for (auto date : data) {
      output << date->t << "\t" << date->acceleration->x << "\t"
             << date->acceleration->y << "\t" << date->acceleration->z << "\t"
             << date->angle->x << "\t" << date->angle->y << "\t"
             << date->angle->z << "\t" << date->rotationVelocity->x << "\t"
             << date->rotationVelocity->y << "\t" << date->rotationVelocity->z
             << endl;
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    cout << "usage: binary <input binary> <output file>" << endl;
    return 0;
  }

  ifstream input(argv[1], ios::binary);
  if (!input.good()) {
    cout << "Bad input file!" << endl;
    return 0;
  }

  ofstream output(argv[2]);
  if (!output.good()) {
    cout << "Can't write to ouput file" << endl;
    return 0;
  }

  vector<DataPoint *> data = readFile(input);

  write(data, output);

  return 0;
}
