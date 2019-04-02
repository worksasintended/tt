#include <iostream>
#include <numeric>
#include <vector>
#include <limits>
#include <fstream>

using namespace std;
constexpr bool checkConsistency = true;
constexpr int blockSize = 11;
constexpr double nan = numeric_limits<double>::quiet_NaN();


enum attributeType {
  angleType = 180, 
  accelerationType = 16,
  rotationVelocityType = 2000
};


// values stored in two bytes, low and hight part
auto bytesToVal(char low, char high) {
  short val = low | (high << 8);
  return val;
}

class PhysicalAttribute {
public:
  double x, y, z, temp, type;
  bool consistent;  // check sum correct?

  PhysicalAttribute( attributeType theType ) {
    type = theType;
    setToNan();
  }

  void setToNan() {
    x = y = z = temp = nan;
    consistent = false;
  }

  void checkConsistency(char data[]) {
    (accumulate(data, data + 10, (char) 0) == data[10]) ? consistent = true : consistent = false;
  }

  void decode(char data[]) {
    x = bytesToVal(data[2], data[3]) / 32760.0 * type;
    y = bytesToVal(data[4], data[5]) / 32760.0 * type;
    z = bytesToVal(data[6], data[7]) / 32760.0 * type;
    temp = bytesToVal(data[8], data[9]) / 340. + 36.53;
    checkConsistency(data);
  }

  void filterSmallValues(double minValue) {

    for( auto& i : {&x, &y, &z}){
        if (*i< minValue){
            *i = nan;
        }
    }
  }

  void filterSmallValuesAbs(double minValue){
    if(x*x+y*y+z*z < minValue){
      x=y=z=nan;
    }
  }

  virtual ~PhysicalAttribute() = default;
};


class DataPoint {
public:
  int t;
  PhysicalAttribute* acceleration;
  PhysicalAttribute* angle;
  PhysicalAttribute* rotationVelocity;

  DataPoint(int t_, PhysicalAttribute* angle_, PhysicalAttribute* acceleration_, PhysicalAttribute* rotationVelocity_)
      : t(t_), acceleration(acceleration_), angle(angle_), rotationVelocity(rotationVelocity_) {
  }
  ~DataPoint() {
    delete acceleration;
    delete angle;
    delete rotationVelocity;
  }

 void filterSmallValuesAbs(double minValueAcceleration, double minValueAngle, double minValueRotationVelocity) {
    acceleration->filterSmallValuesAbs(minValueAcceleration);
    angle->filterSmallValuesAbs(minValueAngle);
    rotationVelocity->filterSmallValuesAbs(minValueRotationVelocity);
 }

 void filterSmallValues(double minValueAcceleration, double minValueAngle, double minValueRotationVelocity) {
    acceleration->filterSmallValues(minValueAcceleration);
    angle->filterSmallValues(minValueAngle);
    rotationVelocity->filterSmallValues(minValueRotationVelocity);
 }
};

void analyzePhysicalAttribute(PhysicalAttribute* physicalAttribute, char* data, int& blockIdx, ifstream& input) {
  char val;
  while(blockIdx < blockSize) {
    if(input >> val) {  // handle file end
      data[blockIdx++] = val;
    } else {
      return;
    }
  }
  physicalAttribute->decode(data);
  blockIdx = 0;
}

