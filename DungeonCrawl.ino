#include "State.h"

#define PATH_COLOR dim(BLUE, 64)
#define AVATAR_COLOR GREEN
#define WALL_COLOR RED
#define FOG_COLOR WHITE
#define RESET_COLOR MAGENTA
#define STAIRS_COLOR YELLOW
#define GAME_TIME_MAX 360000 //6 minutes
//#define GAME_TIME_MAX 10000 //10 seconds
#define LEVEL_MAX 6
//                 0    1    10    11     100   101          110         111          1000         1001     1010  1011 1100
enum tileTypes {NONE, FOG, PATH, WALL, STAIRS, MOVE, BECOME_PATH, BECOME_WALL, BECOME_STAIRS, BECOME_FOG, ASCEND, WIN, LOSE}; //add a treasure tile?
AM/BECOME FOG,PATH,WALL,STAIRS AVATAR 012345WL
byte gameOverState = NONE;
byte heading = 0;
Timer timer;
unsigned long startMillis;
byte level = 0;
byte neighbors[6];
bool isStairs = false;
bool avatarReceived = false;

STATE_DEC(setupAvatarS);
STATE_DEC(avatarS);
STATE_DEC(avatarEnteringS);
STATE_DEC(avatarLeavingS);
STATE_DEC(avatarLeftS);
STATE_DEC(avatarAscendingS);
STATE_DEC(avatarAscendedS);
STATE_DEC(fogS);
STATE_DEC(pathS);
STATE_DEC(stairsS);
STATE_DEC(wallS);
STATE_DEC(gameOverBroadcastS);
STATE_DEC(gameOverIgnoreS);
STATE_DEC(gameOverAvatarS);

void conformToAvatarMap() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      switch(getLastValueReceivedOnFace(f)) {
        case BECOME_FOG:
          changeState(fogS::state);
          break;
        case BECOME_WALL:
          changeState(wallS::state);
          break;
        case BECOME_PATH:
          changeState(pathS::state);
          break;
        case BECOME_STAIRS:
          changeState(stairsS::state);
          break;
        case WIN:
          gameOverState = WIN;
          changeState(gameOverBroadcastS::state);
          break;
        case LOSE:
          gameOverState = LOSE;
          changeState(gameOverBroadcastS::state);
          break;
      }
    }
  }
}

STATE_DEF(setupAvatarS,
  { //entry
    randomize();
    startMillis = millis();
  },
  { //loop
    changeState(avatarS::state);
  }
)

STATE_DEF(avatarS, 
  { //entry
    byte wallCount = 0;
    bool stairsAdj = false;
    setColor(AVATAR_COLOR);
    FOREACH_FACE(f) { //accept existing tiles
      if (!isValueReceivedOnFaceExpired(f)) {
        neighbors[f] = getLastValueReceivedOnFace(f);
        switch(neighbors[f]) {
          case WALL:
            wallCount++;
            break;
          case STAIRS:
            stairsAdj = true;
            break;
        }
      }
    }
    FOREACH_FACE(f) {// assign all the faces keeping existing neighbors
      switch(neighbors[f]) {
        case PATH:
          setValueSentOnFace(BECOME_PATH, f);
          break;
        case STAIRS:
          setValueSentOnFace(BECOME_STAIRS, f);
          break;
        case WALL:
          setValueSentOnFace(BECOME_WALL, f);
          break;
        case NONE:
        case FOG:
          byte newNeighbor = wallCount < 4 && random(2) == 0 ? BECOME_WALL : ( stairsAdj || random(9) > 0  ? BECOME_PATH : BECOME_STAIRS );
          //^^ if wallCount < 4 then 1 in 3 chance for WALL else if stairsAdj or 2 in 3 * 9 in 10 chance for PATH else 2 in 3 * 1 in 10 for STAIRS
          switch(newNeighbor) {
            case BECOME_WALL:
              wallCount++;
              break;
            case BECOME_STAIRS:
              stairsAdj = true;
              break;
          }
          setValueSentOnFace(newNeighbor, f);
          break;
      }
    }
  },
  { //loop
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case MOVE: //avatar is being pulled to neighbor
            heading = f;
            changeState(avatarLeavingS::state);
            break;
        }
      }
    }
    if (millis() - startMillis > GAME_TIME_MAX) {
      gameOverState = LOSE;
      changeState(gameOverAvatarS::state);
    }
    if (buttonLongPressed()) changeState(fogS::state);
    //animate time remaining    
    setColor(AVATAR_COLOR);
    byte blinkFace = (millis() - startMillis) / (GAME_TIME_MAX / 6);
    Color  on = AVATAR_COLOR;
    Color off = dim(AVATAR_COLOR,  32);
    Color blinq = millis() % 1000 / 500 == 0 ? off : on;
    FOREACH_FACE(f) {
           if (f < blinkFace) setColorOnFace(  off, f);
      else if (f > blinkFace) setColorOnFace(   on, f);
      else                    setColorOnFace(blinq, f);
    }
  }
)

STATE_DEF(avatarLeavingS, 
  { //entry
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  },
  { //loop
    sendAvatar(heading, level, millis() - startMillis);
    changeState(avatarLeftS::state);
  }
)

