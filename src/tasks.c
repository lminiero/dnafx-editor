#include "tasks.h"
#include "presets.h"
#include "utils.h"
#include "debug.h"

/* Queue of tasks */
static GAsyncQueue *tasks = NULL;

/* Stringify task type */
const char *dnafx_task_type_str(dnafx_task_type type) {
	switch(type) {
		case DNAFX_TASK_CLI:
			return "cli";
		case DNAFX_TASK_HELP:
			return "help";
		case DNAFX_TASK_INIT_1:
			return "init 1";
		case DNAFX_TASK_INIT_2:
			return "init 2";
		case DNAFX_TASK_INIT_RESPONSE:
			return "init resp";
		case DNAFX_TASK_GET_PRESETS_1:
			return "presets 1";
		case DNAFX_TASK_GET_PRESETS_2:
			return "presets 2";
		case DNAFX_TASK_GET_PRESETS_RESPONSE:
			return "presets resp";
		case DNAFX_TASK_GET_EXTRAS_1:
			return "extras 1";
		case DNAFX_TASK_GET_EXTRAS_2:
			return "extras 2";
		case DNAFX_TASK_GET_EXTRAS_RESPONSE:
			return "extras resp";
		case DNAFX_TASK_CHANGE_PRESET:
			return "change";
		case DNAFX_TASK_RENAME_PRESET:
			return "rename";
		case DNAFX_TASK_UPLOAD_PRESET_1:
			return "upload 1";
		case DNAFX_TASK_UPLOAD_PRESET_2:
			return "upload 2";
		case DNAFX_TASK_UPLOAD_PRESET_3:
			return "upload 3";
		case DNAFX_TASK_UPLOAD_PRESET_4:
			return "upload 4";
		case DNAFX_TASK_UPLOAD_PRESET_RESPONSE:
			return "upload resp";
		case DNAFX_TASK_INTERRUPT:
			return "interrupt";
		case DNAFX_TASK_LIST_PRESETS:
			return "list presets";
		case DNAFX_TASK_QUIT:
			return "quit";
		case DNAFX_TASK_NONE:
		default:
			break;
	}
	return NULL;
};

/* Create a new task out of a command */
dnafx_task *dnafx_task_new(int argc, char **argv) {
	if(argc == 0 || argv == NULL)
		return NULL;
	dnafx_task *task = g_malloc0(sizeof(dnafx_task));
	if(!strcasecmp(argv[0], "cli")) {
		task->type = DNAFX_TASK_CLI;
	} else if(!strcasecmp(argv[0], "help")) {
		task->type = DNAFX_TASK_HELP;
	} else if(!strcasecmp(argv[0], "quit")) {
		task->type = DNAFX_TASK_QUIT;
	} else if(!strcasecmp(argv[0], "init")) {
		task->type = DNAFX_TASK_INIT_1;
	} else if(!strcasecmp(argv[0], "get-presets")) {
		task->type = DNAFX_TASK_GET_PRESETS_1;
	} else if(!strcasecmp(argv[0], "get-extras")) {
		task->type = DNAFX_TASK_GET_EXTRAS_1;
	} else if(!strcasecmp(argv[0], "change-preset")) {
		if(argc < 2) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'change-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		int preset_number = atoi(argv[1]);
		if(preset_number < 1 || preset_number > 200) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'change-preset' format\n");
			dnafx_task_free(task);
			task = NULL;
		} else {
			task->type = DNAFX_TASK_CHANGE_PRESET;
			task->number[0] = preset_number;
		}
	} else if(!strcasecmp(argv[0], "rename-preset")) {
		if(argc < 3) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'rename-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		int preset_number = atoi(argv[1]);
		if(preset_number < 1 || preset_number > 200) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'rename-preset' format\n");
			dnafx_task_free(task);
			task = NULL;
		} else {
			task->type = DNAFX_TASK_UPLOAD_PRESET_1;
			task->number[0] = preset_number;
			task->text[0] = g_strdup(argv[2]);
			/* TODO */
			DNAFX_LOG(DNAFX_LOG_WARN, "[task] Rename preset TBD.\n");
			dnafx_task_free(task);
			task = NULL;
		}
	} else if(!strcasecmp(argv[0], "upload-preset")) {
		if(argc < 3) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'upload-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		int preset_number = atoi(argv[1]);
		if(preset_number < 1 || preset_number > 200) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'upload-preset' format\n");
			dnafx_task_free(task);
			task = NULL;
		} else {
			task->type = DNAFX_TASK_UPLOAD_PRESET_1;
			task->number[0] = preset_number;
			task->text[0] = g_strdup(argv[2]);
		}
	} else if(!strcasecmp(argv[0], "interrupt")) {
		task->type = DNAFX_TASK_INTERRUPT;
	} else if(!strcasecmp(argv[0], "list-presets")) {
		task->type = DNAFX_TASK_LIST_PRESETS;
	} else if(!strcasecmp(argv[0], "import-preset")) {
		if(argc < 3) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'import-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		if(strcasecmp(argv[1], "binary") && strcasecmp(argv[1], "phb")) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Unsupported 'import-preset' format '%s'\n", argv[1]);
			dnafx_task_free(task);
			return NULL;
		}
		task->type = DNAFX_TASK_IMPORT_PRESET;
		task->text[0] = g_strdup(argv[1]);
		task->text[1] = g_strdup(argv[2]);
	} else if(!strcasecmp(argv[0], "parse-preset")) {
		if(argc < 2) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'parse-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		task->type = DNAFX_TASK_PARSE_PRESET;
		task->number[0] = atoi(argv[1]);
		if(strlen(argv[1]) > 3 || task->number[0] == 0) {
			task->number[0] = 0;
			task->text[0] = g_strdup(argv[1]);
		}
	} else if(!strcasecmp(argv[0], "export-preset")) {
		if(argc < 4) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Invalid 'export-preset' format\n");
			dnafx_task_free(task);
			return NULL;
		}
		if(strcasecmp(argv[2], "binary") && strcasecmp(argv[2], "phb")) {
			DNAFX_LOG(DNAFX_LOG_WARN, "Unsupported 'export-preset' format '%s'\n", argv[2]);
			dnafx_task_free(task);
			return NULL;
		}
		task->type = DNAFX_TASK_EXPORT_PRESET;
		task->number[0] = atoi(argv[1]);
		if(strlen(argv[1]) > 3 || task->number[0] == 0) {
			task->number[0] = 0;
			task->text[0] = g_strdup(argv[1]);
		}
		task->text[1] = g_strdup(argv[2]);
		task->text[2] = g_strdup(argv[3]);
	} else {
		DNAFX_LOG(DNAFX_LOG_WARN, "Unsupported command '%s'\n", argv[0]);
		dnafx_task_free(task);
		task = NULL;
	}
	return task;
}

