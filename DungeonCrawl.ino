#include "State.h"

#define PATH_COLOR dim(BLUE, 64)
#define AVATAR_COLOR GREEN
#define WALL_COLOR RED
#define FOG_COLOR WHITE
#define RESET_COLOR MAGENTA
#define STAIRS_COLOR YELLOW
#define GAME_TIME_MAX 360000 //6 minutes
//#define GAME_TIME_MAX 10000 //10 seconds
//              0     1      10   11    100       101       110       111      1000      1001      1010      1011      1100      1101      1110
enum protoc {NONE, MOVE, ASCEND, WIN, RESET, UNUSED_1, UNUSED_2, UNUSED_3, AVATAR_0, AVATAR_1, AVATAR_2, AVATAR_3, AVATAR_4, AVATAR_5, AVATAR_6};
Timer timer;
unsigned long startMillis;
bool isStairs = false;
bool won = false;
byte heading = 0;
protoc broadcastMessage = NONE;
protoc level = AVATAR_6;
State* postBroadcastState;

STATE_DEC(initS);
STATE_DEC(avatarS);
STATE_DEC(avatarEnteringS);
STATE_DEC(avatarLeavingS);
STATE_DEC(avatarAscendedS);
STATE_DEC(fogS);
STATE_DEC(pathS);
STATE_DEC(wallS);
STATE_DEC(gameOverS);
STATE_DEC(broadcastS);
STATE_DEC(broadcastIgnoreS);

void handleBroadcasts(bool handleResetRequest) {  
  if (handleResetRequest && buttonLongPressed()) {
    broadcastMessage = RESET;
    changeState(broadcastS::state);
    return;
  }
  broadcastMessage = NONE;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      protoc lastValue = getLastValueReceivedOnFace(f);
      switch(lastValue) {
        case ASCEND:
        case WIN:
        case RESET:
          broadcastMessage = lastValue;
          break;
      }
    }
  }
  if (broadcastMessage != NONE) changeState(broadcastS::state);
  //handle times up
  if (millis() - startMillis > GAME_TIME_MAX) {
    changeState(gameOverS::state);
  }
}

STATE_DEF(avatarS, 
  { //entry
    setValueSentOnAllFaces(level);
    setColor(OFF);
    for(uint8_t x = 0; x < level - AVATAR_0 ; ++ x) {
      setColorOnFace(AVATAR_COLOR, x);
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
    handleBroadcasts(true);
  }
)

STATE_DEF(avatarLeavingS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    setColor(PATH_COLOR);
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  },
  { //loop
    if (!isValueReceivedOnFaceExpired(heading)) {
      // if neighbor is sending avatar then the avatar has succesfully moved
      if ((getLastValueReceivedOnFace(heading) & AVATAR_0) == AVATAR_0) changeState(pathS::state);
      //TODO this might be tricky when the avatar moved to stairs...
    }
    handleBroadcasts(true);
  }
)

STATE_DEF(avatarEnteringS, 
  { //entry
    setValueSentOnFace(MOVE, heading); //request the avatar move here
    setColor(PATH_COLOR);
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  },
  { //loop
    if (getLastValueReceivedOnFace(heading) == NONE) { //after avatar is confirmed to be here then transition to actual Avatar state
      if ( isStairs) {
        level = level - 1;
        changeState(avatarAscendedS::state);
      } else {
        changeState(avatarS::state);
      }
    }
    handleBroadcasts(true);
  }
)

STATE_DEF(avatarAscendedS, 
  { //entry
    timer.set(750);
    if (level <= AVATAR_0) {//we won
      won = true;
      broadcastMessage = WIN;
    } else {
      broadcastMessage = ASCEND;
    }
    setColor(OFF);
    setColorOnFace(AVATAR_COLOR, 0);
    setValueSentOnAllFaces(broadcastMessage);
  },
  { //loop
    if (timer.isExpired()) {
      isStairs = false;
      changeState(avatarS::state);
    }
  }
)

