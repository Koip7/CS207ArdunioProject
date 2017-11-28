#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define EN  6

#define GEAR_ANGLE_MODE 0
#define MOTOR_ANGLE_MODE 1
#define REST_MODE 2
int mode = GEAR_ANGLE_MODE;

const float STEP_ANGLE = 1.8;
//might not be needed since its fairly straight forward may be helpful when a larger attached gear is added though and that is the value we are interested in 
//the larger gear is approximately 160 and the smaller is 20 mm so when they are attached the new value of full rotation should be about 360 * 8
const int FULL_ROTATION_MOTOR = 360;

int fullRotationRatio = 2;
int FULL_ROTATION = FULL_ROTATION_MOTOR * fullRotationRatio;

//This value is used to store the current value of the rotation since this uses dead reckoning essentially this may be an issue in the future with long term use
//TODO find a replacement for this or some way to prove this is true maybe a LED over a light resistor every 360 degrees signal and comapre it to this value to check and ensure that this is correct
float currentExpectedRotationValue;

void reset_ED_pins();
void output_help();
void step_to_angle(float toAngle);

void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  
  reset_ED_pins(); //Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  output_help();
  Serial.println("Enter -? for help");
  Serial.println("Begin motor control");
  //Print function list for user selection
  Serial.println("Enter angle to rotate to:");
  Serial.setTimeout(50);
  currentExpectedRotationValue = 0;
}


 //Main loop
void loop() {
  float toAngle;
  String input;
  String option;
  String sub_option;
  int numIn;
  int dash_index;
  int space_index;
  
  
  while(Serial.available()){
      digitalWrite(EN, LOW);  //Pull enable pin low to allow motor control
      input = Serial.readString(); //Read user input and trigger appropriate function
      if(input.length() > 0){
        dash_index = input.indexOf('-');
        
        if(input.length() > dash_index && dash_index != -1){
          space_index = input.indexOf(' ', dash_index);
          if(space_index == input.length())
            space_index = -1;
            
          if(space_index != -1){
            option = input.substring(dash_index + 1, space_index);
            sub_option = input.substring(space_index + 1, input.length());
            numIn = sub_option.toInt();
          }
          else{
            option = input.substring(dash_index + 1, input.length());
          }
          
          if(option == "?"){
              output_help();
          }
          else if(option == "g"){
              if(space_index == -1){
                step_to_angle(0);
                Serial.println("GEAR ANGLE MODE ACTIVATED");
                mode = GEAR_ANGLE_MODE;
              }
              else if(numIn > 0){
                step_to_angle(numIn * fullRotationRatio);
              }
              else{
                Serial.println("Expected value incorrect enter -? for help");
              }
          }
          else if(option == "m"){
              if(space_index == -1){
                step_to_angle(0);
                Serial.println("MOTOR ANGLE MODE ACTIVATED");
                mode = MOTOR_ANGLE_MODE;
              }
              else if(numIn > 0){
                step_to_angle(numIn);
              }
              else{
                Serial.println("Expected value incorrect enter -? for help");
              }
          }
          else if(option == "ratio"){
              if(space_index != -1){
                FULL_ROTATION = numIn * FULL_ROTATION_MOTOR;
              }
              else if(numIn > 0){
                step_to_angle(0);
                step_to_angle(numIn * fullRotationRatio);
              }
              else{
                Serial.println("Expected value incorrect enter -? for help");
              }
          }
          else if(option == "rest"){
            Serial.println("MOTOR REST MODE ACTIVATED");
            mode = REST_MODE;
            digitalWrite(EN, HIGH);
          }
          else if(option == "gangle"){
            if(space_index != -1 && numIn > 0){
              step_to_angle(0);
              FULL_ROTATION = numIn;
              fullRotationRatio = FULL_ROTATION / FULL_ROTATION_MOTOR;
              Serial.print("Set driven gear full rotation to: ");
              Serial.println(numIn);
            }
            else{
              Serial.print("Number of degr10ees that motor must turn for driven gear to turn: ");
              Serial.println(FULL_ROTATION);
            }
          }
         }
         else{
          numIn = input.toInt();
          switch(mode){
          case GEAR_ANGLE_MODE:
            digitalWrite(EN, LOW);
            Serial.println("Gear angle mode");
            step_to_angle(numIn * fullRotationRatio);
            reset_ED_pins();
            break;
          case MOTOR_ANGLE_MODE:
            digitalWrite(EN, LOW);
            Serial.println("Motor angle mode");
            step_to_angle(numIn);
            reset_ED_pins();
            break;
          case REST_MODE:
            Serial.println("No change in rest mode");
            reset_ED_pins();
          }
        }
    }
  }      
}


