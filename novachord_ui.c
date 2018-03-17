#include "novachord_ui.h"

/*
    TODO:
    - Implement two poll filter
    - Implement the save and load functionality

    - Measure the vibrato speed and depth
    - Compare with actual Novachord recordings and tweak values

    TIME DEPENDANT:
    - Tidy up all the code
*/

float signf(float value)
{
    return value > 0 ? 1 : value < 0 ? -1 : 0;
}

float absf(float value)
{
    return value * signf(value);
}

int clampi(int value, int min, int max)
{
    return value < min ? min : value > max ? max : value;
}

float controlPositionToAngle(int controlPositionCount, int controlPosition)
{
    float range = PI / 2.0f;
    int positions = controlPositionCount-1;
    float position = (float) controlPosition - ((float)(positions) / 2);
    float increment = range / (positions);
    
    return increment * position;
}

void drawControl(Control* control, cairo_t* c, NovachordUi* ui)
{
    if(control->info->type == ControlType_Dial)
    {
        transformPush(&ui->transformStack, c);

        cairo_translate(c, control->x, control->y);

        cairo_translate(c, 14, 16);
        cairo_rotate(c, control->angle);
        cairo_translate(c, -14, -16);
        cairo_set_source_surface(c, ui->resources.dial->data, 0, 0);
        cairo_paint(c);

        transformPop(&ui->transformStack, c);

        cairo_set_source_rgb(c, 1.0, 1.0, 0.4); 
        cairo_select_font_face(c, "arial",
            CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, 9);
        cairo_text_extents_t extents;
        cairo_text_extents(c, control->info->label, &extents);
        cairo_move_to(c, control->x + 38 - (extents.width / 2), control->y + 76);
        cairo_show_text(c, control->info->label); 

        for(int i = 0; i < control->info->stateCount; ++i)
        {
            float angle = controlPositionToAngle(control->info->stateCount, i);
            float dist = 44;
            cairo_text_extents(c, control->info->valueLabels[i], &extents);
            cairo_move_to(c, control->x + 14 + (cos(angle) * dist) + 16, 
                          control->y + 16 + (sin(angle) * dist) + (extents.height / 2));
            cairo_show_text(c, control->info->valueLabels[i]);
        }
    }
    else if(control->info->type == ControlType_Pedal)
    {
        Control* pedal = control;

        transformPush(&ui->transformStack, c);

        cairo_translate(c, pedal->x, pedal->y + (pedal->position * 20));
        cairo_set_source_surface(c, ui->resources.pedal->data, 0, 0);
        cairo_paint(c);
        
        transformPop(&ui->transformStack, c);

        cairo_set_source_rgb(c, 1.0, 1.0, 0.4); 
        cairo_select_font_face(c, "arial",
            CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, 9);
        cairo_text_extents_t extents;
        cairo_text_extents(c, pedal->info->label, &extents);
        cairo_move_to(c, pedal->x + 4 - extents.width, pedal->y + 26 - (extents.height/2));
        cairo_show_text(c, pedal->info->label); 
    }
    
}

void animateControl(Control* control)
{   
    if(control->info->type == ControlType_Dial)
    {
        float angleDelta = 
        (controlPositionToAngle(control->info->stateCount, control->position) 
            - control->angle) / 1.5f;
        if(absf(angleDelta) > 0.02)
        {
            control->angle += angleDelta;
            control->requiresRedraw = true;
        }
    }
}

void changeControlPosition(Control* control, int value)
{
    control->position += value;
    control->position = clampi(control->position + 0.5, 0, control->info->stateCount - 1);
}