STATE_DEF(fogS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    setColor(FOG_COLOR);
  },
  { //loop

    FOREACH_FACE(f) { //check if avatar is on neighbor
      if (!isValueReceivedOnFaceExpired(f)) {
        if((getLastValueReceivedOnFace(f) & AVATAR_0) == AVATAR_0) {//next to avatar become path or wall or stairs
          byte chance = random(20);
          isStairs = chance == 0;
          if (chance < 10) changeState(pathS::state);
          else changeState(wallS::state);
        }
      }
    }
  
    //animate time remaining    
    byte blinkFace = (millis() - startMillis) / (GAME_TIME_MAX / 6);
    Color  on = FOG_COLOR;
    Color off = dim(FOG_COLOR,  32);
    Color blinq = millis() % 1000 / 500 == 0 ? off : on;
    FOREACH_FACE(f) {
           if (f < blinkFace) setColorOnFace(  off, f);
      else if (f > blinkFace) setColorOnFace(   on, f);
      else                    setColorOnFace(blinq, f);
    }

    if (buttonLongPressed()) changeState(avatarS::state);

    handleBroadcasts(false);
  }
)

STATE_DEF(pathS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    if (isStairs) FOREACH_FACE(f) { setColorOnFace(dim(STAIRS_COLOR, f * (255 / 6)), f); }
    else setColor(PATH_COLOR);
    timer.set(10000); //revert to fog after a longer bit
  },
  { //loop
    if(isAlone()) { changeState(fogS::state); return; }

    if(timer.isExpired()) {
      bool avatarIsNeighbor = false;
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          protoc lastValue = getLastValueReceivedOnFace(f);
          if ((lastValue & AVATAR_0) == AVATAR_0) { // is avatar?
            avatarIsNeighbor = true;
          }
        }
      }
      if (!avatarIsNeighbor) changeState(fogS::state); //if avatar is not on any neighbor revert to fog
    }
      
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          protoc lastValue = getLastValueReceivedOnFace(f);
          if ((lastValue & AVATAR_0) == AVATAR_0) { // is avatar?
              heading = f;
              level = lastValue;
              changeState(avatarEnteringS::state);
          }
        }
      }
    }
    
    handleBroadcasts(true);
  }
)

STATE_DEF(wallS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    setColor(WALL_COLOR);
    timer.set(5000); //revert to fog after a bit
  },
  { //loop
    if(isAlone()) { changeState(fogS::state); return; }

    if(timer.isExpired()) {
      bool avatarIsNeighbor = false;
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          protoc lastValue = getLastValueReceivedOnFace(f);
          if ((lastValue & AVATAR_0) == AVATAR_0) { // is avatar?
            avatarIsNeighbor = true;
            //do nothing, stay wall
          }
        }
      }
      if (!avatarIsNeighbor) changeState(fogS::state); //if avatar is not on any neighbor revert to fog
    }
    
    handleBroadcasts(true);
  }
)

STATE_DEF(gameOverS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    if(won) { //TODO better win celebration animation
      FOREACH_FACE(f) {
        setColorOnFace(dim(STAIRS_COLOR, f * (255 / 6)), f);
      }
    } else {
      setColor(WALL_COLOR);
      FOREACH_FACE(f) {
        if (f % 2 == 0) setColorOnFace(OFF, f);
      }
    }
  },
  { //loop

    //animate 
    
    byte offset = (millis() % 1200 / 200);
    if(won) {
      FOREACH_FACE(f) {
        setColorOnFace(dim(STAIRS_COLOR, (f-offset)%6 * (255 / 6)), f);
      }
    }
    handleBroadcasts(true);
  }
)

STATE_DEF(broadcastS, 
  { //entry
    timer.set(500);
    setValueSentOnAllFaces(broadcastMessage);
    switch(broadcastMessage) {
      case ASCEND:
        setColor(FOG_COLOR);
        postBroadcastState = fogS::state;
        break;
      case WIN:
        won = true;
        setColor(STAIRS_COLOR);
        postBroadcastState = gameOverS::state;
        break;
      case RESET:
        setColor(RED);
        postBroadcastState = initS::state;
        break;
    }
  },
  { //loop
    if(timer.isExpired()) {
      changeState(broadcastIgnoreS::state);
    }
  }
)

STATE_DEF(broadcastIgnoreS,
  { //entry
    timer.set(500);
    setValueSentOnAllFaces(NONE); //stop broadcasting
    setColor(BLUE);
  },
  { //loop
    if(timer.isExpired()) { //stop ignoring
      changeState(postBroadcastState);
    }
  }
)

STATE_DEF(initS,
  { //entry
    setValueSentOnAllFaces(NONE);
    startMillis = millis();
    randomize();
    won = false;
    level = AVATAR_6;
    broadcastMessage = NONE;
    setColor(GREEN);
  },
  { //loop
    changeState(fogS::state);
  }
)

void setup() {
  changeState(initS::state);
}

void loop() {
  stateFn();
}