vector<DataPoint*> readFile(ifstream& input) {
  // read data Blocks
  vector<DataPoint*> dataPoints = {};
  int blockIdx = 0;
  int timestep = 0;
  char data[blockSize] = {0};
  char val;
  while(input >> val) {
    // hit header
    if((val ^ 0x55) == 0) {
      auto* angle = new PhysicalAttribute(angleType);
      auto* rotationVelocity = new PhysicalAttribute(rotationVelocityType);
      auto* acceleration = new PhysicalAttribute(accelerationType);
      blockIdx = 0;
      data[blockIdx++] = val;
      // the demanded order  is 0x51, 0x52, 0x52
      // read 0x51
      if(((input >> val) && val ^ 0x51) == 0) {
        data[blockIdx++] = 0x51;
        analyzePhysicalAttribute(acceleration, data, blockIdx, input);
      }
      // read 0x52
      if(input >> val && (val ^ 0x55) == 0 && (input >> val) && (val ^ 0x52) == 0) {
        data[blockIdx++] = 0x55;
        data[blockIdx++] = val;
        analyzePhysicalAttribute(rotationVelocity, data, blockIdx, input);
      }  // read 0x53
      if(input >> val && (val ^ 0x55) == 0 && (input >> val) && (val ^ 0x53) == 0) {
        data[blockIdx++] = 0x55;
        data[blockIdx++] = val;
        analyzePhysicalAttribute(angle, data, blockIdx, input);
      }
      // only if a single valid point was hit (may create missing time steps)
      if(acceleration->consistent || rotationVelocity->consistent || angle->consistent) {
        dataPoints.push_back(new DataPoint(timestep++, angle, acceleration, rotationVelocity));
      } else {
        delete angle;
        delete rotationVelocity;
        delete acceleration;
      }
    }
  }
  return dataPoints;
}

void writeNanPoint(ofstream& output) {
  output << nan << "\t" << nan << "\t" << nan << "\t";
}

void write(vector<DataPoint*> data, ofstream& output) {
  output << "#t,xAcceleration,yAcceleration,zAcceleration,xAngle,yAngle,zAngle,"
            "xAngulearVelocity,yAngularVelocity,zAngularVelocity"
         << endl;
  // print only data with valid consistency check
  if(checkConsistency) {
      for (DataPoint* date : data) {
          output << date->t << "\t"; 
          for (PhysicalAttribute* physicalAttribute : {date->acceleration, date->angle, date->rotationVelocity}) {
              if (physicalAttribute->consistent) {
                  for (double i : {physicalAttribute->x, physicalAttribute->y, physicalAttribute->z}) {
                      output << i << "\t";
                  }
              } else {
                  writeNanPoint(output);
              }

          }
          output << endl;
      }
  }else {
    for(auto date : data) {
      output << date->t << "\t" << date->acceleration->x << "\t" << date->acceleration->y << "\t"
             << date->acceleration->z << "\t" << date->angle->x << "\t" << date->angle->y << "\t" << date->angle->z
             << "\t" << date->rotationVelocity->x << "\t" << date->rotationVelocity->y << "\t"
             << date->rotationVelocity->z << endl;
    }
  }
}

void filterSmallValuesAbs(vector<DataPoint*> data,
                       double minValueAcceleration,
                       double minValueAngle,
                       double minValueRotationVelocity) {
  for(auto date : data) {
    date->filterSmallValuesAbs(minValueAcceleration, minValueAngle, minValueRotationVelocity);
  }
}


void filterSmallValues(vector<DataPoint*> data,
                       double minValueAcceleration,
                       double minValueAngle,
                       double minValueRotationVelocity) {
  for(auto date : data) {
    date->filterSmallValues(minValueAcceleration, minValueAngle, minValueRotationVelocity);
  }
}

void filterLowPass(vector<DataPoint*> data) {
}

void filterEnveloping(vector<DataPoint*> data) {
}

int main(int argc, char** argv) {
  if(argc != 3) {
    cout << "usage: binary <input binary> <output file>" << endl;
    return 0;
  }

  ifstream input(argv[1], ios::binary);
  if(!input.good()) {
    cout << "Bad input file!" << endl;
    return 0;
  }

  ofstream output(argv[2]);
  if(!output.good()) {
    cout << "Can't write to ouput file" << endl;
    return 0;
  }

  vector<DataPoint*> data = readFile(input);
  cout << "Data read" << endl;

  // filters
  //filterSmallValues(data, 0.1, 0, 0);

  write(data, output);
  cout << "Results written to output file" << endl;
  return 0;
}
