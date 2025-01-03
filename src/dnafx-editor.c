#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "dnafx-editor.h"
#include "usb.h"
#include "tasks.h"
#include "presets.h"
#include "utils.h"
#include "options.h"
#include "embedded_cli.h"
#include "debug.h"

/* Command line options */
static dnafx_options options = { 0 };

/* Logging */
int dnafx_log_level = DNAFX_LOG_INFO;
gboolean dnafx_log_timestamps = FALSE;
gboolean dnafx_log_colors = TRUE;

/* Signal */
static volatile int stop = 0;
static void dnafx_handle_signal(int signum) {
	dnafx_quit();
}

/* Embedded CLI */
static volatile int cli_started = 0;
static struct embedded_cli cli = { 0 };
static void dnafx_putch(void *data, char ch, bool is_last);
static char dnafx_getch(void);

/* Polling */
#define DNAFX_POLL_TIMEOUT	250;

/* Main */
int main(int argc, char *argv[]) {
	int res = 0;
	dnafx_tasks_init();

	DNAFX_PRINT("\nOpen source DNAfx GiT editor (experimental and WIP)\n\n");
	DNAFX_PRINT("  ####################################################################\n");
	DNAFX_PRINT("  #                                                                  #\n");
	DNAFX_PRINT("  #  NOTE WELL: Not affiliated with, nor endorsed by, Harley Benton  #\n");
	DNAFX_PRINT("  #                                                                  #\n");
	DNAFX_PRINT("  ####################################################################\n\n");

	/* Initialize some command line options defaults */
	options.debug_level = DNAFX_LOG_INFO;
	/* Let's call our cmdline parser */
	if(!dnafx_options_parse(&options, argc, argv)) {
		dnafx_options_show_usage();
		dnafx_options_destroy();
		res = 1;
		goto done;
	}
	/* Logging level */
	dnafx_log_level = options.debug_level;
	dnafx_log_timestamps = options.debug_timestamps;
	dnafx_log_colors = !options.disable_colors;

	/* Presets management */
	if(dnafx_presets_init(options.save_presets_folder) < 0) {
		res = 1;
		goto done;
	}

	/* Check if we need to parse a preset file */
	if(options.preset_file_in != NULL && options.phb_file_in != NULL) {
		DNAFX_LOG(DNAFX_LOG_FATAL, "Can't provide both binary and PHB file as preset input\n");
		res = 1;
		goto done;
	}
	if((options.preset_file_out != NULL || options.phb_file_out != NULL) &&
			options.preset_file_in == NULL && options.phb_file_in == NULL) {
		DNAFX_LOG(DNAFX_LOG_FATAL, "Can't convert preset to a different format, no input preset provided\n");
		res = 1;
		goto done;
	}
	/* Parse the preset file, if any, to our internal format */
	dnafx_preset *preset = NULL;
	if(options.preset_file_in != NULL) {
		/* Open the provided binary file and parse it */
		preset = dnafx_preset_import(options.preset_file_in, FALSE);
	} else if(options.phb_file_in != NULL) {
		/* Open the provided PHB file and parse it */
		preset = dnafx_preset_import(options.phb_file_in, TRUE);
	}
	/* Check if we need to convert this preset to something else */
	if(options.preset_file_out != NULL) {
		/* Convert the preset to the binary format */
		if(dnafx_preset_export(preset, options.preset_file_out, FALSE) < 0) {
			res = 1;
			goto done;
		}
	}
	if(options.phb_file_out != NULL) {
		/* Convert the preset to the PHB (JSON) format */
		if(dnafx_preset_export(preset, options.phb_file_out, TRUE) < 0) {
			res = 1;
			goto done;
		}
	}
	if(options.preset_file_out == NULL && options.phb_file_out == NULL && preset != NULL)
		dnafx_preset_print_debug(preset);

	/* Check what tasks should be queued at startup */
	if(!options.offline) {
		if(!options.no_init) {
			char *command[] = { "init" };
			dnafx_tasks_add(dnafx_task_new(1, command));
		}
		if(!options.no_get_presets) {
			char *command[] = { "get-presets" };
			dnafx_tasks_add(dnafx_task_new(1, command));
		}
		if(!options.no_get_extras) {
			char *command[] = { "get-extras" };
			dnafx_tasks_add(dnafx_task_new(1, command));
		}
		if(options.change_preset > 0) {
			if(options.change_preset > 200) {
				DNAFX_LOG(DNAFX_LOG_WARN, "Invalid preset number %d\n", options.change_preset);
			} else {
				char preset_number[4];
				g_snprintf(preset_number, sizeof(preset_number), "%d", options.change_preset);
				char *command[] = { "change-preset", preset_number };
				dnafx_tasks_add(dnafx_task_new(2, command));
			}
		}
		if(options.upload_preset > 0) {
			if(preset == NULL) {
				DNAFX_LOG(DNAFX_LOG_WARN, "Can't upload a preset, none was imported\n");
			} else if(options.upload_preset > 200) {
				DNAFX_LOG(DNAFX_LOG_WARN, "Invalid preset number %d\n", options.upload_preset);
			} else {
				char preset_number[4];
				g_snprintf(preset_number, sizeof(preset_number), "%d", options.upload_preset);
				char *command[] = { "upload-preset", preset_number, preset->name };
				dnafx_tasks_add(dnafx_task_new(3, command));
			}
		}
	}
	if(options.interactive) {
		char *command[] = { "cli" };
		dnafx_tasks_add(dnafx_task_new(1, command));
	} else {
		char *command[] = { "quit" };
		dnafx_tasks_add(dnafx_task_new(1, command));
	}

	/* Handle SIGINT (CTRL-C), SIGTERM (from service managers) */
	signal(SIGINT, dnafx_handle_signal);
	signal(SIGTERM, dnafx_handle_signal);

	/* Connect to the device */
	if(!options.offline && dnafx_usb_init(options.debug_libusb) < 0) {
		res = 1;
		goto done;
	}

	/* Loop */
	int fds_num = 0, i = 0, ret = 0, timeout = 0;
	struct pollfd fds[20];
	const struct libusb_pollfd **usb_fds = dnafx_usb_fds();
	struct timeval tv = { 0 };
	while(!g_atomic_int_get(&stop)) {
		/* Check what the next timeout for libusb is */
		ret = dnafx_usb_get_next_timeout(&tv);
		if(ret == -1) {
			/* FIXME Something went wrong? */
			break;
		} else if(ret == 1 || !dnafx_tasks_is_empty()) {
			/* We need to do something with USB right away */
			dnafx_usb_step();
			continue;
		}
		timeout = DNAFX_POLL_TIMEOUT;
		if(tv.tv_sec == 0 && tv.tv_sec > 0 && (tv.tv_usec/1000) < timeout)
			timeout = tv.tv_usec/1000;
		/* Prepare the file descriptors to monitor */
		fds_num = 0;
		/* Track the standard input, for the embedded CLI */
		fds[fds_num].fd = 0;
		fds[fds_num].events = POLLIN;
		fds[fds_num].revents = 0;
		fds_num++;
		/* Track libusb file descriptors */
		if(usb_fds != NULL) {
			for(i=0; usb_fds[i] != NULL; i++) {
				fds[fds_num].fd = usb_fds[i]->fd;
				fds[fds_num].events = usb_fds[i]->events;
				fds[fds_num].revents = 0;
				fds_num++;
			}
		}
		/* Poll the file descriptors */
		ret = poll(fds, fds_num, timeout);
		if(ret < 0) {
			if(!stop)
				DNAFX_LOG(DNAFX_LOG_ERR, "Polling error: %d (%s)\n", errno, g_strerror(errno));
			break;
		} else if(ret > 0) {
			/* Check what changed */
			for(i=0; i<fds_num; i++) {
				if(fds[i].revents & (POLLERR | POLLHUP)) {
					if(errno != EINTR) {
						DNAFX_LOG(DNAFX_LOG_ERR, "Error polling %d (socket #%d): %s\n",
							fds[i].fd, i, fds[i].revents & POLLERR ? "POLLERR" : "POLLHUP");
					}
				} else if(fds[i].fd == 0 && fds[i].revents & POLLIN) {
					/* We have data on stdin, pass it to the CLI */
					char ch = dnafx_getch();
					if(embedded_cli_insert_char(&cli, ch)) {
						int cli_argc;
						char **cli_argv;
						cli_argc = embedded_cli_argc(&cli, &cli_argv);
						if(cli_argc > 0)
							dnafx_tasks_add(dnafx_task_new(cli_argc, cli_argv));
						embedded_cli_prompt(&cli);
					}
				} else {
					/* We need to do something with USB */
					dnafx_usb_step();
				}
			}
		}
	}

done:
	/* Cleanup */
	dnafx_tasks_deinit();
	dnafx_presets_deinit();
	dnafx_usb_deinit();

	/* Done */
	dnafx_options_destroy();
	DNAFX_PRINT("\nBye!\n");
	exit(res);
}

