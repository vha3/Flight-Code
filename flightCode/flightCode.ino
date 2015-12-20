#include <SpriteMag.h>
#include <SpriteGyro.h>
#include <SpriteRadio.h>
#include <stdint.h>


unsigned char prn2[64] = {
  0b00000001, 0b01011110, 0b11010100, 0b01100001, 0b00001011, 0b11110011, 0b00110001, 0b01011100,
  0b01100110, 0b10010010, 0b01011011, 0b00101010, 0b11100000, 0b10100011, 0b00000000, 0b11100001,
  0b10111011, 0b10011111, 0b00110001, 0b11001111, 0b11110111, 0b11000000, 0b10110010, 0b01110101,
  0b10101010, 0b10100111, 0b10100101, 0b00010010, 0b00001111, 0b01011011, 0b00000010, 0b00111101,
  0b01001110, 0b01100000, 0b10001110, 0b00010111, 0b00110100, 0b10000101, 0b01100001, 0b01000101,
  0b00000110, 0b10100010, 0b00110110, 0b00101111, 0b10101001, 0b00011111, 0b11010111, 0b11111101,
  0b10011101, 0b01001000, 0b00011001, 0b00011000, 0b10101111, 0b00110110, 0b10010011, 0b00000000,
  0b00010000, 0b10000101, 0b00101000, 0b00011101, 0b01011100, 0b10101111, 0b01100100, 0b11011010
};

unsigned char prn3[64] = {
  0b11111101, 0b00111110, 0b01110111, 0b11010101, 0b00100101, 0b11101111, 0b00101100, 0b01101001,
  0b00101010, 0b11101001, 0b00111100, 0b11000100, 0b00000111, 0b10010011, 0b11000101, 0b00000111,
  0b00110111, 0b00011111, 0b01111011, 0b11010001, 0b10111010, 0b00000111, 0b10010000, 0b00110111,
  0b11011111, 0b01011010, 0b11101101, 0b11001000, 0b10001100, 0b01101001, 0b10010111, 0b00101001,
  0b10101100, 0b11011001, 0b11010110, 0b00011010, 0b11010110, 0b10101000, 0b00000101, 0b11010011,
  0b01101010, 0b11001011, 0b11010110, 0b01010010, 0b00111111, 0b11100111, 0b10000010, 0b10000110,
  0b01101110, 0b10011010, 0b01100101, 0b10100110, 0b00101110, 0b01010100, 0b11110100, 0b01111010,
  0b11001011, 0b00101110, 0b01100011, 0b10111111, 0b01010100, 0b11000100, 0b11010100, 0b01010100
};

struct {
  char axisone;
  unsigned char x;
  char axistwo;
  unsigned char y;
  char axisthree;
  unsigned char z;
} typedef Data;

union {
  Data d;
  char str[6];
} typedef Measurement;

SpriteMag mag = SpriteMag();
SpriteGyro gyro = SpriteGyro();
SpriteRadio m_radio = SpriteRadio(prn2, prn3);

int plusZ = 37;
int minusZ = 38;
int LED = 5;
float muhatx = 0;
float muhaty = 0;
float muhatz = 1;
float vx;
float vy;
float vz;
float vhatx;
float vhaty;
float vhatz;
float k11 = 100;
float k22 = 100;
float k33 = 0;
float dotprod;
float thresh = .0006;
Data omega;
Measurement meas;

void setup() {
  gyro.init();
  mag.init();
  m_radio.txInit();
  pinMode(LED, OUTPUT);
  pinMode(plusZ, OUTPUT);
  pinMode(minusZ, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  
  //Values are in Guass
  MagneticField b = mag.read();
  
  //Convert to Tesla
  b.x = .0001*b.x;
  b.y = .0001*b.y;
  b.z = .0001*b.z;
  
  //Cross mag measurements
  //with coil direction
  vx = -1*b.y;
  vy = b.x;
  vz = 0;
  
  //Get unit vector
  vhatx = vx/(sqrt(vx*vx + vy*vy));
  vhaty = vy/(sqrt(vx*vx + vy*vy));
  vhatz = 0;
  
  //Get angular velocity measurements (rad/sec)
  AngularVelocity w = gyro.read();
  w.x = (float) (w.x/(14.375))*(PI/180);
  w.y = (float) (w.y/(14.375))*(PI/180);
  w.z = (float) (w.z/(14.375))*(PI/180);

    
  //Convert gyro measurements to string,
  //send them back to the Earthlings.
  omega.axisone = 'x';
  omega.x = map(w.x, -40, 40, 0, 255);
  omega.x = (unsigned char) omega.x;
  omega.axistwo = 'y';
  omega.y = map(w.y, -40, 40, 0, 255);
  omega.y = (unsigned char) omega.y;
  omega.axisthree = 'z';
  omega.z = map(w.z, -40, 40, 0, 255);
  omega.z = (unsigned char) omega.z;
  meas.d = omega;
  digitalWrite(LED, HIGH);
//  m_radio.transmit(meas.str, 6);
  digitalWrite(LED, LOW);
  
  
      //Print those out to make sure they
  //look ok.
  Serial.print("Wx: ");
  Serial.print((unsigned int) omega.x);
  Serial.print("    Wy:");
  Serial.print((unsigned int) omega.y);
  Serial.print("    Wz: ");
  Serial.println((unsigned int) omega.z);
  
  
  //Gain the angular velocity
  w.x = k11*w.x;
  w.y = k22*w.y;
  w.z = k33*w.z;
  
  //Dot gained angular velocity with vhat
  dotprod = w.x*vhatx + w.y*vhaty + w.z*vhatz;
  
  //Check magnitude of current. If greater than
  //minimium possible current, apply that current.
  //If not, apply no current.
  if (abs(dotprod) > thresh) {
    if(dotprod/abs(dotprod) < 0) {
      digitalWrite(plusZ, LOW);
      digitalWrite(minusZ, HIGH);
    }
    else {
      digitalWrite(minusZ, LOW);
      digitalWrite(plusZ, HIGH);
    }
  } else {
    digitalWrite(minusZ, LOW);
    digitalWrite(plusZ, LOW);
  }
  
  //Print
//  Serial.println("");
  
  //Short delay
  delay(100);
}