STATE_DEF(avatarLeftS, 
  { //entry
    timer.set(1000); //timeout for sending avatar datagram
    setValueSentOnAllFaces(PATH);
    setColor(OFF);
    setColorOnFace(AVATAR_COLOR, heading);
  },
  { //loop
    if (timer.isExpired()) { //avatar datagram was not acknowledged within timeout
      changeState(avatarS::state); //revert back to avatar
    }
    if (!isValueReceivedOnFaceExpired(heading)) {
      switch(getLastValueReceivedOnFace(heading)) {
        case BECOME_PATH: //avatar moved to path
          changeState(pathS::state);
          break;
        case BECOME_FOG: //avatar ascended stairs
        case WIN: 
          changeState(fogS::state);
          break;
      }
    }
  }
)

STATE_DEF(avatarEnteringS, 
  { //entry
    setValueSentOnFace(MOVE, heading); //request the avatar move here
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
    avatarReceived = false;
  },
  { //loop
    if (!avatarReceived) {
      unsigned long millisRemaining;
      byte levelReceived;
      avatarReceived = receiveAvatar(heading, &levelReceived, &millisRemaining);
      if (avatarReceived) {
        setColorOnFace(AVATAR_COLOR, (heading + 3) % 6);
        startMillis = millis() - millisRemaining;
        level = levelReceived;
      }
    }
    bool doneMoving = getLastValueReceivedOnFace(heading) == PATH;
    if (avatarReceived && doneMoving) { //after avatar is confirmed to be here then transition to actual Avatar state
      if ( isStairs) {
        changeState(avatarAscendedS::state);
      } else {
        changeState(avatarS::state);
      }
    }
  }
)

STATE_DEF(avatarAscendedS, 
  { //entry
    level++;
    setValueSentOnAllFaces(BECOME_FOG);
  },
  { //loop
    if (level >= LEVEL_MAX) {//we won
      gameOverState = WIN;
      changeState(gameOverAvatarS::state);
      return;
    }
    bool doneAscending = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f) && getLastValueReceivedOnFace(f) != FOG) {
        doneAscending = false; //wait for all neighbors to be FOG, this means the AVATAR has ascended
      }
    }
    if (doneAscending) { //after avatar is confirmed to have ascended then transition to actual Avatar state
      changeState(avatarS::state);
    }
  }
)

STATE_DEF(fogS, 
  { //entry
    setValueSentOnAllFaces(FOG);
    setColor(FOG_COLOR);
    isStairs = false;
  },
  { //loop
    conformToAvatarMap();
    if (buttonLongPressed()) changeState(setupAvatarS::state);
  }
)

STATE_DEF(pathS, 
  { //entry
    setValueSentOnAllFaces(PATH);
    setColor(PATH_COLOR);
    isStairs = false;
  },
  { //loop
    if(isAlone()) { changeState(fogS::state); return; }
    conformToAvatarMap();
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case BECOME_PATH: //a neighbor is telling us we are a path, its the avatar, attempt to pull avatar
              heading = f;
              changeState(avatarEnteringS::state);
              break;
          }
        }
      }
    }
  }
)


STATE_DEF(stairsS, 
  { //entry
    setValueSentOnAllFaces(STAIRS);
    FOREACH_FACE(f) { 
      setColorOnFace(dim(STAIRS_COLOR, f * (255 / 6)), f);
    }
    isStairs = true;
  },
  { //loop
    if(isAlone()) { changeState(fogS::state); return; }
    conformToAvatarMap();
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case BECOME_STAIRS: //a neighbor is telling us we are stairs, it might be the avatar, attempt to pull avatar
              heading = f;
              changeState(avatarEnteringS::state);
              break;
          }
        }
      }
    }
  }
)

STATE_DEF(wallS, 
  { //entry
    setValueSentOnAllFaces(WALL);
    setColor(WALL_COLOR);
    isStairs = false;
  },
  { //loop
    if(isAlone()) { changeState(fogS::state); return; }
    conformToAvatarMap();
  }
)

STATE_DEF(gameOverBroadcastS, 
  { //entry
    timer.set(500);
    setValueSentOnAllFaces(gameOverState);
    switch(gameOverState) {
      case WIN: //TODO better win celebration animation
        FOREACH_FACE(f) { 
          setColorOnFace(dim(STAIRS_COLOR, f * (255 / 6)), f);
        }
        break;
      case LOSE:
        setColor(WALL_COLOR);
        FOREACH_FACE(f) {
          if (f % 2 == 0) setColorOnFace(OFF, f);
        }
        break;
    }
  },
  { //loop
    if(timer.isExpired()) {
      changeState(gameOverIgnoreS::state);
    }
  }
)

STATE_DEF(gameOverIgnoreS,
  { //entry
    timer.set(500);
    setValueSentOnAllFaces(NONE); //stop broadcasting gameOver wave
  },
  { //loop
    if(timer.isExpired()) { //stop ignoring
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f) && getLastValueReceivedOnFace(f) != NONE) { //a neighbor is broadcasting a new state this must be a new game
          changeState(fogS::state);
        }
      }
    }
    if (buttonSingleClicked()) changeState(fogS::state);
  }
)

STATE_DEF(gameOverAvatarS, 
  { //entry
    timer.set(500);
    setValueSentOnAllFaces(gameOverState);
    setColor(AVATAR_COLOR);
  },
  { //loop
    if(timer.isExpired()) {
      setValueSentOnAllFaces(NONE); //stop broadcasting gameOver wave
    }
    if (buttonSingleClicked()) changeState(setupAvatarS::state);
    if (buttonLongPressed()) changeState(fogS::state);
  }
)

void setup() {
  changeState(fogS::state);
}

void loop() {
  stateFn();
}
