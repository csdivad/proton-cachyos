#include <stdlib.h>
#include <string.h>

#include "xinput.h"

#include "steamclient_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(steamclient);

#define XINPUT_STEAM_HANDLE_BASE UINT64_C(0x474558494e500100)
#define XINPUT_ACTION_SET_BASE UINT64_C(0x4745585345540000)
#define XINPUT_DIGITAL_ACTION_BASE UINT64_C(0x4745584449470000)
#define XINPUT_ANALOG_ACTION_BASE UINT64_C(0x474558414e410000)

enum steam_input_source_mode
{
    STEAM_INPUT_SOURCE_MODE_JOYSTICK_MOVE = 6,
    STEAM_INPUT_SOURCE_MODE_TRIGGER = 10,
};

enum steam_input_type
{
    STEAM_INPUT_TYPE_XBOX_ONE = 3,
    STEAM_INPUT_TYPE_PS4 = 5,
    STEAM_INPUT_TYPE_PS5 = 13,
};

enum xinput_digital_source
{
    XINPUT_DIGITAL_BUTTON,
    XINPUT_DIGITAL_LEFT_TRIGGER,
    XINPUT_DIGITAL_RIGHT_TRIGGER,
    XINPUT_DIGITAL_UNAVAILABLE,
};

struct xinput_digital_action
{
    const char *name;
    uint64_t handle;
    enum xinput_digital_source source;
    WORD buttons;
};

struct xinput_analog_action
{
    const char *name;
    uint64_t handle;
    enum
    {
        XINPUT_ANALOG_LEFT_STICK,
        XINPUT_ANALOG_RIGHT_STICK,
        XINPUT_ANALOG_LEFT_TRIGGER,
        XINPUT_ANALOG_RIGHT_TRIGGER,
        XINPUT_ANALOG_UNAVAILABLE,
    } source;
};

#define DIGITAL_BUTTON(name, button) {name, 0, XINPUT_DIGITAL_BUTTON, button}
#define DIGITAL_TRIGGER(name, trigger) {name, 0, trigger, 0}
#define DIGITAL_UNAVAILABLE(name) {name, 0, XINPUT_DIGITAL_UNAVAILABLE, 0}

/* Action aliases used by the supported Steam Input manifests. */
static struct xinput_digital_action xinput_digital_actions[] =
{
    /* Monster Hunter Wilds. */
    DIGITAL_BUTTON("LUp", XINPUT_GAMEPAD_DPAD_UP),
    DIGITAL_BUTTON("LDown", XINPUT_GAMEPAD_DPAD_DOWN),
    DIGITAL_BUTTON("LLeft", XINPUT_GAMEPAD_DPAD_LEFT),
    DIGITAL_BUTTON("LRight", XINPUT_GAMEPAD_DPAD_RIGHT),
    DIGITAL_BUTTON("RUp", XINPUT_GAMEPAD_Y),
    DIGITAL_BUTTON("RDown", XINPUT_GAMEPAD_A),
    DIGITAL_BUTTON("RLeft", XINPUT_GAMEPAD_X),
    DIGITAL_BUTTON("RRight", XINPUT_GAMEPAD_B),
    DIGITAL_BUTTON("CLeft", XINPUT_GAMEPAD_BACK),
    DIGITAL_BUTTON("CRight", XINPUT_GAMEPAD_START),
    DIGITAL_UNAVAILABLE("CCenter"),
    DIGITAL_BUTTON("LStickPush", XINPUT_GAMEPAD_LEFT_THUMB),
    DIGITAL_BUTTON("RStickPush", XINPUT_GAMEPAD_RIGHT_THUMB),
    DIGITAL_BUTTON("LTrigTop", XINPUT_GAMEPAD_LEFT_SHOULDER),
    DIGITAL_BUTTON("RTrigTop", XINPUT_GAMEPAD_RIGHT_SHOULDER),

