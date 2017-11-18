#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define EN  6
const float STEP_ANGLE = 1.8;
//might not be needed since its fairly straight forward may be helpful when a larger attached gear is added though and that is the value we are interested in 
//the larger gear is approximately 160 and the smaller is 20 mm so when they are attached the new value of full rotation should be about 360 * 8
const int FULL_ROTATION = 360;

//This value is used to store the current value of the rotation since this uses dead reckoning essentially this may be an issue in the future with long term use
//TODO find a replacement for this or some way to prove this is true maybe a LED over a light resistor every 360 degrees signal and comapre it to this value to check and ensure that this is correct
float currentExpectedRotationValue;

void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  reset_ED_pins(); //Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  Serial.println("Begin motor control");
  //Print function list for user selection
  Serial.println("Enter angle to rotate to:");
  digitalWrite(EN, LOW);  //Pull enable pin low to allow motor control
  currentExpectedRotationValue = 0;
}


 //Main loop
void loop() {
  float toAngle;
  while(Serial.available()){
      toAngle = Serial.parseInt(); //Read user input and trigger appropriate function
      step_to_angle(toAngle);
      reset_ED_pins();
  }
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
void step_to_angle(int toAngle){
  while(toAngle < 0){
    toAngle += FULL_ROTATION;
  }
  //if rotating C will get you toAngle quicker if the rotation travelled if travelled clockwise is greater than half a full rotation(180 if 360) then go the other way
  if(toAngle - currentExpectedRotationValue > FULL_ROTATION / 2){
    step_by_angle(-FULL_ROTATION + toAngle - currentExpectedRotationValue);
  }
  //if rotating CC will get you toAngle quicker
  else{
    step_by_angle(toAngle - currentExpectedRotationValue);
  }
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

  for(float curAngle = 0; curAngle <= (float)toAngle; curAngle += STEP_ANGLE)
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

