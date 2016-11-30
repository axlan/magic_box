#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
 
class Fade
{
  public:
    Fade(int pin, char key)
    {
      _pin = pin;
      _key = key;
      reset();
    }
    
    void update()
    {
      if (!_active) {
        return; 
      }
      if (_brightness == 0 || _brightness == 64)
      {
        _fade_amount = -_fade_amount;
      }
     
      analogWrite(_pin, _brightness);
      // change the brightness for next time through the loop:
      _brightness = _brightness + _fade_amount;
    }
    
    void check(char in)
    {
      if (!_active && in == _key) {
        _active = true;
        Serial.print(in);
        Serial.print('\n');
      }
    }
    
    bool isActive()
    {
      return _active;
    }
    void stop()
    {
      _fade_amount=0;
      analogWrite(_pin, 255);
    }
    
    void reset()
    {
      _active = false;
      _fade_amount = 1;
      _brightness = 1;
      analogWrite(_pin, 0);
    }
    
  private:
    bool _active;
    char _key;
    int _pin;
    byte _brightness;
    byte _fade_amount;
};

const byte NUM_COLORS = 5;
Fade colors[] = {{9, 'r'}, {10, 'g'}, {6, 'b'}, {5, 'u'}, {11, 'w'}};
//const byte NUM_COLORS = 1;
//Fade colors[] = {{9, 'r'}};

Servo myservo;  // create servo object to control a servo 
const byte ROM_ADDR = 0b1010000; 
const byte DATA_ADDR = 0;
const byte SERVO_PIN = 2;
const byte SERVO_OPEN = 60;
const byte SERVO_CLOSE = 10;
bool locked;
bool inserted;

char read_i2c_eeprom()
{
  Wire.beginTransmission(ROM_ADDR); // transmit to device #ROM_ADDR
  Wire.write(DATA_ADDR);             // sends value byte  
  byte ret = Wire.endTransmission();     // stop transmitting
  Wire.requestFrom(ROM_ADDR, (byte)1);    // request 1 bytes from slave device #8
 
  if (Wire.available()) { // slave may send less than requested
    return Wire.read(); // receive a byte as character
  }
}

void restoreTimer()
{
  //For Arduino Uno
  TCCR1B = 0;
  TCCR1B = _BV(CS11);
  TCCR1B = _BV(CS10);
  TCCR1A = _BV(WGM10);
  TIMSK1 = 0;
}

void servo_write(byte val)
{
  myservo.attach(SERVO_PIN);
  myservo.write(val);
  delay(1000);
  myservo.detach();
  restoreTimer();
}

void lock()
{
  locked = true;
  for(int i = 0; i < NUM_COLORS; i++) {
    colors[i].reset();
  }
  EEPROM.write(DATA_ADDR, 1);
  Serial.print("lock\n");
  servo_write(SERVO_CLOSE);
}

void unlock()
{
  locked = false;
  inserted = false;
  for(int i = 0; i < NUM_COLORS; i++) {
    colors[i].stop();
  }
  EEPROM.write(DATA_ADDR, 0);
  Serial.print("unlock\n");
  servo_write(SERVO_OPEN);
}


void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  if (EEPROM.read(DATA_ADDR)) {
    lock();
  } else {
    unlock();
  }
}


void loop() {
  if (locked) {  
    char val = read_i2c_eeprom();
    for(int i = 0; i < NUM_COLORS; i++) {
      colors[i].check(val);
    }
    
    // set the brightness of pins
    for(int i = 0; i < NUM_COLORS; i++) {
      if (!colors[i].isActive())
      {
         break; 
      }
      if (i == NUM_COLORS - 1) {
        unlock();
        return;
      }
    }
    
    // set the brightness of pins
    for(int i = 0; i < NUM_COLORS; i++) {
      colors[i].update();
    }
    delay(60);
  } else {
    if (read_i2c_eeprom()) {
      inserted = true;
      Serial.print("inserted\n");
    } else {
      Serial.print("empty\n");
      if (inserted) {
        lock();
      } 
    }
    delay(500);
  }
}
