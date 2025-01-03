#ifndef DNAFX_OPTIONS
#define DNAFX_OPTIONS

#include <glib.h>

/* Struct containing the parsed command line options */
typedef struct dnafx_options {
	gboolean interactive;
	gboolean offline;
	gboolean no_init, no_get_presets, no_get_extras;
	const char *save_presets_folder;
	int change_preset, upload_preset;
	const char *preset_file_in, *preset_file_out;
	const char *phb_file_in, *phb_file_out;
	int debug_level;
	gboolean debug_timestamps;
	gboolean disable_colors;
	int debug_libusb;
} dnafx_options;

/* Helper method to parse the command line options */
gboolean dnafx_options_parse(dnafx_options *opts, int argc, char *argv[]);

/* Helper method to show the application usage */
void dnafx_options_show_usage(void);

/* Helper method to get rid of the options parser resources */
void dnafx_options_destroy(void);

#endif
