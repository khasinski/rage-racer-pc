#ifndef RAGE_ONLINE_CORE_API_H
#define RAGE_ONLINE_CORE_API_H
#include <stdint.h>
#include <stddef.h>
int rw_load_disc(const char *);
const char *rw_disc_id(void);
int rw_start_net_race(int,int,int,int,int,int,const int32_t *,int);
int rw_start_race(int,int,int,int,int,int,int);
void rw_set_pad(int,int,int,int,int);
int on_set_input(const int32_t *);
int on_feedback(int32_t *);
int on_local_seat(int);
int rw_set_view_seat(int);
int32_t *rw_take_input(void);
uint32_t rw_input_tick(void);
uint32_t rw_input_seq(void);
int32_t rw_apply_frame(const uint8_t *,int,uint32_t,uint32_t);
int rw_tick(void);
int32_t *rw_hud(void);
const uint8_t *rw_hud_atlas(void);
uint32_t rw_hud_atlas_revision(void);
int on_hud_unit_snapshot(void *,int);
int on_hud_apply_units(const void *,int,int);
const int32_t *rw_tachometer(void);
int rw_last_tick_stepped(void);
int rw_class_car(int,int);
int rw_car_model(int);
int rw_car_grade(int);
const char *rw_car_name(int);
int rw_car_automatic(int);
int rw_car_allowed(int,int);
void rw_set_look(int,int,int,const uint8_t *,const char *);
void rw_audio_enable(int);
int rw_audio_take(void);
const int16_t *rw_audio_data(void);
int rw_sky_width(void);
int rw_sky_height(void);
int rw_decode_sky(uint8_t *);
uint32_t rw_palette_hash(void);
int rw_texture_page(void);
int on_prepare(float,float);
void on_native_client(void);
void on_release(void);
int on_camera_size(void);
int on_instance_size(void);
int on_world_count(void);
int on_copy_camera(void*,int);
int on_copy_light(void*,int);
int on_copy_instance(int,void*,int);
const void *on_mesh_bytes(unsigned,unsigned,size_t *);
int on_material(unsigned,unsigned,unsigned,unsigned,unsigned,int,int,int,uint8_t*,size_t);
#endif