    /* Horizon Zero Dawn. */
    DIGITAL_BUTTON("button_a", XINPUT_GAMEPAD_A),
    DIGITAL_BUTTON("button_b", XINPUT_GAMEPAD_B),
    DIGITAL_BUTTON("button_x", XINPUT_GAMEPAD_X),
    DIGITAL_BUTTON("button_y", XINPUT_GAMEPAD_Y),
    DIGITAL_BUTTON("button_touchpad", XINPUT_GAMEPAD_BACK),
    DIGITAL_BUTTON("button_start", XINPUT_GAMEPAD_START),
    DIGITAL_BUTTON("dpad_up", XINPUT_GAMEPAD_DPAD_UP),
    DIGITAL_BUTTON("dpad_down", XINPUT_GAMEPAD_DPAD_DOWN),
    DIGITAL_BUTTON("dpad_left", XINPUT_GAMEPAD_DPAD_LEFT),
    DIGITAL_BUTTON("dpad_right", XINPUT_GAMEPAD_DPAD_RIGHT),
    DIGITAL_BUTTON("left_bumper", XINPUT_GAMEPAD_LEFT_SHOULDER),
    DIGITAL_BUTTON("right_bumper", XINPUT_GAMEPAD_RIGHT_SHOULDER),
    DIGITAL_BUTTON("left_stick_click", XINPUT_GAMEPAD_LEFT_THUMB),
    DIGITAL_BUTTON("right_stick_click", XINPUT_GAMEPAD_RIGHT_THUMB),
    DIGITAL_UNAVAILABLE("left_back_panel"),
    DIGITAL_UNAVAILABLE("right_back_panel"),

    /* God of War Ragnarok. */
    DIGITAL_BUTTON("evade", XINPUT_GAMEPAD_A),
    DIGITAL_BUTTON("useworld", XINPUT_GAMEPAD_B),
    DIGITAL_BUTTON("companioninteract", XINPUT_GAMEPAD_X),
    DIGITAL_BUTTON("axerecall", XINPUT_GAMEPAD_Y),
    DIGITAL_TRIGGER("aim", XINPUT_DIGITAL_LEFT_TRIGGER),
    DIGITAL_TRIGGER("heavyattack", XINPUT_DIGITAL_RIGHT_TRIGGER),
    DIGITAL_BUTTON("defend", XINPUT_GAMEPAD_LEFT_SHOULDER),
    DIGITAL_BUTTON("lightattack", XINPUT_GAMEPAD_RIGHT_SHOULDER),
    DIGITAL_BUTTON("sprint", XINPUT_GAMEPAD_LEFT_THUMB),
    DIGITAL_BUTTON("togglelockon", XINPUT_GAMEPAD_RIGHT_THUMB),
    DIGITAL_BUTTON("companionammo", XINPUT_GAMEPAD_DPAD_UP),
    DIGITAL_BUTTON("tertiaryweapontoggle", XINPUT_GAMEPAD_DPAD_DOWN),
    DIGITAL_BUTTON("primaryweapontoggle", XINPUT_GAMEPAD_DPAD_RIGHT),
    DIGITAL_BUTTON("secondaryweapontoggle", XINPUT_GAMEPAD_DPAD_LEFT),
    DIGITAL_BUTTON("pausemenu", XINPUT_GAMEPAD_START),
    DIGITAL_BUTTON("weaponmenu", XINPUT_GAMEPAD_BACK),
    DIGITAL_BUTTON("SELECT", XINPUT_GAMEPAD_A),
    DIGITAL_BUTTON("Select", XINPUT_GAMEPAD_A),
    DIGITAL_BUTTON("Cancel", XINPUT_GAMEPAD_B),
    DIGITAL_BUTTON("MenuX", XINPUT_GAMEPAD_X),
    DIGITAL_BUTTON("MenuY", XINPUT_GAMEPAD_Y),
    DIGITAL_TRIGGER("MenuPrevScreen", XINPUT_DIGITAL_LEFT_TRIGGER),
    DIGITAL_TRIGGER("MenuNextScreen", XINPUT_DIGITAL_RIGHT_TRIGGER),
    DIGITAL_BUTTON("MenuPrevOption", XINPUT_GAMEPAD_LEFT_SHOULDER),
    DIGITAL_BUTTON("MenuNextOption", XINPUT_GAMEPAD_RIGHT_SHOULDER),
    DIGITAL_BUTTON("MenuLclick", XINPUT_GAMEPAD_LEFT_THUMB),
    DIGITAL_BUTTON("MenuRclick", XINPUT_GAMEPAD_RIGHT_THUMB),
    DIGITAL_BUTTON("MenuUp", XINPUT_GAMEPAD_DPAD_UP),
    DIGITAL_BUTTON("MenuDown", XINPUT_GAMEPAD_DPAD_DOWN),
    DIGITAL_BUTTON("MenuRight", XINPUT_GAMEPAD_DPAD_RIGHT),
    DIGITAL_BUTTON("MenuLeft", XINPUT_GAMEPAD_DPAD_LEFT),
    DIGITAL_BUTTON("MenuCharacter", XINPUT_GAMEPAD_BACK),
    DIGITAL_BUTTON("MenuExit", XINPUT_GAMEPAD_START),
};

