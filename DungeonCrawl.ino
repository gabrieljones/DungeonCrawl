typedef void (*State) ();

void Setup();
void AvatarInit();
void Avatar();
void AvatarMovingInit();
void AvatarMoving();
void PathInit();
void Path();
void WallInit();
void Wall();
void ResetInit();
void ResetIgnore();
void Reset();

State state = Setup;

#define PATH_COLOR BLUE
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
    state = AvatarInit;
  } else {
    state = random(1) ? PathInit : WallInit;
  }
}

void AvatarInit() {
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
    state = AvatarMovingInit;
  }
}

void AvatarMovingInit() {
  setValueSentOnFace(MOVE, heading);
  state = AvatarMoving;
}

void AvatarMoving() {
  if (!isValueReceivedOnFaceExpired(heading)) {
    switch(getLastValueReceivedOnFace(heading)) {
      case AVATAR:
        state = PathInit; //avatar moved onto next path, become an empty path tile
        break;
      case WALL_REVEALED:
        state = AvatarInit; //wall has been revealed, avatar is still here
        break;
      case RESET: //we are only listening on one face here, but catch this in case a reset has happened somehow
        state = ResetInit;
        break;
    }
  }
}

void PathInit() {
  setValueSentOnAllFaces(PATH);
  setColor(revealed ? PATH_COLOR : FOG_COLOR);
  state = Path;
}

void Path() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case MOVE:
          state = AvatarInit;
          break;
        case RESET:
          state = ResetInit;
          break;
      }
    }
  }
}

void WallInit() {
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
          state = WallInit;
          break;
        case RESET:
          state = ResetInit;
          break;
      }
    }
  }
  if (buttonLongPressed()) {
    state = ResetInit;
  }
}

void ResetInit() {
  setValueSentOnAllFaces(RESET);
  setColor(RESET_COLOR);
  timer.set(512);
  state = ResetIgnore;
}

//stop broadcasting reset after a bit, then ignore the reset broadcast
void ResetIgnore() {
  setValueSentOnAllFaces(NONE);
  if (timer.isExpired()) {
    timer.set(512);
    state = Reset;
  }
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
