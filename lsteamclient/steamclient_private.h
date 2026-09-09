/* TODO these should be generated */
#ifndef __STEAMCLIENT_PRIVATE_H
#define __STEAMCLIENT_PRIVATE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <windef.h>
#include <winbase.h>

#include "steamclient_structs.h"
#include "unixlib.h"

#include "wine/debug.h"
#include "wine/list.h"

#ifndef __cplusplus
#include "cxx.h"
#else
typedef void (*vtable_ptr)(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct u_iface;
struct w_iface
{
    vtable_ptr *vtable;
    struct u_iface u_iface;
};

typedef struct w_iface *(*iface_constructor)( struct u_iface );
extern iface_constructor find_iface_constructor( const char *iface_version );
extern struct w_iface *create_winISteamNetworkingFakeUDPPort_SteamNetworkingFakeUDPPort001( struct u_iface );

extern void execute_pending_callbacks(void);

struct w_iface *create_win_interface( const char *name, struct u_iface );
void *alloc_mem_for_iface(size_t size, const char *iface_version);
void *alloc_vtable(void *vtable, unsigned int method_count, const char *iface_version);
void *get_unix_buffer( struct u_buffer buf );

void init_rtti( char *base );

#include "steamclient_generated.h"

int32_t steaminput006_xinput_get_connected_controllers( int32_t native_count, uint64_t *handles );
uint64_t steaminput006_xinput_register_digital_action( uint64_t native_handle, const char *name );
int steaminput006_xinput_get_digital_action_data( InputDigitalActionData_t *data,
        uint64_t input_handle, uint64_t action_handle );
uint64_t steaminput006_xinput_register_analog_action( uint64_t native_handle, const char *name );
int steaminput006_xinput_get_analog_action_data( InputAnalogActionData_t *data,
        uint64_t input_handle, uint64_t action_handle );
int steaminput006_xinput_get_motion_data( InputMotionData_t *data, uint64_t input_handle );
int steaminput006_xinput_trigger_vibration( uint64_t input_handle, uint16_t left, uint16_t right );
int steaminput006_xinput_trigger_vibration_extended( uint64_t input_handle, uint16_t left, uint16_t right,
        uint16_t left_trigger, uint16_t right_trigger );
int steaminput006_xinput_get_input_type( uint32_t *type, uint64_t input_handle );
int steaminput006_xinput_get_controller_for_gamepad_index( uint64_t *handle, int32_t index );
int steaminput006_xinput_get_gamepad_index_for_controller( int32_t *index, uint64_t input_handle );

#ifdef __cplusplus
}
#endif

#endif /* __STEAMCLIENT_PRIVATE_H */
