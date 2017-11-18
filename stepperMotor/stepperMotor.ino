#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define EN  6
const float STEP_ANGLE = 1.8;

//Declare variables for functions
String user_input;
float currentExpectedRotationValue = 0;
float toAngle = 0;

void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  reset_ED_pins(); //Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Enter number for control option:");
  Serial.println("1. Turn at default microstep mode.");
  Serial.println("2. Reverse direction at default microstep mode.");
  Serial.println("3. Turn at 1/8th microstep mode.");
  Serial.println("4. Step forward and reverse directions.");
  Serial.println();
}


 //Main loop
void loop() {
  while(Serial.available()){
      toAngle = Serial.parseInt(); //Read user input and trigger appropriate function
      //toRotate = user_input.toInt();
      
      digitalWrite(EN, LOW); //Pull enable pin low to allow motor control
      step_to_angle(toAngle);
      
      reset_ED_pins();
  }
}

void step_to_angle(int toAngle){
  //if rotating C will get you toAngle quicker
  if(toAngle - currentExpectedRotationValue > 180){
    //this is using the assumption that the angle to be rotate to is always entered as a positive value may have to change this in the future
    step_by_angle(-360 + toAngle - currentExpectedRotationValue);
  }
  //if rotating CC will get you toAngle quicker
  else{
    step_by_angle(currentExpectedRotationValue - toAngle);
  }
}

//cleans up current angle so we dont get extre mely large angle values and we have data that we can actually use in the future
void update_current_angle(int angleMoved){
  //might not be needed since its fairly straight forward may be helpful when a larger attached gear is added though and that is the value we are interested in 
  const int FULL_ROTATION = 360;
  currentExpectedRotationValue += angleMoved;

  //keeps the angle positive and under FULL_ROTATION
  //theres probally an effcient mathematiical way to do this
  //TODO: find this way
  while(currentExpectedRotationValue > FULL_ROTATION || currentExpectedRotationValue < 0){
    currentExpectedRotationValue -= FULL_ROTATION;
  }

  Serial.print("Current angle: ");
  Serial.println(currentExpectedRotationValue);
}

//Outputs signals to make the stepper motor rotate by a specifc number of degrees.
void step_by_angle(int toAngle)
{ 
  //set direction to rotate
  if(toAngle >= 0){
    digitalWrite(dir, HIGH);
    //TODO: when testing make sure this is the right direction
    Serial.print("Moving CC by ");
    Serial.println(toAngle);
  }
  else{
    digitalWrite(dir, LOW);
    //TODO: when testing make sure this is the right direction
    Serial.print("Moving C by ");
    Serial.println(toAngle);
  }

  for( float curAngle = 0; curAngle <= (float)toAngle; curAngle += STEP_ANGLE)
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);//need to check if this can be made smaller ot make motor move faster 
    //TODO look at an acceleration library to increase speed
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
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
  digitalWrite(EN, HIGH);
}

