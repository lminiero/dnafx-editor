#include "options.h"
#include "debug.h"

static GOptionContext *opts = NULL;

gboolean dnafx_options_parse(dnafx_options *options, int argc, char *argv[]) {
	/* Supported command-line arguments */
	GOptionEntry opt_entries[] = {
		{ "interactive", 'i', 0, G_OPTION_ARG_NONE, &options->interactive, "Provide a CLI to interact with the device (default=no, quit when done)", NULL },
		{ "offline", 'o', 0, G_OPTION_ARG_NONE, &options->offline, "Don't connect to the device via USB (default=always connect)", NULL },
		{ "no-init", 'I', 0, G_OPTION_ARG_NONE, &options->no_init, "Don't send the initialization messages at startup (default=no)", NULL },
		{ "no-get-presets", 'G', 0, G_OPTION_ARG_NONE, &options->no_get_presets, "Don't retrieve all presets at startup (default=no)", NULL },
		{ "no-get-extras", 'E', 0, G_OPTION_ARG_NONE, &options->no_get_extras, "Don't retrieve extras (IRs?) at startup (default=no)", NULL },
		{ "save-presets", 's', 0, G_OPTION_ARG_STRING, &options->save_presets_folder, "Folder to store all retrieved presets to by default (default=none, don't save presets)", "path" },
		{ "change-preset", 'c', 0, G_OPTION_ARG_INT, &options->change_preset, "Change the current preset at startup (default=0, which means no)", "1-200" },
		{ "preset-in", 'b', 0, G_OPTION_ARG_STRING, &options->preset_file_in, "Binary preset file to read at startup (default=none)", "path" },
		{ "preset-out", 'B', 0, G_OPTION_ARG_STRING, &options->preset_file_out, "Binary preset file to write at startup (default=none)", "path" },
		{ "phb-in", 'p', 0, G_OPTION_ARG_STRING, &options->phb_file_in, "PHB preset file to read at startup (default=none)", "path" },
		{ "phb-out", 'P', 0, G_OPTION_ARG_STRING, &options->phb_file_out, "PHB preset file to write at startup (default=none)", "path" },
		{ "upload-preset", 'u', 0, G_OPTION_ARG_INT, &options->upload_preset, "Upload the imported preset to the specified preset number (default=0, don't upload anything)", "1-200" },
		{ "debug-level", 'd', 0, G_OPTION_ARG_INT, &options->debug_level, "Debug/logging level (0=disable debugging, 7=maximum debug level; default=4)", "0-7" },
		{ "debug-timestamps", 't', 0, G_OPTION_ARG_NONE, &options->debug_timestamps, "Enable debug/logging timestamps", NULL },
		{ "disable-colors", 'C', 0, G_OPTION_ARG_NONE, &options->disable_colors, "Disable color in the logging", NULL },
		{ "libusb-debug", 'D', 0, G_OPTION_ARG_INT, &options->debug_libusb, "Debug/logging level for libusb (0=disable libusb debugging, 4=maximum libusb debug level; default=0)", "0-4" },
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};

	/* Parse the command-line arguments */
	GError *error = NULL;
	opts = g_option_context_new("");
	g_option_context_set_help_enabled(opts, TRUE);
	g_option_context_add_main_entries(opts, opt_entries, NULL);
	if(!g_option_context_parse(opts, &argc, &argv, &error)) {
		DNAFX_LOG(DNAFX_LOG_WARN, "%s\n", error->message);
		g_error_free(error);
		dnafx_options_show_usage();
		dnafx_options_destroy();
		return FALSE;
	}

	/* Done */
	return TRUE;
}

void dnafx_options_show_usage(void) {
	if(opts == NULL)
		return;
	char *help = g_option_context_get_help(opts, TRUE, NULL);
	DNAFX_PRINT("\n%s", help);
	g_free(help);
}

void dnafx_options_destroy(void) {
	if(opts != NULL)
		g_option_context_free(opts);
	opts = NULL;
}