void updateControlsWithInput(NovachordUi* ui, InputType inputType)
{
    Input* input = &ui->input;

    bool send = false;

    for(int i = 0; i < CONTROL_COUNT; ++i)
    {
        Control* control = &ui->controls[i];

        if(control->info->type == ControlType_Dial)
        {
            if(inputType == InputType_press 
            && input->mouseX > control->x - 16 && input->mouseY > control->y - 16 
            && input->mouseX < control->x + 64 && input->mouseY < control->y + 48)
            {
                control->moving = true;
                control->lastPosition = control->position;
            }
            else if(inputType == InputType_release)
            {
                control->moving = false;
            }
            else if(inputType == InputType_drag)
            {
                if(control->moving)
                {
                    control->position = control->lastPosition 
                        + ((input->mouseY - input->mousePressY) / 32);
                    changeControlPosition(control, 0); 

                    send = true;
                }
            }
        }
        else if(control->info->type == ControlType_Pedal)
        {
            Control* pedal = control;
            if(inputType == InputType_press 
            && input->mouseX > pedal->x && input->mouseY > pedal->y + (pedal->position * 20)
            && input->mouseX < pedal->x + 64 && input->mouseY < pedal->y + 64 + (pedal->position * 20))
            {
                pedal->moving = true;
                pedal->lastPosition = pedal->position;
            }
            else if(inputType == InputType_release)
            {
                pedal->moving = false;
            }
            else if(inputType == InputType_drag)
            {
                if(pedal->moving)
                {
                    pedal->position = pedal->lastPosition 
                        + ((float)(input->mouseY - input->mousePressY) / 128.0f);
                    if(pedal->position < 0.0)
                        pedal->position = 0.0;
                    if(pedal->position > 1.0)
                        pedal->position = 1.0;

                    pedal->requiresRedraw = true;
                    send = true;
                }
            }
        }
    }

    if(send)
        sendUiState(ui, true);
}

void novachordUiInit(NovachordUi* ui)
{
    int i, t; 
    for(t = 0; t < 2; ++t)
        for(i = 0; i < 6; ++i)
        {
            Control* control = &ui->controls[i + (t*6)];
            ControlInfo* controlInfo = &controlData[i + (t*6)];
            control->x = 40 + (i*96);
            control->y = 64 + (112 * t);
            control->position = 0;
            control->angle = 0;
            control->lastPosition = 0;
            control->requiresRedraw = true;
            control->info = controlInfo;
        }
    for(i = 0; i < 3; ++i)
    {
        Control* pedal = &ui->controls[12 + i];
        int x = 112;
        int y = 268;
        int hSpace = 80 + 64;

        pedal->x = x + (i * hSpace);
        pedal->y = y;
        pedal->position = 0.5f * i;
        pedal->lastPosition = 0;
        pedal->requiresRedraw = true;
        pedal->info = &controlData[12 + i];
    }
}

void novachordUiDraw(NovachordUi* ui)
{
    cairo_t* cairo = gdk_cairo_create(ui->drawingArea->window);
    cairo_identity_matrix(cairo);

    cairo_set_source_surface(cairo, ui->resources.background->data, 0, 0);
    cairo_paint(cairo);

    for(int i = 0; i < CONTROL_COUNT; ++i)
        drawControl(&ui->controls[i], cairo, ui);

    cairo_destroy(cairo);
}

gboolean NovachordUiAnimate(gpointer data)
{
    NovachordUi* ui = (NovachordUi*)data;
    bool redraw = false;
    
    int i, t; 
    //for(t = 0; t < 2; ++t)
    for(i = 0; i < CONTROL_COUNT; ++i)
    {
        Control* control = &ui->controls[i];
        animateControl(control);
        if(control->requiresRedraw)
        {
            redraw = true;
            control->requiresRedraw = false;
        }
    }
        
    if(redraw)
        gtk_widget_queue_draw(ui->drawingArea);
    
    return true;
}

void transformPush(TransformStack* t, cairo_t* c)
{
    if(t->depth < TRANSFORM_STACK_SIZE)
    {
        cairo_get_matrix(c, 
            &t->data[t->depth++]);
    }
    else
        printf("Warning: Tranformation stack is full before push\n");
}

void transformPop(TransformStack* t, cairo_t* c)
{
    if(t->depth > 0)
    {
        cairo_set_matrix(c, 
            &t->data[--t->depth]);
    }
    else
    {
        cairo_identity_matrix (c);
        printf("Warning: Tranformation stack is empty before pop\n");
    }
}


