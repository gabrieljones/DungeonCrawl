typedef void (*State) ();

void Setup();
void Avatar0();
void Avatar();
void AvatarMoving0();
void AvatarMoving();
void Path0();
void Path();
void Wall0();
void Wall();
void Reset0();
void ResetIgnore();
void Reset();

State state = Setup;

#define PATH_COLOR dim(BLUE, 64)
#define AVATAR_COLOR GREEN
#define WALL_COLOR RED
#define FOG_COLOR WHITE
#define RESET_COLOR MAGENTA

enum commandStates {NONE, AVATAR, MOVE, PATH, WALL, WALL_REVEALED, RESET}; //add a treasure tile?
bool revealed = false;
byte heading = 0;
Timer timer;

void Setup() {
  revealed = false;
  randomize();
  if(startState() == START_STATE_WE_ARE_ROOT) {
    state = Avatar0;
  } else {
    state = random(1) ? Path0 : Wall0;
  }
}

void Avatar0() {
  setValueSentOnAllFaces(AVATAR);
  setColor(AVATAR_COLOR);
  revealed = true; //will become revealed path after adventurer leaves
  state = Avatar;
}

void Avatar() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case MOVE: //avatar is being pulled to neighbor revert to path
          state = Path0;
          break;
      }
    }
  }
}

void AvatarMoving0() {
  setValueSentOnFace(MOVE, heading); //tell neighbor avatar is moving here
  setColorOnFace(AVATAR_COLOR, heading);
  setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
  setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  state = AvatarMoving;
}

void AvatarMoving() {
  bool doneMoving = true;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case AVATAR: //wait for all neighbors to be not AVATARs
          doneMoving = false;
          break;
        case RESET:
          state = ResetBroadcast0;
          break;
      }
    }
  }
  if (doneMoving) { //after avatar is confirmed to be here then transition to actual Avatar state
    state = Avatar0;
  }
  if (buttonLongPressed()) {
    state = ResetBroadcast0;
  }
}

void Path0() {
  setValueSentOnAllFaces(PATH);
  setColor(revealed ? PATH_COLOR : FOG_COLOR);
  state = Path;
}

void Path() {
  if (buttonSingleClicked()) {      
    FOREACH_FACE(f) { //check if avatar is on neighbor
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case AVATAR: //reveal and pull avatar
            revealed = true;
            heading = f;
            state = AvatarMoving0;
            break;
        }
      }
    }
  }
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case RESET:
          state = ResetBroadcast0;
          break;
      }
    }
  }
  if (buttonLongPressed()) {
    state = ResetBroadcast0;
  }
}

void Wall0() {
  setValueSentOnAllFaces(revealed ? WALL_REVEALED : WALL);
  setColor(revealed ? WALL_COLOR : FOG_COLOR);
  state = Wall;
}

void Wall() {
  if (buttonSingleClicked()) {      
    FOREACH_FACE(f) { //check if avatar is on neighbor
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case AVATAR: //reveal and don't pull avatar
            revealed = true;
            heading = f;
            state = Wall0;
            break;
        }
      }
    }
  }
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case RESET:
          state = ResetBroadcast0;
          break;
      }
    }
  }
  if (buttonLongPressed()) {
    state = ResetBroadcast0;
  }
}

void ResetBroadcast0() {
  setValueSentOnAllFaces(RESET);
  setColor(RESET_COLOR);
  timer.set(512);
  state = ResetBroadcast;
}

//broadcast reset for a bit
void ResetBroadcast() {
  if (timer.isExpired()) {
    state = ResetIgnore0;
  }
}

void ResetIgnore0() {
  setValueSentOnAllFaces(NONE);
  setColor(dim(RESET_COLOR, 128));
  timer.set(512);
  state = ResetIgnore;
}

//stop broadcasting reset after a bit, then ignore the reset broadcast
void ResetIgnore() {
  if (timer.isExpired()) {
    state = Reset0;
  }
  if (buttonLongPressed()) {
    state = ResetBroadcast0;
  }
}


void Reset0() {
  timer.set(512);
  state = Reset;
}

//ignore reset wave for a bit more, then reinitialize
void Reset() {
  if (timer.isExpired()) {
    state = Setup;
  }
  if (buttonLongPressed()) {
    state = ResetBroadcast0;
  }
}

void setup() {
}

void loop() {
  state();
}