/* Free a task */
void dnafx_task_free(dnafx_task *task) {
	if(task) {
		g_free(task->text[0]);
		g_free(task->text[1]);
		g_free(task->text[2]);
		g_free(task->text[3]);
		g_free(task);
	}
}

/* Tasks management */
void dnafx_task_show_help(void) {
	DNAFX_LOG(DNAFX_LOG_INFO,
		"\nSupported CLI commands:\n\n"
		"\t" DNAFX_COLOR_BOLD "help" DNAFX_COLOR_OFF "\n\t\tShow this message\n\n"
		"\t" DNAFX_COLOR_BOLD "init" DNAFX_COLOR_OFF "\n\t\tSend the initialization messages\n\n"
		"\t" DNAFX_COLOR_BOLD "get-presets" DNAFX_COLOR_OFF "\n\t\tRetrieve the presets from the device\n\n"
		"\t" DNAFX_COLOR_BOLD "get-extras" DNAFX_COLOR_OFF "\n\t\tRetrieve the list of extras from the device\n\n"
		"\t" DNAFX_COLOR_BOLD "change-preset <number>" DNAFX_COLOR_OFF "\n\t\tChange the active preset on the device\n\n"
		"\t" DNAFX_COLOR_BOLD "upload-preset \"<name>\" <slot>" DNAFX_COLOR_OFF "\n\t\tUpload a named preset to the specified slot on the device\n\n"
		"\t" DNAFX_COLOR_BOLD "import-preset <binary|phb> \"filename\"" DNAFX_COLOR_OFF "\n\t\tImport the specified binary or PHB preset\n\n"
		"\t" DNAFX_COLOR_BOLD "parse-preset <number>|\"name\"" DNAFX_COLOR_OFF "\n\t\tPrints the content of the specified preset\n\n"
		"\t" DNAFX_COLOR_BOLD "export-preset <number>|\"name\" <binary|phb> \"filename\"" DNAFX_COLOR_OFF "\n\t\tExport the specified preset as a binary of PHB file\n\n"
		"\t" DNAFX_COLOR_BOLD "list-presets" DNAFX_COLOR_OFF "\n\t\tPrints the list of known presets\n\n"
		"\t" DNAFX_COLOR_BOLD "quit" DNAFX_COLOR_OFF "\n\t\tClose the editor\n\n");
}

void dnafx_tasks_init(void) {
	tasks = g_async_queue_new_full((GDestroyNotify)dnafx_task_free);
}

void dnafx_tasks_add(dnafx_task *task) {
	if(tasks != NULL && task != NULL)
		g_async_queue_push(tasks, task);
}

gboolean dnafx_tasks_is_empty(void) {
	return tasks ? (g_async_queue_length(tasks) == 0) : TRUE;
}

dnafx_task *dnafx_tasks_next(void) {
	dnafx_task *task = tasks ? g_async_queue_try_pop(tasks) : NULL;
	return task;
}

void dnafx_tasks_deinit(void) {
	if(tasks != NULL)
		g_async_queue_unref(tasks);
	tasks = NULL;
}
