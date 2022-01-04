#ifndef _PCEM_DEVICES_H_
#define _PCEM_DEVICES_H_

#include <pcem/cpu.h>

typedef struct device_config_selection_t
{
        char description[256];
        int value;
} device_config_selection_t;

typedef struct device_config_t
{
        char name[256];
        char description[256];
        int type;
        char default_string[256];
        int default_int;
        device_config_selection_t selection[16];
} device_config_t;

typedef struct device_t
{
        char name[50];
        uint32_t flags;
        void *(*init)();
        void (*close)(void *p);
        int  (*available)();
        void (*speed_changed)(void *p);
        void (*force_redraw)(void *p);
        void (*add_status_info)(char *s, int max_len, void *p);
        device_config_t *config;
} device_t;

typedef struct SOUND_CARD
{
        char name[64];
        char internal_name[24];
        device_t *device;
} SOUND_CARD;

typedef struct video_timings_t
{
        int type;
        int write_b, write_w, write_l;
        int read_b, read_w, read_l;
} video_timings_t;

typedef struct VIDEO_CARD
{
        char name[64];
        char internal_name[24];
        device_t *device;
        int legacy_id;
        int flags;
        video_timings_t timing;
} VIDEO_CARD;

typedef struct MODEL
{
        char name[64];
        int id;
        char internal_name[24];
        struct
        {
                char name[8];
                CPU *cpus;
        } cpu[5];
        int flags;
        int min_ram, max_ram;
        int ram_granularity;
        void (*init)();
        struct device_t *device;
} MODEL;

typedef struct HDD_CONTROLLER
{
        char name[50];
        char internal_name[16];
        device_t *device;
        int is_mfm;
        int is_ide;
        int is_scsi;
} HDD_CONTROLLER;

typedef struct NETWORK_CARD
{
        char name[32];
        char internal_name[24];
        device_t *device;
} NETWORK_CARD;

extern void pcem_add_model(MODEL *model);
extern void pcem_add_video(VIDEO_CARD *video);
extern void pcem_add_sound(SOUND_CARD *sound);
extern void pcem_add_hddcontroller(HDD_CONTROLLER *hddcontroller);
extern void pcem_add_networkcard(NETWORK_CARD *netcard);
extern void pcem_add_device(device_t *device);

#endif /* _PCEM_DEVICES_H_ */
