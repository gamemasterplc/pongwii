#ifndef GFXMGR_H
#define GFXMGR_H

#define BASE_SCREEN_WIDTH 640
#define WIDE_SCREEN_WIDTH 848
#define BASE_SCREEN_HEIGHT 480
#define SCREEN_WIDTH GetScreenWidth()
#define SCREEN_HEIGHT GetScreenHeight()
#define HALF_SCREEN_WIDTH (GetScreenWidth()/2)
#define HALF_SCREEN_HEIGHT (GetScreenHeight()/2)

//General Purpose Graphics Functions
void GFXInit();
void InitVI();
void InitGX();

//Graphics Getters
int GetRenderWidth();
int GetRenderHeight();
int GetScreenWidth();
int GetScreenHeight();
bool GetWidescreenFlag();

//Graphics Setters
void SetBGColor(u8 r, u8 g, u8 b);
void SetWidescreenFlag(bool val);

void SwapBuffers();

#endif