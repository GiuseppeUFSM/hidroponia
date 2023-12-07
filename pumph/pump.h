#include <analogWrite.h>


class Pump{

private:

  int level;            //The current PWM value
  float levelPercent;   //The 0.0~1.0 current flow value (estimated when setted with setPWM)
  uint8_t pin;          //The Pump's Pin
  int pwmFrequency;     //The PWM frequency
  int pwmResolution;    //The PWM resolution
  int pwmMaxLevel;

  //Values to better work with the pump's linearity section.
  float minFlow;         //The minimum value (in flow) the pump needs to work (flow equivalent to the minimal power (% of the source) the pump needs)
  float (*pumpEquation)(float); //Function that we enter the percentage 0~1 of flow we want and it gives us back the percentage 0~1 of power that we need for it.
                                //By default uses a 1 to 1 equation (the defaultEquation() method).
  
  SemaphoreHandle_t accessMutex = NULL; //A FreeRTOS Mutex to access and change the PUMP critical values (used when more than 1 task uses the same resource)
                                 //**for the future, can make a readFree WriteLock semaphore.


  bool set(int value){ //helper method to set the pwm pin
    level = constrain(value, 0, pwmMaxLevel);
    analogWrite(pin, level);
    return 1;
  }

  float calculatePwmByPercent(float value){//helper method to calculate the PWM value using the flow percentage
    float linearFlowValuePercent = (value > 0) ? ( (1.0 - this->minFlow)*constrain(value, 0.0, 1.0) ) + this->minFlow : 0;
    float v = (linearFlowValuePercent > 0.0) ? this->pumpEquation(linearFlowValuePercent)*this->pwmMaxLevel : 0.0;
    return v;
  }

public:

  Pump(uint8_t outputPin, float minimalFlow = 0.0, float (*pump_equation)(float) = &Pump::defaultEquation, int resolution = 8, int frequency = 5000) : 
    level(0), pin(outputPin), pwmResolution(resolution), pwmFrequency(frequency), minFlow(minimalFlow), pumpEquation(pump_equation)
  {
    accessMutex = xSemaphoreCreateMutex();
    if(accessMutex != NULL){
      Serial.println("Semaphore created");
      xSemaphoreGive(accessMutex);
    }
    else Serial.println("Error creating semaphore");
    this->levelPercent = 0.0;
    this->pwmMaxLevel = ((int)1 << this->pwmResolution) - 1; //for power of base 2 we can just bit-shitf leftwise
    pinMode(this->pin, OUTPUT);
    analogWriteFrequency(this->pwmFrequency);
    analogWriteResolution(this->pwmResolution);
    analogWrite(this->pin, this->level);
  }

  float getLevelPercent(){
    float lvl;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      lvl = this->levelPercent;
      xSemaphoreGive(this->accessMutex);
    }
    return lvl;
  }

  int getPwm(){
    int lvl;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      lvl = this->level;
      xSemaphoreGive(this->accessMutex);
    }
    return lvl;
  }

  bool setResolution(int resolution){
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      Serial.println("[setResolution]Semaphore Taken");
      this->pwmResolution = resolution; //may put a constrain here
      analogWriteResolution(this->pwmResolution);
      this->pwmMaxLevel = ((int)1 << this->pwmResolution) - 1;
      xSemaphoreGive(this->accessMutex);
      Serial.println("[setResolution]Semaphore Given");
      return true;
    }
    return false;
  }

  bool setFrequency(int frequency){
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      Serial.println("[setFrequency]Semaphore Taken");
      this->pwmFrequency = frequency;
      analogWriteFrequency(this->pwmFrequency);
      xSemaphoreGive(this->accessMutex);
      return true;
    }
    return false;
  }

  bool setMinFlow(float minimal){
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      Serial.println("[setMinFlow]Semaphore Taken");
      this->minFlow = constrain(minimal, 0.0, 1.0);
      xSemaphoreGive(this->accessMutex);
      return true;
    }
    return false;
  }

  bool resetPump(){
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      Serial.println("[reset]Semaphore Taken");
      bool flag = this->set(0);
      xSemaphoreGive(this->accessMutex);
      return 1;
    }
    return 0;
  }

  bool setPWM(int value){ //arbitrarelly set values
    bool done = false;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      Serial.println("[setPWM]Semaphore Taken");
      this->levelPercent = (float)(constrain(value, 0, this->pwmMaxLevel)) / this->pwmMaxLevel;
      done = this->set(value);
      xSemaphoreGive(this->accessMutex);
    }
    Serial.println("Level: " + String(this->level));
    return done;
  }

  bool setByFlowPercent(float value){ //the percent is considered in the linearity
    bool done = false;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      this->levelPercent = constrain(value, 0.0, 1.0);
      float v = calculatePwmByPercent(value);
      done = this->set(ceil(v));
      xSemaphoreGive(this->accessMutex);
    }
    return done;
  }

  bool increaseFlow(float percentage){
    bool done = false;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      this->levelPercent = constrain( (this->levelPercent + percentage), 0.0, 1.0 );
      float v = calculatePwmByPercent(this->levelPercent);
      done = this->set(ceil(v));
      xSemaphoreGive(this->accessMutex);
    }
    return done;
  }

  bool decreaseFlow(float percentage){
    bool done = false;
    if( xSemaphoreTake(this->accessMutex, portMAX_DELAY) == pdTRUE){
      this->levelPercent = constrain( (this->levelPercent - percentage), 0.0, 1.0 );
      float v = calculatePwmByPercent(this->levelPercent);
      done = this->set(ceil(v));
      xSemaphoreGive(this->accessMutex);
    }
    return done;
  }
  
  static float defaultEquation(float value){
    return value;
  }
  
};
