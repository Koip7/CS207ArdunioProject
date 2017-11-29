/* Stepper motor control by JSmithers
 *  
 * This program allows for a user to send angles to a stepper motor and ensures that the most efficient path is taken to get to the angle inputted, clockwise or counter clockwise rotation
 * The program currently assumes that the stepper motor driving a gear that is two times larger than it but this can be changed with the commands -ratio ANGLE or -gangle ANGLE commands
 */

//Motor pin definitions
#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define EN  6

//The various modes the system can be in
//The angle entered while in this mode corresponds to the angle of the driven gear
#define GEAR_ANGLE_MODE 0
//The angle entered while in this mode corresponds to the angle of the motor gear
#define MOTOR_ANGLE_MODE 1
//EN is set  low allowing the motor to be manually turned the currentExpectedRotationValue is reset tp 0 when in this state
#define REST_MODE 2
int mode = GEAR_ANGLE_MODE;

//the step angle of the motor
const float STEP_ANGLE = 0.225;
//might not be needed since its fairly straight forward may be helpful when a larger attached gear is added though and that is the value we are interested in 
//the larger gear is approximately 160 and the smaller is 20 mm so when they are attached the new value of full rotation should be about 360 * 8
const int FULL_ROTATION_MOTOR = 360;
//The ratio of the driven gear to the motor gear
int fullRotationRatio = 2;
//The number of degrees the motor gear must rotate to fully rotate the driven gear
int fullRotation = FULL_ROTATION_MOTOR * fullRotationRatio;
//This value is used to store the current value of the rotation since this uses dead reckoning essentially this may be an issue in the future with long term use
//TODO find a replacement for this or some way to prove this is true maybe a LED over a light resistor every 360 degrees signal and comapre it to this value to check and ensure that this is correct
float currentExpectedRotationValue;

//Reset Easy Driver pins to default states
void reset_ED_pins();
//outputs help info to the serial port
void output_help();
//rotates to a specific angle where the motors starting position (currentExpectedRotationValue) is used as the starting angle from which all else is measured
//finds the most efficient route to the requested rotation (C or CC)and translates it into a value that step_by_angle can use
void step_to_angle(float toAngle);
//Outputs signals to make the stepper motor rotate by a specifc number of degrees.
void step_by_angle(float toAngle);
//cleans up current angle so we dont get extremely large angle values and we have data that we can actually use in the future
void update_current_angle(int angleMoved);

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