static struct xinput_analog_action xinput_analog_actions[] =
{
    {"AxisL",    0, XINPUT_ANALOG_LEFT_STICK},
    {"AxisR",    0, XINPUT_ANALOG_RIGHT_STICK},
    {"TouchPad", 0, XINPUT_ANALOG_UNAVAILABLE},
    {"AnalogL",  0, XINPUT_ANALOG_LEFT_TRIGGER},
    {"AnalogR",  0, XINPUT_ANALOG_RIGHT_TRIGGER},
    {"left_trigger",  0, XINPUT_ANALOG_LEFT_TRIGGER},
    {"right_trigger", 0, XINPUT_ANALOG_RIGHT_TRIGGER},
    {"left_stick",    0, XINPUT_ANALOG_LEFT_STICK},
    {"right_stick",   0, XINPUT_ANALOG_RIGHT_STICK},
    {"Move",          0, XINPUT_ANALOG_LEFT_STICK},
    {"Camera",         0, XINPUT_ANALOG_RIGHT_STICK},
};

static struct
{
    const char *name;
    uint64_t handle;
} xinput_action_sets[] =
{
    {"GamepadSetting", 0},
    {"GameControls", 0},
    {"MenuControls", 0},
};

static BOOL env_enabled(const char *name)
{
    const char *env = getenv(name);

    return env && env[0] == '1' && !env[1];
}

int steaminput_xinput_fallback_enabled(void)
{
    return env_enabled("PROTON_STEAMINPUT_FALLBACK") ||
            env_enabled("PROTON_STEAMINPUT_XINPUT_FALLBACK");
}

static uint32_t steaminput_xinput_fallback_type(void)
{
    if (env_enabled("PROTON_STEAMINPUT_LAYOUT_DS5")) return STEAM_INPUT_TYPE_PS5;
    if (env_enabled("PROTON_STEAMINPUT_LAYOUT_DS4")) return STEAM_INPUT_TYPE_PS4;
    if (env_enabled("PROTON_STEAMINPUT_LAYOUT_XBOX")) return STEAM_INPUT_TYPE_XBOX_ONE;
    return STEAM_INPUT_TYPE_XBOX_ONE;
}

static uint64_t xinput_steam_handle(unsigned int index)
{
    return XINPUT_STEAM_HANDLE_BASE + index;
}

static int xinput_index_from_steam_handle(uint64_t handle)
{
    uint64_t index;

    if (handle < XINPUT_STEAM_HANDLE_BASE) return -1;
    index = handle - XINPUT_STEAM_HANDLE_BASE;
    if (index >= XUSER_MAX_COUNT) return -1;
    return (int)index;
}

static float normalize_thumb(SHORT value)
{
    return value < 0 ? value / 32768.0f : value / 32767.0f;
}

int32_t steaminput006_xinput_get_connected_controllers(int32_t native_count, uint64_t *handles)
{
    static unsigned int previous_mask = ~0u;
    unsigned int index, count = 0, mask = 0;
    XINPUT_STATE state;

    if (!steaminput_xinput_fallback_enabled() || !handles) return native_count;

    for (index = 0; index < XUSER_MAX_COUNT; ++index)
    {
        if (XInputGetState(index, &state) != ERROR_SUCCESS) continue;
        handles[count++] = xinput_steam_handle(index);
        mask |= 1u << index;
    }

    if (mask != previous_mask)
    {
        TRACE("Steam Input XInput fallback replacing %d native controllers with mask %#x.\n",
                native_count, mask);
        previous_mask = mask;
    }

    return (int32_t)count;
}

