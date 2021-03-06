/*
Copyright (C) 2015 Felipe Izzo

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//FIX ME: load all hardcoded values from file

#include "quakedef.h"

#include <3ds.h>
#include "touch_ctr.h"

//Keyboard is currently laid out on a 14*4 grid of 20px*20px boxes for lazy implementation
char keymap[14 * 6] = {
  K_ESCAPE , K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, 0,
  '`' , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', K_BACKSPACE,
  K_TAB, 'q' , 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '|',
  0, 'a' , 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', K_ENTER, K_ENTER,
  K_SHIFT, 'z' , 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, K_UPARROW, 0,
  0, 0 , 0, 0, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, 0, K_LEFTARROW, 	K_DOWNARROW, K_RIGHTARROW
};

u16* touchpadOverlay;
u16* keyboardOverlay;
char lastKey = 0;
int tmode;
u16* tfb;
touchPosition oldtouch, touch;
u64 tick;

u64 lastTap = 0;
void Touch_TouchpadTap(){
  u64 thisTap = Sys_FloatTime();
  if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 0 && oldtouch.px < 45){
    Key_Event('`', true);
    lastKey = '`';
  }
  else if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 270 && oldtouch.px < 320){
    tmode = 2;
    Touch_DrawOverlay();
  }
  else if ((thisTap - lastTap) < 0.5){
    Key_Event(K_SPACE, true);
    lastKey = K_SPACE;
  }
  lastTap = thisTap;
}

int shiftToggle = 0;
void Touch_KeyboardTap(){
  if(oldtouch.py > 5 && oldtouch.py < 138 && oldtouch.px > 5 && oldtouch.px < 315){
    char key = keymap[((oldtouch.py - 6) / 22) * 14 + (oldtouch.px - 6)/22];
    if(key == K_SHIFT){
      shiftToggle = !shiftToggle;
      Key_Event(K_SHIFT,shiftToggle);
      Touch_DrawOverlay();
    }
    else {
      Key_Event(key, true);
      lastKey = key;
    }
  }

  if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 270 && oldtouch.px < 320){
    tmode = 1;
    shiftToggle = 0;
    Key_Event(K_SHIFT,false);
    Touch_DrawOverlay();
  }
}

void Touch_ProcessTap(){
  if(tmode == TMODE_TOUCHPAD)
    Touch_TouchpadTap();
  else
    Touch_KeyboardTap();
}

void Touch_DrawOverlay(){
  u16* overlay = 0;
  if(tmode == TMODE_TOUCHPAD)
    overlay = touchpadOverlay;
  else
    overlay = keyboardOverlay;

  if(!overlay)
    return;
  int x,y;

	for(x=0; x<320; x++){
		for(y=0; y<240;y++){
			tfb[(x*240 + (239 - y))] = overlay[(y*320 + x)];
		}
	}

  if(tmode == TMODE_KEYBOARD && shiftToggle == 1){
    for(x=20; x<24; x++){
  		for(y=98; y<102;y++){
  			tfb[(x*240 + (239 - y))] = RGB8_to_565(0,255,0);
  		}
  	}
  }

}

void Touch_Init(){
  tmode = TMODE_TOUCHPAD; //Start in touchpad Mode
  tfb = (u16*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

  //Load overlay files from sdmc for easier testing
  FILE *texture = fopen("touchpadOverlay.bin", "rb");
  if(!texture)
    Sys_Error("Could not open touchpadOverlay.bin\n");
  fseek(texture, 0, SEEK_END);
  int size = ftell(texture);
  fseek(texture, 0, SEEK_SET);
  touchpadOverlay = malloc(size);
  fread(touchpadOverlay, 1, size, texture);
  fclose(texture);

  texture = fopen("keyboardOverlay.bin", "rb");
  if(!texture)
    Sys_Error("Could not open keyboardOverlay.bin\n");
  fseek(texture, 0, SEEK_END);
  size = ftell(texture);
  fseek(texture, 0, SEEK_SET);
  keyboardOverlay = malloc(size);
  fread(keyboardOverlay, 1, size, texture);
  fclose(texture);
}

void Touch_Update(){
  if(lastKey){
    Key_Event(lastKey, false);
    lastKey = 0;
  }

  if(hidKeysDown() & KEY_TOUCH){
    hidTouchRead(&oldtouch);
    tick = Sys_FloatTime();
  }

  //If touchscreen is released in certain amount of time it's a tap
  if(hidKeysUp() & KEY_TOUCH){
    if((Sys_FloatTime() - tick) < 1.0) //FIX ME: find optimal timeframe
      Touch_ProcessTap();
  }
}