void loop() {
  float toAngle;
  String input;
  String option;
  String subOption;
  int numIn;
  int dashIndex;
  int spaceIndex;
  
  while(Serial.available()){
      digitalWrite(EN, LOW);  //Pull enable pin low to allow motor control
      input = Serial.readString(); //Read user input and trigger appropriate function

      //This is the parsing of the input
      /*Currently its pretty inefficient
        TODO:Make this better
      */
      if(input.length() > 0){
        dashIndex = input.indexOf('-');
        if(input.length() > dashIndex && dashIndex != -1){
          spaceIndex = input.indexOf(' ', dashIndex);
          if(spaceIndex == input.length())
            spaceIndex = -1;

          
          if(spaceIndex != -1){
            subOption = input.substring(spaceIndex + 1, input.length());
            //this is with the assumption that the second part of a command will always be a number
            numIn = subOption.toInt();
            //toInt() ret 0 when the string is invalid hopefully no functions need the number 0
          }

          if (numIn == 0)
          {
            option = input.substring(dashIndex + 1, input.length());
            spaceIndex = -1;
            Serial.println("Expected value incorrect enter -? for help");
          }
          else
            option = input.substring(dashIndex + 1, spaceIndex);
            
          
          if(option == "?"){
              output_help();
          }
          else if(option == "g"){
              if(spaceIndex == -1){
                step_to_angle(0);
                Serial.println("GEAR ANGLE MODE ACTIVATED");
                mode = GEAR_ANGLE_MODE;
              }
              else{
                step_to_angle(numIn * fullRotationRatio);
              }
          }
          else if(option == "m"){
              if(spaceIndex == -1){
                step_to_angle(0);
                Serial.println("MOTOR ANGLE MODE ACTIVATED");
                mode = MOTOR_ANGLE_MODE;
              }
              else{
                step_to_angle(numIn);
              }
          }
          else if(option == "ratio"){
            step_to_angle(0);
            fullRotationRatio = numIn;
            fullRotation = numIn * FULL_ROTATION_MOTOR;
          }
          else if(option == "reset"){
            Serial.println("MOTOR REST MODE ACTIVATED");
            currentExpectedRotationValue = 0;
            mode = REST_MODE;
            digitalWrite(EN, HIGH);
          }
          else if(option == "gangle"){
            if(spaceIndex != -1){
              step_to_angle(0);
              fullRotation = numIn;
              fullRotationRatio = fullRotation / FULL_ROTATION_MOTOR;
              Serial.print("Set driven gear full rotation to: ");
              Serial.println(numIn);
            }
            else{
              Serial.print("Number of degrees that motor must turn for driven gear to turn(Gangle): ");
              Serial.println(fullRotation);
            }
          }
          else if(option == "values"){
            Serial.print("Gangle: ");
            Serial.println(fullRotation);
            Serial.print("Gear ratio: ");
            Serial.println(fullRotationRatio);
          }
          }
        else{
          //If no input functions were activated just act according to the current mode
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
          Serial.println("Enter new option");
          Serial.println();
        }
        //Tabbing is off here messing with me TODO:fix this
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
  Serial.println("\t Does not change mode rotates the motor to ANGLE, ANGLE must be greater than 0");
  Serial.println("-ratio RATIO_VAL");
  Serial.println("\tChanges the gear ratio of the driven gear vs the motor gear to RATIO_VAL, RATIO_VAL must be greater than 0");
  Serial.println("-reset");
  Serial.println("\tChanges the most to rest mode allows the motor to be manually rotated\n");
  Serial.println("-gangle ANGLE");
  Serial.println("\tChanges the driven gear angle. ANGLE is the number of degrees the motor must rotate for the driven gear to make a full rotation");
  Serial.println("-values");
  Serial.println("\tOutputs the current Gangle Value and the Gear Ratio");
}

void update_current_angle(int angleMoved){
  currentExpectedRotationValue += angleMoved;

  //keeps the angle positive and under FULL_ROTATION
  //theres probally an effcient mathematiical way to do this
  //TODO: find this way
  while(currentExpectedRotationValue > fullRotation){
    currentExpectedRotationValue -= fullRotation;
  }
  while(currentExpectedRotationValue < 0){
    currentExpectedRotationValue += fullRotation;
  }

  Serial.print("Current motor angle: ");
  Serial.println(currentExpectedRotationValue);
  Serial.print("Current driven gear angle: ");
  Serial.println(currentExpectedRotationValue / fullRotationRatio);
}

void step_to_angle(float toAngle){
  while(toAngle > fullRotation)
    toAngle -= fullRotation;
  while(toAngle < 0){
    toAngle += fullRotation;
  }

  toAngle -= currentExpectedRotationValue;
  if(toAngle == fullRotation || toAngle == 0) return;
  else if(abs(toAngle) > fullRotation / 2)
    if(toAngle > 0)
      toAngle -= fullRotation;
    else
      toAngle += fullRotation;

  step_by_angle(toAngle);
}

void step_by_angle(float toAngle)
{ 
  float curAngleMoved;
  //set direction to rotate
  if(toAngle >= 0){
    digitalWrite(dir, LOW);
    Serial.print("Moving CC by ");
    Serial.println(toAngle);
  }
  else{
    digitalWrite(dir, HIGH);
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
    /*micro step code to get close to the exact value since STEP_ANGLE is kinda huge the logic and everything is correct it just seems to a problem with the wiring
    * TODO: maybe add this in for getting precise movement
    if (curAngleMoved != abs(toAngle))
    {
      digitalWrite(MS1,HIGH);
      digitalWrite(MS2,LOW);

      for(curAngleMoved = 0; curAngleMoved <= abs(toAngle) - STEP_ANGLE; curAngleMoved += MICRO_STEP_ANGLE)
      {
        digitalWrite(stp,HIGH); //Trigger one step forward
        delay(1);//need to check if this can be made smaller ot make motor move faster 
        //TODO look at an acceleration library to increase speed
        digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
        delay(1);
      }
    }*/
  }
  //update current rotation angle
  update_current_angle(toAngle);
}

void reset_ED_pins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
}
