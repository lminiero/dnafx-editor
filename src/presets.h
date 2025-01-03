#ifndef DNAFX_PRESETS
#define DNAFX_PRESETS

#include <stddef.h>
#include <stdint.h>

#include <glib.h>
#include <jansson.h>

/* Defines */
#define DNAFX_PRESET_SIZE		184
#define DNAFX_PRESET_NAME_SIZE	14
#define DNAFX_PRESET_EFFECTS	9
#define DNAFX_PRESET_EXPS		6

/* Presets state */
int dnafx_presets_init(const char *folder);
char *dnafx_presets_folder(void);
void dnafx_presets_deinit(void);

/* Internal presets structure */
typedef enum dnafx_preset_effect_type {
	DNAFX_EFFECT_FXCOMP = 0,
	DNAFX_EFFECT_DSOD,
	DNAFX_EFFECT_AMP,
	DNAFX_EFFECT_CAB,
	DNAFX_EFFECT_NSGATE,
	DNAFX_EFFECT_EQ,
	DNAFX_EFFECT_MOD,
	DNAFX_EFFECT_DELAY,
	DNAFX_EFFECT_REVERB
} dnafx_preset_effect_type;

typedef struct dnafx_preset_effect {
	dnafx_preset_effect_type type;
	gboolean active;
	uint16_t id;
	uint16_t values[6];
} dnafx_preset_effect;

typedef uint16_t dnafx_preset_expression;

typedef struct dnafx_preset {
	int id;
	char name[DNAFX_PRESET_NAME_SIZE+1];
	dnafx_preset_effect effects[DNAFX_PRESET_EFFECTS];
	dnafx_preset_expression expressions[DNAFX_PRESET_EXPS];
} dnafx_preset;
void dnafx_preset_print_debug(dnafx_preset *preset);
void dnafx_preset_free(dnafx_preset *preset);

/* Parsing */
dnafx_preset *dnafx_preset_from_bytes(uint8_t *buf, size_t blen);
dnafx_preset *dnafx_preset_from_phb(const char *phb);

/* Encoding */
int dnafx_preset_to_bytes(dnafx_preset *preset, uint8_t *buf, size_t blen);
char *dnafx_preset_to_phb(dnafx_preset *preset);

/* Importing and exporting */
dnafx_preset *dnafx_preset_import(const char *filename, gboolean phb);
int dnafx_preset_export(dnafx_preset *preset, const char *filename, gboolean phb);

/* Presets management */
int dnafx_preset_add_byid(dnafx_preset *preset, int id);
dnafx_preset *dnafx_preset_find_byid(int id);
int dnafx_preset_remove_byid(int id, gboolean unref);
int dnafx_preset_add_byname(dnafx_preset *preset);
dnafx_preset *dnafx_preset_find_byname(const char *name);
int dnafx_preset_remove_byname(const char *name, gboolean unref);

/* Listing presets */
void dnafx_presets_print(void);

#endif