void output_help(){
  Serial.println("-g");
  Serial.println("\tChanges mode to driven gear mode. The angles now inputted into the system will now be in relation to the driven gear rather than the motor gear rotation");
  Serial.println("-g ANGLE");
  Serial.println("\t Does not change mode rotates the driven gear to ANGLE, ANGLE must be greater than 0");
  
  Serial.println("-m");
  Serial.println("\tChanges mode to motor gear mode. The angles now inputted into the system will now be in relation to the motor gear rather than the driven gear rotation");
  Serial.println("-m ANGLE");
  Serial.println("\t Does not change mode rotates the motor to ANGLE, ANGLE must be greater than 0");
  
  Serial.println("-ratio RATIO_VAL");
  Serial.println("\tChanges the gear ratio of the driven gear vs the motor gear to RATIO_VAL, RATIO_VAL must be greater than 0");

  Serial.println("-rest");
  Serial.println("\tChanges the most to rest mode allows the motor to be manually rotated\n");

  Serial.println("-gangle ANGLE");
  Serial.println("\tChanges the driven gear angle. ANGLE is the number of degrees the motor must rotate for the driven gear to make a full rotation");
}

//cleans up current angle so we dont get extre mely large angle values and we have data that we can actually use in the future
void update_current_angle(int angleMoved){
  currentExpectedRotationValue += angleMoved;

  //keeps the angle positive and under FULL_ROTATION
  //theres probally an effcient mathematiical way to do this
  //TODO: find this way
  while(currentExpectedRotationValue > FULL_ROTATION){
    currentExpectedRotationValue -= FULL_ROTATION;
  }
  while(currentExpectedRotationValue < 0){
    currentExpectedRotationValue += FULL_ROTATION;
  }

  Serial.print("Current angle: ");
  Serial.println(currentExpectedRotationValue);
}

//rotates to a specific angle where the motors starting position is used as the starting angle from which all else is measured
void step_to_angle(float toAngle){
  while(toAngle > FULL_ROTATION)
    toAngle -= FULL_ROTATION;
  while(toAngle < 0){
    toAngle += FULL_ROTATION;
  }

  toAngle -= currentExpectedRotationValue;
  if(toAngle == FULL_ROTATION || toAngle == 0) return;
  else if(abs(toAngle) > FULL_ROTATION / 2)
    if(toAngle > 0)
      toAngle -= FULL_ROTATION;
    else
      toAngle += FULL_ROTATION;

  step_by_angle(toAngle);
}

//Outputs signals to make the stepper motor rotate by a specifc number of degrees.
void step_by_angle(float toAngle)
{ 
  float curAngleMoved;
  //set direction to rotate
  if(toAngle >= 0){
    digitalWrite(dir, LOW);
    //TODO: when testing make sure this is the right direction
    Serial.print("Moving CC by ");
    Serial.println(toAngle);
  }
  else{
    digitalWrite(dir, HIGH);
    //TODO: when testing make sure this is the right direction
    Serial.print("Moving C by ");
    Serial.println(toAngle);
  }
  
  if (toAngle != 0)
  {
    for(curAngleMoved = 0; curAngleMoved <= abs((float)toAngle) /*- STEP_ANGLE*/; curAngleMoved += STEP_ANGLE)
    {
      digitalWrite(stp,HIGH); //Trigger one step forward
      delay(1);//need to check if this can be made smaller ot make motor move faster 
      //TODO look at an acceleration library to increase speed
      digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
    if (curAngleMoved != abs(toAngle))
    {
      /*micro step code to get close to the exact value since STEP_ANGLE is kinda huge the logic and everything is correct it just seems to a problem with the wiring
       * TODO: maybe add this in for getting precise movement
      digitalWrite(MS1,HIGH);
      digitalWrite(MS2,LOW);

      for(curAngleMoved = 0; curAngleMoved <= abs(toAngle) - STEP_ANGLE; curAngleMoved += MICRO_STEP_ANGLE)
      {
        digitalWrite(stp,HIGH); //Trigger one step forward
        delay(1);//need to check if this can be made smaller ot make motor move faster 
        //TODO look at an acceleration library to increase speed
        digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
        delay(1);
      }*/
    }
    
  }

  //update current rotation angle
  update_current_angle(toAngle);
  
  Serial.println("Enter new option");
  Serial.println();
}

//Reset Easy Driver pins to default states
void reset_ED_pins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  //digitalWrite(EN, HIGH);
}