uint64_t steaminput006_xinput_register_action_set(uint64_t native_handle, const char *name)
{
    unsigned int i;

    if (!steaminput_xinput_fallback_enabled() || !name) return native_handle;

    for (i = 0; i < ARRAY_SIZE(xinput_action_sets); ++i)
    {
        if (strcmp(name, xinput_action_sets[i].name)) continue;
        if (native_handle) xinput_action_sets[i].handle = native_handle;
        else if (!xinput_action_sets[i].handle)
            xinput_action_sets[i].handle = XINPUT_ACTION_SET_BASE + i;
        return xinput_action_sets[i].handle;
    }

    return native_handle;
}

uint64_t steaminput006_xinput_register_digital_action(uint64_t native_handle, const char *name)
{
    unsigned int i;

    if (!steaminput_xinput_fallback_enabled() || !name) return native_handle;

    for (i = 0; i < ARRAY_SIZE(xinput_digital_actions); ++i)
    {
        if (strcmp(name, xinput_digital_actions[i].name)) continue;
        if (native_handle) xinput_digital_actions[i].handle = native_handle;
        else if (!xinput_digital_actions[i].handle)
            xinput_digital_actions[i].handle = XINPUT_DIGITAL_ACTION_BASE + i;
        TRACE("Steam Input XInput fallback mapped digital action '%s' to %#I64x.\n",
                name, xinput_digital_actions[i].handle);
        return xinput_digital_actions[i].handle;
    }

    TRACE("Steam Input XInput fallback has no mapping for digital action '%s' (native handle %#I64x).\n",
            name, native_handle);
    return native_handle;
}

int steaminput006_xinput_get_digital_action_data(InputDigitalActionData_t *data,
        uint64_t input_handle, uint64_t action_handle)
{
    XINPUT_STATE state;
    unsigned int i;
    int index;

    if (!steaminput_xinput_fallback_enabled() ||
        (index = xinput_index_from_steam_handle(input_handle)) < 0)
        return FALSE;

    memset(data, 0, sizeof(*data));
    if (!action_handle) return TRUE;
    if (XInputGetState(index, &state) != ERROR_SUCCESS) return TRUE;

    for (i = 0; i < ARRAY_SIZE(xinput_digital_actions); ++i)
    {
        if (xinput_digital_actions[i].handle != action_handle) continue;

        switch (xinput_digital_actions[i].source)
        {
            case XINPUT_DIGITAL_BUTTON:
                data->bState = !!(state.Gamepad.wButtons & xinput_digital_actions[i].buttons);
                data->bActive = TRUE;
                break;

            case XINPUT_DIGITAL_LEFT_TRIGGER:
                data->bState = state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
                data->bActive = TRUE;
                break;

            case XINPUT_DIGITAL_RIGHT_TRIGGER:
                data->bState = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
                data->bActive = TRUE;
                break;

            case XINPUT_DIGITAL_UNAVAILABLE:
                break;
        }
        break;
    }

    return TRUE;
}

uint64_t steaminput006_xinput_register_analog_action(uint64_t native_handle, const char *name)
{
    unsigned int i;

    if (!steaminput_xinput_fallback_enabled() || !name) return native_handle;

    for (i = 0; i < ARRAY_SIZE(xinput_analog_actions); ++i)
    {
        if (strcmp(name, xinput_analog_actions[i].name)) continue;
        if (native_handle) xinput_analog_actions[i].handle = native_handle;
        else if (!xinput_analog_actions[i].handle)
            xinput_analog_actions[i].handle = XINPUT_ANALOG_ACTION_BASE + i;
        return xinput_analog_actions[i].handle;
    }

    return native_handle;
}