Image* loadImage(char* filePath, const char* bundlePath)
{
    Image* image = (Image*)malloc(sizeof(Image));

    // TODO: Unsafe, needs bounds check
    char filePathFull[2048];
    strcpy(filePathFull, bundlePath);
    strcat(filePathFull, filePath);
    
    image->data = cairo_image_surface_create_from_png(filePathFull);
    switch(cairo_surface_status(image->data))
    {
        case CAIRO_STATUS_FILE_NOT_FOUND:
            fprintf(stderr, "Novachord.lv2 UI: Could not find image: \"%s\"\n",
                filePath);
            break;
        case CAIRO_STATUS_READ_ERROR:
            fprintf(stderr, "Novachord.lv2 UI: Image corrupt: \"%s\"\n",
                filePath);
            break;
        case CAIRO_STATUS_NO_MEMORY:
            fprintf(stderr, "Novachord.lv2 UI: Ran out of memory while loading: \"%s\"\n",
                filePath);
            break;
    }
    image->width = cairo_image_surface_get_width(image->data);
    image->height = cairo_image_surface_get_height(image->data);
    
    return image;
}

void freeImage(Image* image)
{
    cairo_surface_destroy(image->data);
    free(image);
}

void loadResources(Resources* resources, const char* bundlePath)
{
    resources->background = loadImage("background.png", bundlePath);
    resources->dial = loadImage("knob.png", bundlePath);
    resources->pedal = loadImage("pedal.png", bundlePath);
}

void freeResources(Resources* resources)
{
    freeImage(resources->background);
    freeImage(resources->dial);
    freeImage(resources->pedal);
}

static void sendUiState(LV2UI_Handle handle, bool parameters)
{
    NovachordUi* ui = (NovachordUi*) handle;

    // TODO: Why does the UI never show when I remove this?
    u8 buffer[1024];
    lv2_atom_forge_set_buffer(&ui->forge, buffer, sizeof(buffer));
    
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* message = 
        (LV2_Atom*)lv2_atom_forge_object(&ui->forge, &frame, 
                                         0, ui->uris.ui_State);

    lv2_atom_forge_pop(&ui->forge, &frame);

    ui->write(ui->controller,
              NOVACHORD_CONTROL,
              lv2_atom_total_size(message),
              ui->uris.atom_eventTransfer,
              message);

    // Write each of the control values
    for(int i = 0; i < CONTROL_COUNT; ++i)
    {
        Control* control = &ui->controls[i];
        float value = control->position;
        ui->write(ui->controller, control->info->port, 4, 0, &value);
    }
}

static void sendUiDisable(LV2UI_Handle handle)
{
    // Notify backend that the UI is closed
    NovachordUi* ui = (NovachordUi*) handle;
    sendUiState(handle, true);
    
    u8 buffer[64];
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* message = 
        (LV2_Atom*)lv2_atom_forge_object(
            &ui->forge, &frame, 0, ui->uris.ui_Off);
    lv2_atom_forge_pop(&ui->forge, &frame);
    ui->write(ui->controller, NOVACHORD_CONTROL, lv2_atom_total_size(message),
              ui->uris.atom_eventTransfer, message); 
}

static void sendUiEnable(LV2UI_Handle handle)
{
    // Notify backend that the UI is closed
    NovachordUi* ui = (NovachordUi*) handle;
    sendUiState(handle, false);
    
    u8 buffer[64];
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* message = 
        (LV2_Atom*)lv2_atom_forge_object(
            &ui->forge, &frame, 0, ui->uris.ui_On);
    lv2_atom_forge_pop(&ui->forge, &frame);
    ui->write(ui->controller, NOVACHORD_CONTROL, lv2_atom_total_size(message),
              ui->uris.atom_eventTransfer, message); 
}

static gboolean onExposeEvent(GtkWidget* widget, 
    GdkEventExpose* event, gpointer data)
{
    NovachordUi* ui = (NovachordUi*) data;

    novachordUiDraw(ui);
    return true;
}


static gboolean onConfigChanged(GtkWidget* widget, gpointer data)
{
    sendUiState(data, true);
    return true;
}

static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    NovachordUi* ui = (NovachordUi*)data;
    Input* i = &ui->input;

    i->mouseButtons[event->button] = true;
    i->mouseX = event->x;
    i->mouseY = event->y;
    i->mousePressX = event->x;
    i->mousePressY = event->y;
    
    updateControlsWithInput(ui, InputType_press);
    return true;
}

