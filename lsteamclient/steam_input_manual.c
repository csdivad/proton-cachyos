#include <stdlib.h>
#include <string.h>

#include "xinput.h"

#include "steamclient_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(steamclient);

#define XINPUT_STEAM_HANDLE_BASE UINT64_C(0x474558494e500100)
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
};

struct xinput_digital_action
{
    const char *name;
    uint64_t handle;
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

/* Monster Hunter Wilds uses these action names from its Xbox One action manifest. */
static struct xinput_digital_action xinput_digital_actions[] =
{
    {"LUp",         0, XINPUT_GAMEPAD_DPAD_UP},
    {"LDown",       0, XINPUT_GAMEPAD_DPAD_DOWN},
    {"LLeft",       0, XINPUT_GAMEPAD_DPAD_LEFT},
    {"LRight",      0, XINPUT_GAMEPAD_DPAD_RIGHT},
    {"RUp",         0, XINPUT_GAMEPAD_Y},
    {"RDown",       0, XINPUT_GAMEPAD_A},
    {"RLeft",       0, XINPUT_GAMEPAD_X},
    {"RRight",      0, XINPUT_GAMEPAD_B},
    {"CLeft",       0, XINPUT_GAMEPAD_BACK},
    {"CRight",      0, XINPUT_GAMEPAD_START},
    {"CCenter",     0, 0},
    {"LStickPush",  0, XINPUT_GAMEPAD_LEFT_THUMB},
    {"RStickPush",  0, XINPUT_GAMEPAD_RIGHT_THUMB},
    {"LTrigTop",    0, XINPUT_GAMEPAD_LEFT_SHOULDER},
    {"RTrigTop",    0, XINPUT_GAMEPAD_RIGHT_SHOULDER},
};

static struct xinput_analog_action xinput_analog_actions[] =
{
    {"AxisL",    0, XINPUT_ANALOG_LEFT_STICK},
    {"AxisR",    0, XINPUT_ANALOG_RIGHT_STICK},
    {"TouchPad", 0, XINPUT_ANALOG_UNAVAILABLE},
    {"AnalogL",  0, XINPUT_ANALOG_LEFT_TRIGGER},
    {"AnalogR",  0, XINPUT_ANALOG_RIGHT_TRIGGER},
};

static BOOL steaminput_xinput_fallback_enabled(void)
{
    const char *env = getenv("PROTON_STEAMINPUT_XINPUT_FALLBACK");

    return env && env[0] == '1' && !env[1];
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

    if (!steaminput_xinput_fallback_enabled() || native_count > 0 || !handles) return native_count;

    for (index = 0; index < XUSER_MAX_COUNT; ++index)
    {
        if (XInputGetState(index, &state) != ERROR_SUCCESS) continue;
        handles[count++] = xinput_steam_handle(index);
        mask |= 1u << index;
    }

    if (mask != previous_mask)
    {
        TRACE("Steam Input XInput fallback connected mask %#x.\n", mask);
        previous_mask = mask;
    }

    return (int32_t)count;
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
        return xinput_digital_actions[i].handle;
    }

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
    if (XInputGetState(index, &state) != ERROR_SUCCESS) return TRUE;

    for (i = 0; i < ARRAY_SIZE(xinput_digital_actions); ++i)
    {
        if (xinput_digital_actions[i].handle != action_handle) continue;
        data->bState = !!(state.Gamepad.wButtons & xinput_digital_actions[i].buttons);
        data->bActive = !!xinput_digital_actions[i].buttons;
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

    *type = STEAM_INPUT_TYPE_XBOX_ONE;
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
