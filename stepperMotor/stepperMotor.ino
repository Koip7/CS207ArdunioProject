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

//this value states whether or not micro stepping is used
#define MICROSTEPPING 0

bool isMicroStepping;

//The various modes the system can be in
//The angle entered while in this mode corresponds to the angle of the driven gear
#define GEAR_ANGLE_MODE 0
//The angle entered while in this mode corresponds to the angle of the motor gear
#define MOTOR_ANGLE_MODE 1
//EN is set  low allowing the motor to be manually turned the currentExpectedRotationValue is reset tp 0 when in this state
#define REST_MODE 2

short mode;
//the step angle of the motor
const float STEP_ANGLE = 1.8;
const short NUM_MICRO_STEP_ANGLES = 2;
//                                                          Full Step
const short MICROSTEP_SIG[(NUM_MICRO_STEP_ANGLES + 1) * 2] = {LOW, LOW,
//                                                          eigth Step
                                                            HIGH, HIGH};
//might not be needed since its fairly straight forward may be helpful when a larger attached gear is added though and that is the value we are shorterested in 
//the larger gear is approximately 160 and the smaller is 20 mm so when they are attached the new value of full rotation should be about 360 * 8
const short FULL_ROTATION_MOTOR = 360;
//The ratio of the driven gear to the motor gear
short fullRotationRatio = 2;
//The number of degrees the motor gear must rotate to fully rotate the driven gear
short fullRotation = FULL_ROTATION_MOTOR * fullRotationRatio;

//This value is used to store the current value of the rotation since this uses dead reckoning essentially this may be an issue in the future with float term use
//TODO find a replacement for this or some way to prove this is true maybe a LED over a light resistor every 360 degrees signal and comapre it to this value to check and ensure that this is correct
float currentExpectedRotationValue = 0;
//outputs help info to the serial port
void output_help();
//finds the most efficient route to the requested rotation (C or CC)and translates it shorto a value that step_by_angle can use
void step_to_angle(float toAngle);
//Outputs signals to make the stepper motor rotate by a specifc number of degrees.
void step_by_angle(float toAngle);
//cleans up current angle so we dont get extremely large angle values and we have data that we can actually use in the future
void update_current_angle(float angleMoved);

void setup() {
  mode = GEAR_ANGLE_MODE;
  
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  
  Serial.begin(9600); //Open Serial connection for debugging
  output_help();
  Serial.println("Enter -? for help");
  Serial.println("Begin motor control");
  //Print function list for user selection
  Serial.println("Enter angle to rotate to:");
  Serial.setTimeout(50);
  currentExpectedRotationValue = 0;
  isMicroStepping = false;
}

void loop() {
  float toAngle;
  String input;
  String option;
  short numIn;
  short dashIndex;
  short spaceIndex;
  
  while(Serial.available()){
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
            //this is with the assumption that the second part of a command will always be a number
            numIn = input.substring(spaceIndex + 1, input.length()).toInt();
            //toInt() ret 0 when the string is invalid hopefully no functions need the number 0
          }

          if (numIn == 0 && (spaceIndex != -1))
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
            Serial.print("Microstepping is: ");
            Serial.println(isMicroStepping);
          }
          else if(option == "wipe"){
            if(spaceIndex != -1){
              for(int i = 0; i < 50; i ++){
              step_to_angle(numIn);
              step_to_angle(0);
              }
            }
            else{
              Serial.print("Number of degrees that motor must turn for driven gear to turn(Gangle): ");
              Serial.println(fullRotation);
            }
          }
          else if(option == "micro"){
            isMicroStepping = !isMicroStepping;
            Serial.print("Microstepping is: ");
            Serial.println(isMicroStepping);
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
            break;
          case MOTOR_ANGLE_MODE:
            digitalWrite(EN, LOW);
            Serial.println("Motor angle mode");
            step_to_angle(numIn);
            break;
          case REST_MODE:
            Serial.println("No change in rest mode");
          }
          Serial.println("Enter new option");
          Serial.println();
        }
        //Tabbing is off here messing with me TODO:fix this
    }
  }      
}