static gboolean onButtonRelease(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    NovachordUi* ui = (NovachordUi*)data;
    Input* i = &ui->input;
    
    i->mouseButtons[event->button] = false;
    i->mouseX = event->x;
    i->mouseY = event->y;
    
    updateControlsWithInput(ui, InputType_release);
    
    return true;
}

static gboolean onMotion(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    NovachordUi* ui = (NovachordUi*)data;
    Input* i = &ui->input;

    GdkModifierType state;

    i->mouseX = event->x;
    i->mouseY = event->y;
    state = event->state;
    
    updateControlsWithInput(ui, InputType_drag);
    
    return true;
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor* descriptor,
    const char* pluginUri, const char* bundlePath,
    LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
    LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    NovachordUi* ui = (NovachordUi*) calloc(1, sizeof(NovachordUi));
    novachordUiInit(ui);

    loadResources(&ui->resources, bundlePath);

    if(!ui)
    {
        fprintf(stderr, "Novachord.lv2 UI: Out of memory\n");
        return null;
    }

    ui->map = null;
    *widget = null;

    for(int i = 0; features[i]; ++i)
        if(!strcmp(features[i]->URI, LV2_URID_URI "#map"))
            ui->map = (LV2_URID_Map*) features[i]->data;

    if(!ui->map)
    {
        fprintf(stderr, "Novachord.lv2 UI: Host does not support urid:map\n");
        free(ui);
        return null;
    }

    ui->write = writeFunction;
    ui->controller = controller;

    mapNovachordUris(ui->map, &ui->uris);
    lv2_atom_forge_init(&ui->forge, ui->map);

    ui->eventBox = gtk_event_box_new();
    ui->drawingArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(ui->drawingArea, WIDTH, HEIGHT);
    gtk_widget_set_size_request(ui->eventBox, WIDTH, HEIGHT);

    gtk_container_add(GTK_CONTAINER(ui->eventBox), ui->drawingArea);

    g_signal_connect(G_OBJECT(ui->drawingArea), "expose_event",
                     G_CALLBACK(onExposeEvent), ui);

    g_signal_connect(ui->eventBox, "button_press_event",
                     G_CALLBACK(onButtonPress), ui);
    g_signal_connect(ui->eventBox, "button_release_event",
                     G_CALLBACK(onButtonRelease), ui);
    g_signal_connect(ui->eventBox, "motion_notify_event",
                     G_CALLBACK(onMotion), ui);

    *widget = ui->eventBox;
    sendUiEnable(ui);

    g_timeout_add(1000.0f / 60.0f, (GSourceFunc)NovachordUiAnimate, ui);

    return ui;
}

static void cleanUp(LV2UI_Handle handle)
{
    NovachordUi* ui = (NovachordUi*) handle;
    freeResources(&ui->resources);
    sendUiDisable(ui);
    gtk_widget_destroy(ui->drawingArea);
    free(ui);
}

static void portEvent(LV2UI_Handle handle, u32 portIndex, u32 bufferSize, 
                      u32 format, const void* buffer)
{
    NovachordUi* ui = (NovachordUi*) handle;
    const LV2_Atom* atom = (const LV2_Atom*) buffer;

    if(portIndex >= NOVACHORD_DEEP_TONE && portIndex <= NOVACHORD_RIGHT_SUSTAIN)
    {
        for(int i = 0; i < CONTROL_COUNT; ++i)
        {
            Control* control = &ui->controls[i];
            if(control->info->port == portIndex)
            {
                if(!control->initialized)
                    control->initialized = true;
                control->position = roundf(*((float*)buffer));
            }
        }
    }

    // Sync the initial GUI state with the host
    if(!ui->initialized)
    {
        bool allInitialized = true;
        for(int i = 0; i < CONTROL_COUNT; ++i)
        {
            if(!ui->controls[i].initialized)
            {
                allInitialized = false;
                break;
            }
        }
        if(allInitialized)
        {
            ui->initialized = true;
            sendUiState(ui, true);
        }
    }
}

static const LV2UI_Descriptor descriptor = {
    NOVACHORD_URI "#ui",
    instantiate,
    cleanUp,
    portEvent,
    null
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(u32 index)
{
    switch(index)
    {
        case 0:
            return &descriptor;
        default:
            return null;
    }
}
