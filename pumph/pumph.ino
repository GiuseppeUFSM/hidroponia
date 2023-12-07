#include "pump.h"

#define PIN_SENSOR 4
#define PIN_PUMP 2
#define PIN_VALVE 27
#define PIN_EXTRA_PUMP 22

#define PIN_BTN_PLUS 26
#define PIN_BTN_MINUS 25

Pump* p;
//Pump* pe;

float jt108(float value){
  Serial.println((value - 0.1502)/0.8568);
  return (value - 0.1502)/0.8568;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //pinMode(PIN_EXTRA_PUMP, OUTPUT);
  pinMode(PIN_VALVE, OUTPUT);
  pinMode(PIN_BTN_PLUS, INPUT);
  pinMode(PIN_BTN_MINUS, INPUT);

  //analogWrite(PIN_EXTRA_PUMP, 255);
  //digitalWrite(PIN_VALVE, HIGH);
  p = new Pump(PIN_PUMP, 0);
  //pe = new Pump(PIN_EXTRA_PUMP, .8, &jt108);
  //p->setMinLevel(.3333333334);
}

void loop() {
  // put your main code here, to run repeatedly:
  static float flow = 0.0;
  static float flow_old = 0.0;

  static bool btn_plus = digitalRead(PIN_BTN_PLUS);
  static bool btn_minus = digitalRead(PIN_BTN_MINUS);
  static bool btn_plus_last = LOW;
  static bool btn_minus_last = LOW;

  btn_plus_last = btn_plus;
  btn_minus_last = btn_minus;
  btn_plus = digitalRead(PIN_BTN_PLUS);
  btn_minus = digitalRead(PIN_BTN_MINUS);
  
  if(btn_plus && btn_minus){}
  else if( (btn_plus != btn_plus_last) && btn_plus){
    p->increaseFlow(.1);
  }
  else if( (btn_minus!=btn_minus_last) && btn_minus){
    p->decreaseFlow(.1);
  }

  
  //Serial.println("Now I'll set it: " + String(flow));
  //bool test = p->increaseFlow(0.1);
  //bool test2 = pe->increaseFlow(0.1);
  //Serial.println("success? " + String(test));
  flow_old = flow;
  flow = p->getLevelPercent();
  float voltage = 3.3*flow;
  if(flow_old != flow){
    Serial.println("Voltage = " + String(voltage));
    Serial.println("Pump's flow: " + String(flow));
    Serial.println("Pump's PWM: " + String(p->getPwm()));
    delay(200);
    //Serial.println("Pump Extra's PWM: " + String(pe->getPwm()));
  }
}