int steaminput006_xinput_get_analog_action_data(InputAnalogActionData_t *data,
        uint64_t input_handle, uint64_t action_handle)
{
    XINPUT_STATE state;
    unsigned int i;
    int index;

    if (!steaminput_xinput_fallback_enabled() ||
        (index = xinput_index_from_steam_handle(input_handle)) < 0)
        return FALSE;

    memset(data, 0, sizeof(*data));
    if (!action_handle) return TRUE;
    if (XInputGetState(index, &state) != ERROR_SUCCESS) return TRUE;

    for (i = 0; i < ARRAY_SIZE(xinput_analog_actions); ++i)
    {
        if (xinput_analog_actions[i].handle != action_handle) continue;

        switch (xinput_analog_actions[i].source)
        {
            case XINPUT_ANALOG_LEFT_STICK:
                data->bActive = TRUE;
                data->eMode = STEAM_INPUT_SOURCE_MODE_JOYSTICK_MOVE;
                data->x = normalize_thumb(state.Gamepad.sThumbLX);
                data->y = normalize_thumb(state.Gamepad.sThumbLY);
                break;

            case XINPUT_ANALOG_RIGHT_STICK:
                data->bActive = TRUE;
                data->eMode = STEAM_INPUT_SOURCE_MODE_JOYSTICK_MOVE;
                data->x = normalize_thumb(state.Gamepad.sThumbRX);
                data->y = normalize_thumb(state.Gamepad.sThumbRY);
                break;

            case XINPUT_ANALOG_LEFT_TRIGGER:
                data->bActive = TRUE;
                data->eMode = STEAM_INPUT_SOURCE_MODE_TRIGGER;
                data->x = state.Gamepad.bLeftTrigger / 255.0f;
                break;

            case XINPUT_ANALOG_RIGHT_TRIGGER:
                data->bActive = TRUE;
                data->eMode = STEAM_INPUT_SOURCE_MODE_TRIGGER;
                data->x = state.Gamepad.bRightTrigger / 255.0f;
                break;

            case XINPUT_ANALOG_UNAVAILABLE:
                data->eMode = STEAM_INPUT_SOURCE_MODE_JOYSTICK_MOVE;
                break;
        }
        break;
    }

    return TRUE;
}

int steaminput006_xinput_get_motion_data(InputMotionData_t *data, uint64_t input_handle)
{
    if (!steaminput_xinput_fallback_enabled() || xinput_index_from_steam_handle(input_handle) < 0)
        return FALSE;

    memset(data, 0, sizeof(*data));
    data->rotQuatW = 1.0f;
    return TRUE;
}

int steaminput006_xinput_trigger_vibration(uint64_t input_handle, uint16_t left, uint16_t right)
{
    XINPUT_VIBRATION vibration = {left, right};
    int index;

    if (!steaminput_xinput_fallback_enabled() ||
        (index = xinput_index_from_steam_handle(input_handle)) < 0)
        return FALSE;

    XInputSetState(index, &vibration);
    return TRUE;
}

int steaminput006_xinput_trigger_vibration_extended(uint64_t input_handle, uint16_t left, uint16_t right,
        uint16_t left_trigger, uint16_t right_trigger)
{
    if (left_trigger > left) left = left_trigger;
    if (right_trigger > right) right = right_trigger;
    return steaminput006_xinput_trigger_vibration(input_handle, left, right);
}

int steaminput006_xinput_get_input_type(uint32_t *type, uint64_t input_handle)
{
    if (!steaminput_xinput_fallback_enabled() || xinput_index_from_steam_handle(input_handle) < 0)
        return FALSE;

    *type = steaminput_xinput_fallback_type();
    return TRUE;
}

int steaminput006_xinput_get_controller_for_gamepad_index(uint64_t *handle, int32_t index)
{
    XINPUT_STATE state;

    if (!steaminput_xinput_fallback_enabled() || index < 0 || (unsigned int)index >= XUSER_MAX_COUNT ||
        XInputGetState(index, &state) != ERROR_SUCCESS)
        return FALSE;

    *handle = xinput_steam_handle(index);
    return TRUE;
}

int steaminput006_xinput_get_gamepad_index_for_controller(int32_t *index, uint64_t input_handle)
{
    int xinput_index;

    if (!steaminput_xinput_fallback_enabled() ||
        (xinput_index = xinput_index_from_steam_handle(input_handle)) < 0)
        return FALSE;

    *index = xinput_index;
    return TRUE;
}

