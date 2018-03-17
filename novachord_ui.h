#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include <math.h>
#include <gtk/gtk.h>
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "uris.h"
#include "novachordShared.h"

#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
#define s32 int32_t
#define s16 int16_t
#define s8 int8_t
#define f32 float
#define f64 double
#define null 0
#define true 1
#define false 0

#define PI 3.141592
#define WIDTH 640
#define HEIGHT 352
#define CONTROL_COUNT 15
#define TRANSFORM_STACK_SIZE 8
#define MOUSE_LEFT = 1;
#define MOUSE_MIDDLE = 2;
#define MOUSE_RIGHT = 3;

typedef enum ControlType
{
    ControlType_Dial,
    ControlType_Pedal
} ControlType;

typedef struct ControlInfo
{
    ControlType type;
    char* label;
    u32 port;
    u32 stateCount;
    char* valueLabels[7];
} ControlInfo;

ControlInfo controlData[] = {
    {ControlType_Dial, "DEEP TONE", NOVACHORD_DEEP_TONE, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "1ST RESONATOR", NOVACHORD_FIRST_RESONATOR, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "2ND RESONATOR", NOVACHORD_SECOND_RESONATOR, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "3RD RESONATOR", NOVACHORD_THIRD_RESONATOR, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "BRILLIANT TONE", NOVACHORD_BRILLIANT_TONE, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "FULL TONE", NOVACHORD_FULL_TONE, 4, {"OFF", "1", "2", "3"}},
    {ControlType_Dial, "BRIGHT/MELLOW", NOVACHORD_MELLOW, 2, {"BRIGHT", "MELLOW"}},
    {ControlType_Dial, "BALANCER", NOVACHORD_BALANCE, 3, {"SOFT BASS", "", "STRONG BASS"}},
    {ControlType_Dial, "ATTACK", NOVACHORD_ATTACK, 7, {"1", "2", "3", "4", "5", "6", "7"}},
    {ControlType_Dial, "VOLUME", NOVACHORD_VOLUME, 4, {"1", "2", "3", "4"}},
    {ControlType_Dial, "NORMAL VIBRATO", NOVACHORD_NORMAL_VIBRATO, 2, {"OFF", "ON"}},
    {ControlType_Dial, "SMALL VIBRATO", NOVACHORD_SMALL_VIBRATO, 2, {"OFF", "ON"}},

    {ControlType_Pedal, "LEFT SUSTAIN", NOVACHORD_LEFT_SUSTAIN},
    {ControlType_Pedal, "SWELL", NOVACHORD_SWELL},
    {ControlType_Pedal, "RIGHT SUSTAIN", NOVACHORD_RIGHT_SUSTAIN}
};

typedef enum InputType
{
    InputType_press,
    InputType_drag,
    InputType_release
} InputType;

typedef struct TransformStack
{
    u32 depth;
    cairo_matrix_t data[TRANSFORM_STACK_SIZE];
} TransformStack;

void transformPush(TransformStack* t, cairo_t* c);
void transformPop(TransformStack* t, cairo_t* c);

typedef struct Image
{
    cairo_surface_t* data;
    int width, height;
} Image;

Image* loadImage(char* filePath, const char* bundlePath);
void freeImage(Image* image);

typedef struct Resources
{
    Image* background;
    Image* dial;
    Image* pedal;
} Resources;

void loadResources(Resources* resources, const char* bundlePath);
void freeResources(Resources* resources);

typedef struct Input
{
    bool mouseButtons[6];
    int mouseX, mouseY, 
        mousePressX, mousePressY;
} Input;

typedef struct Control
{
    float x, y;
    float width, height, angle;
    float position, lastPosition;
    bool moving, requiresRedraw, initialized;
    ControlInfo* info;
} Control;

typedef struct NovachordUi
{
    LV2_Atom_Forge forge;
    LV2_URID_Map* map;
    NovachordUris uris;
    
    LV2UI_Write_Function write;
    LV2UI_Controller controller;

    GtkWidget* drawingArea,
             * eventBox;

    Resources resources;
    Input input;
    TransformStack transformStack;
    Control controls[15];

    bool initialized;
} NovachordUi;

void drawControl(Control* control, cairo_t* c, NovachordUi* ui);
void novachordUiInit(NovachordUi* ui);
void novachordUiDraw(NovachordUi* ui);

static void sendUiState(LV2UI_Handle handle, bool parameters);
static void sendUiDisable(LV2UI_Handle handle);
static void sendUiEnable(LV2UI_Handle handle);

static gboolean onExposeEvent(GtkWidget* widget, 
    GdkEventExpose* event, gpointer data);
static gboolean onConfigChanged(GtkWidget* widget, gpointer data);
static LV2UI_Handle instantiate(const LV2UI_Descriptor* descriptor,
    const char* pluginUri, const char* bundlePath,
    LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
    LV2UI_Widget* widget, const LV2_Feature* const* features);
static void cleanUp(LV2UI_Handle handle);
static int receiveUiState(NovachordUi* ui, const LV2_Atom_Object* object);
LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(u32 index);
