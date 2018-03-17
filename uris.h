#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define NOVACHORD_URI "http://epihaumut.com/plugins/novachord"

typedef struct NovachordUris
{
	LV2_URID atom_Vector;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_eventTransfer;
	LV2_URID param_sampleRate;

	LV2_URID ui_On;
	LV2_URID ui_Off;
	LV2_URID ui_State;
	LV2_URID ui_Parameters;
} NovachordUris;

static inline void
mapNovachordUris(LV2_URID_Map* map, NovachordUris* uris)
{
	uris->atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);
	uris->atom_Float         = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->param_sampleRate   = map->map(map->handle, LV2_PARAMETERS__sampleRate);

	uris->ui_On         = map->map(map->handle, NOVACHORD_URI "#UIOn");
	uris->ui_Off        = map->map(map->handle, NOVACHORD_URI "#UIOff");
	uris->ui_State      = map->map(map->handle, NOVACHORD_URI "#UIState");
	uris->ui_Parameters = map->map(map->handle, NOVACHORD_URI "#ui-parameters");
}