/* ISteamController_SteamController005 */

const char *__thiscall winISteamController_SteamController005_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController005_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController005_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamController_SteamController006 */

const char *__thiscall winISteamController_SteamController006_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController006_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController006_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamController_SteamController007 */

const char *__thiscall winISteamController_SteamController007_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController007_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController007_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamController_SteamController007_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController007_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController007_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamController_SteamController008 */

const char *__thiscall winISteamController_SteamController008_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController008_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController008_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamController_SteamController008_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamController_SteamController008_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamController_SteamController008_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamInput_SteamInput001 */

const char *__thiscall winISteamInput_SteamInput001_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput001_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput001_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput001_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput001_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput001_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamInput_SteamInput002 */

const char *__thiscall winISteamInput_SteamInput002_GetGlyphForActionOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput002_GetGlyphForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput002_GetGlyphForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput002_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput002_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput002_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamInput_SteamInput005 */

const char *__thiscall winISteamInput_SteamInput005_GetGlyphPNGForActionOrigin( struct w_iface *_this, uint32_t eOrigin,
                                                                                uint32_t eSize, uint32_t unFlags )
{
    struct ISteamInput_SteamInput005_GetGlyphPNGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .eSize = eSize,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput005_GetGlyphPNGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput005_GetGlyphSVGForActionOrigin( struct w_iface *_this,
                                                                                uint32_t eOrigin, uint32_t unFlags )
{
    struct ISteamInput_SteamInput005_GetGlyphSVGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput005_GetGlyphSVGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput005_GetGlyphForActionOrigin_Legacy( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput005_GetGlyphForActionOrigin_Legacy_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput005_GetGlyphForActionOrigin_Legacy, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput005_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput005_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput005_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamInput_SteamInput006 */

const char *__thiscall winISteamInput_SteamInput006_GetGlyphPNGForActionOrigin( struct w_iface *_this, uint32_t eOrigin,
                                                                                uint32_t eSize, uint32_t unFlags )
{
    struct ISteamInput_SteamInput006_GetGlyphPNGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .eSize = eSize,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput006_GetGlyphPNGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput006_GetGlyphSVGForActionOrigin( struct w_iface *_this,
                                                                                uint32_t eOrigin, uint32_t unFlags )
{
    struct ISteamInput_SteamInput006_GetGlyphSVGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput006_GetGlyphSVGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput006_GetGlyphForActionOrigin_Legacy( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput006_GetGlyphForActionOrigin_Legacy_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput006_GetGlyphForActionOrigin_Legacy, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput006_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput006_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput006_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}

/* ISteamInput_SteamInput007 */

const char *__thiscall winISteamInput_SteamInput007_GetGlyphPNGForActionOrigin( struct w_iface *_this, uint32_t eOrigin,
                                                                                uint32_t eSize, uint32_t unFlags )
{
    struct ISteamInput_SteamInput007_GetGlyphPNGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .eSize = eSize,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput007_GetGlyphPNGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput007_GetGlyphSVGForActionOrigin( struct w_iface *_this,
                                                                                uint32_t eOrigin, uint32_t unFlags )
{
    struct ISteamInput_SteamInput007_GetGlyphSVGForActionOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
        .unFlags = unFlags,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput007_GetGlyphSVGForActionOrigin, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput007_GetGlyphForActionOrigin_Legacy( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput007_GetGlyphForActionOrigin_Legacy_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput007_GetGlyphForActionOrigin_Legacy, &params );
    return get_unix_buffer( params._ret );
}

const char *__thiscall winISteamInput_SteamInput007_GetGlyphForXboxOrigin( struct w_iface *_this, uint32_t eOrigin )
{
    struct ISteamInput_SteamInput007_GetGlyphForXboxOrigin_params params =
    {
        .u_iface = _this->u_iface,
        .eOrigin = eOrigin,
    };

    TRACE( "%p\n", _this );

    STEAMCLIENT_CALL( ISteamInput_SteamInput007_GetGlyphForXboxOrigin, &params );
    return get_unix_buffer( params._ret );
}