/* Start the CLI if it wasn't there already */
void dnafx_cli(void) {
	if(g_atomic_int_compare_and_exchange(&cli_started, 0, 1)) {
		/* Setup a terminal using EmbeddedCLI */
		embedded_cli_init(&cli, "DNAfx> ", dnafx_putch, stdout);
		embedded_cli_prompt(&cli);
	}
}

/* Helper to output a single character to stdout (copied from the EmbeddedCLI example) */
static void dnafx_putch(void *data, char ch, bool is_last) {
	FILE *fp = data;
	fputc(ch, fp);
	if(is_last)
		fflush(fp);
}

/* Helper to read one character from stdin (copied from the EmbeddedCLI example) */
static char dnafx_getch(void) {
	char buf = 0;
	struct termios old = {0};
	if(tcgetattr(0, &old) < 0)
		perror("tcsetattr()");

	struct termios raw = old;

	raw.c_iflag &=
		~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	raw.c_oflag &= ~OPOST;
	raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	raw.c_cflag &= ~(CSIZE | PARENB);
	raw.c_cflag |= CS8;

	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if(tcsetattr(0, TCSANOW, &raw) < 0)
		perror("tcsetattr ICANON");
	if(read(0, &buf, 1) < 0)
		perror("read()");
	if(tcsetattr(0, TCSADRAIN, &old) < 0)
		perror("tcsetattr ~ICANON");
	return (buf);
}

/* Quit */
void dnafx_quit(void) {
	g_atomic_int_set(&stop, 1);
}
