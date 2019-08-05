#ifndef PRINT_H
#define PRINT_H

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

void InitPrint();
char GetCharWidth(char c);
void SetPrintColor(char r, char g, char b, char a);
float GetStringWidth(char *string);
void PrintRectangle(float x, float y, float w, float h);
void PrintString(char *string, float x, float y, float scale, int alignment);

#endif