void output_help(){
  //this seems to be taking up alot of dynamic memory (about 42& of the full capcity of the uno) for some reason find a way to reduce this 
  Serial.println(F("-g"));
  Serial.println(F("\tChanges mode to driven gear mode. The angles now inputted into the system will now be in relation to the driven gear rather than the motor gear rotation\n-g ANGLE"));
  Serial.println(F("\t Does not change mode rotates the driven gear to ANGLE, ANGLE must be greater than 0"));
  Serial.println(F("-m"));
  Serial.println(F("\tChanges mode to motor gear mode. The angles now inputted into the system will now be in relation to the motor gear rather than the driven gear rotation"));
  Serial.println(F("\t Does not change mode rotates the motor to ANGLE, ANGLE must be greater than 0"));
  Serial.println(F("-ratio RATIO_VAL"));
  Serial.println(F("\tChanges the gear ratio of the driven gear vs the motor gear to RATIO_VAL, RATIO_VAL must be greater than 0"));
  Serial.println(F("-reset"));
  Serial.println(F("\tChanges the most to rest mode allows the motor to be manually rotated\n"));
  Serial.println(F("-gangle ANGLE"));
  Serial.println(F("\tChanges the driven gear angle. ANGLE is the number of degrees the motor must rotate for the driven gear to make a full rotation"));
  Serial.println(F("-values"));
  Serial.println(F("\tOutputs the current Gangle Value and the Gear Ratio"));
  Serial.println(F("-wipe ANGLE"));
  Serial.println(F("\tRotates the motor btw 0 and ANGLE 20 times"));
  Serial.println(F("-micro"));
  Serial.println(F("\toggles wether or not the system is using micro steeping or not"));
}

void update_current_angle(float angleMoved){  
  currentExpectedRotationValue = angleMoved + currentExpectedRotationValue;

  //keeps the angle positive and under FULL_ROTATION
  //theres probally an effcient mathematiical way to do this
  //TODO: find this way
  while(currentExpectedRotationValue >= fullRotation){
    currentExpectedRotationValue -= fullRotation;
  }
  while(currentExpectedRotationValue < 0){
    currentExpectedRotationValue += fullRotation;
  }

  Serial.print("Current motor angle: ");
  Serial.println(currentExpectedRotationValue, 6);
  Serial.print("Current driven gear angle: ");
  Serial.println(currentExpectedRotationValue / fullRotationRatio, 6);
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

void step_by_angle(float toAngle){ 
  float curAngleMoved = 0;
  float stepAngle;
  //set direction to rotate
  if(toAngle >= 0){
    digitalWrite(dir, LOW);
    Serial.print("Moving CC by ");
    Serial.println(toAngle, 6);
  }
  else{
    digitalWrite(dir, HIGH);
    Serial.print("Moving C by ");
    Serial.println(toAngle,6 );
  }
  if (toAngle != 0)
  {
    /* iterate through all step modes, currently only full step might me the quality of the motor/ or driver that I'm using but even though the correct number of steps are
     * done there is still a small amount of error when using 1/8 step so sticking with only full steps at the moment and this is with the motor under no load
     */
    for (short curMicroStep = 0; curMicroStep < ((isMicroStepping == false) ? 1 : NUM_MICRO_STEP_ANGLES); curMicroStep ++){  
     int  numSteps = 0;
      stepAngle = STEP_ANGLE / ((curMicroStep == 0) ? 1 : ((float)curMicroStep * 8.0));
      Serial.print("step angle: ");
      Serial.println(stepAngle, 6);
      //setting the microstep mode of the stepper motor
      digitalWrite(MS1, MICROSTEP_SIG[curMicroStep * 2]);
      digitalWrite(MS2, MICROSTEP_SIG[curMicroStep * 2 + 1]);

      for(numSteps = 0; (curAngleMoved + stepAngle) - 0.0005f <= abs(toAngle); curAngleMoved += stepAngle, numSteps++)
      {
        digitalWrite(stp,HIGH); //Trigger one step forward
        delay(1);//need to check if this can be made smaller ot make motor move faster 
        //TODO look at an acceleration library to increase speed
        digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
        delay(1);
      }
      //Serial.println(curAngleMoved, 6);
    }
  }
  //update current rotation angle
  update_current_angle((toAngle > 0) ? curAngleMoved : -curAngleMoved);
}
