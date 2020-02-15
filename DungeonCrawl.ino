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
  setColor(PATH_COLOR);
  setColorOnFace(AVATAR_COLOR, heading);
  revealed = true; //will become revealed path after adventurer leaves
  state = Avatar;
}

void Avatar() {
  if (buttonSingleClicked()) { //rotate right
    byte prev = heading;
    heading = (heading + 1) % 6;
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(PATH_COLOR, prev);
  }
  if (buttonDoubleClicked() || buttonMultiClicked()) { //rotate left
    byte prev = heading;
    heading = (heading + 5) % 6;
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(PATH_COLOR, prev);
  }
  if (buttonLongPressed()) {
    state = AvatarMoving0;
  }
}

void AvatarMoving0() {
  setValueSentOnFace(MOVE, heading);
  state = AvatarMoving;
}

void AvatarMoving() {
  if (!isValueReceivedOnFaceExpired(heading)) {
    switch(getLastValueReceivedOnFace(heading)) {
      case AVATAR:
        state = Path0; //avatar moved onto next path, become an empty path tile
        break;
      case WALL_REVEALED:
        state = Avatar0; //wall has been revealed, avatar is still here
        break;
      case RESET: //we are only listening on one face here, but catch this in case a reset has happened somehow
        state = ResetBroadcast0;
        break;
    }
  }
}

void Path0() {
  setValueSentOnAllFaces(PATH);
  setColor(revealed ? PATH_COLOR : FOG_COLOR);
  state = Path;
}

void Path() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case MOVE:
          state = Avatar0;
          break;
        case RESET:
          state = ResetBroadcast0;
          break;
      }
    }
  }
}

void Wall0() {
  setValueSentOnAllFaces(revealed ? WALL_REVEALED : WALL);
  setColor(revealed ? WALL_COLOR : FOG_COLOR);
  state = Wall;
}

void Wall() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case MOVE:
          revealed = true;
          state = Wall0;
          break;
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
}

void setup() {
}

void loop() {
  state();
}
