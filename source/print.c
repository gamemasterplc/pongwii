#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "gfxmgr.h"
#include "print.h"
#include "fontinfo_bin.h"
#include "font_tpl.h"
#include "font.h"

static GXTexObj font_tex_obj;
static int char_width;
static int char_height;
static unsigned char *font_info;
static GXColor print_color;

void InitPrint()
{
	font_info = (unsigned char *)&fontinfo_bin;
	TPLFile tpl;
	TPL_OpenTPLFromMemory(&tpl, (void *)font_tpl, font_tpl_size);
	TPL_GetTexture(&tpl, font, &font_tex_obj);
	GX_InitTexObjFilterMode(&font_tex_obj, GX_LINEAR, GX_LINEAR);
	char_width = font_info[11] >> 24|font_info[10] >> 16|font_info[9] >> 8|font_info[8];
	char_height = font_info[15] >> 24|font_info[14] >> 16|font_info[13] >> 8|font_info[12];
	SetPrintColor(255, 255, 255, 255);
}

char GetCharWidth(char c)
{
	return font_info[c+17];
}

void SetPrintColor(char r, char g, char b, char a)
{
	print_color.r = r;
	print_color.g = g;
	print_color.b = b;
	print_color.a = a;
}

float GetStringWidth(char *string)
{
	float w = 0;
	while(*string)
	{
		w += GetCharWidth(*string);
		string++;
	}
	return w;
}

void PrintRectangle(float x, float y, float w, float h)
{
	Mtx44 projection;
	Mtx modelview;
	GX_InvalidateTexAll();
	guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 10);
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
	GX_SetViewport(0, 0, GetRenderWidth(), GetRenderHeight(), 0, 1);
	GX_SetScissor(0, 0, GetRenderWidth(), GetRenderHeight());
	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_SetNumTevStages(1);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZCompLoc(GX_FALSE);
	GX_SetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetNumTexGens(1);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position2f32(x, y);
	GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
	GX_Position2f32(x+w, y);
	GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
	GX_Position2f32(x+w, y+h);
	GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
	GX_Position2f32(x, y+h);
	GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
	GX_End();
}

void PrintString(char *string, float x, float y, float scale, int alignment)
{
	Mtx44 projection;
	Mtx modelview;
	
	float scaled_width = char_width*scale;
	float scaled_height = char_height*scale;
	int font_tex_width = GX_GetTexObjWidth(&font_tex_obj);
	int font_tex_height = GX_GetTexObjHeight(&font_tex_obj);
	int tex_col_count = font_tex_width/char_width;
	float texcoord_x_spacing = char_width/(float)font_tex_width;
	float texcoord_y_spacing = char_height/(float)font_tex_height;
	
	GX_InvalidateTexAll();
	guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 10);
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
	GX_SetViewport(0, 0, GetRenderWidth(), GetRenderHeight(), 0, 1);
	GX_SetScissor(0, 0, GetRenderWidth(), GetRenderHeight());
	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_LoadTexObj(&font_tex_obj, GX_TEXMAP0);
	GX_SetNumTevStages(1);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZCompLoc(GX_FALSE);
	GX_SetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetNumTexGens(1);
	switch(alignment)
	{
		case ALIGN_CENTER:
			x -= GetStringWidth(string)*scale/2;
			break;
			
		case ALIGN_RIGHT:
			x -= GetStringWidth(string)*scale;
			break;
			
		default:
			break;
	}
	float curr_x = x;
	while(*string)
	{
		char c = *string;
		if(c == '\n')
		{
			y += scaled_height;
			string++;
			curr_x = x;
			continue;
		}
		int row = (c-font_info[16])/tex_col_count;
		int col = (c-font_info[16])%tex_col_count;
		float texcoord_x = col*texcoord_x_spacing;
		float texcoord_y = row*texcoord_y_spacing;
		
		texcoord_x -= (0.5f/font_tex_width);
		texcoord_y -= (0.5f/font_tex_height);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position2f32(curr_x, y);
		GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
		GX_TexCoord2f32(texcoord_x, texcoord_y);
		GX_Position2f32(curr_x+scaled_width, y);
		GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
		GX_TexCoord2f32(texcoord_x+texcoord_x_spacing-(1.0f/font_tex_width), texcoord_y);
		GX_Position2f32(curr_x+scaled_width, y+scaled_height);
		GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
		GX_TexCoord2f32(texcoord_x+texcoord_x_spacing-(1.0f/font_tex_width), texcoord_y+texcoord_y_spacing-(1.0f/font_tex_height));
		GX_Position2f32(curr_x, y+scaled_height);
		GX_Color4u8(print_color.r, print_color.g, print_color.b, print_color.a);
		GX_TexCoord2f32(texcoord_x, texcoord_y+texcoord_y_spacing-(1.0f/font_tex_height));
		GX_End();
		curr_x += GetCharWidth(c)*scale;
		string++;
	}
}