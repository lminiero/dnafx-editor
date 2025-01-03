#ifndef DNAFX_TASKS
#define DNAFX_TASKS

#include <stddef.h>
#include <stdint.h>

#include <glib.h>

/* Task type */
typedef enum dnafx_task_type {
	DNAFX_TASK_NONE = 0,
	DNAFX_TASK_CLI,
	DNAFX_TASK_HELP,
	DNAFX_TASK_INIT_1,
	DNAFX_TASK_INIT_2,
	DNAFX_TASK_INIT_RESPONSE,
	DNAFX_TASK_GET_PRESETS_1,
	DNAFX_TASK_GET_PRESETS_2,
	DNAFX_TASK_GET_PRESETS_RESPONSE,
	DNAFX_TASK_GET_EXTRAS_1,
	DNAFX_TASK_GET_EXTRAS_2,
	DNAFX_TASK_GET_EXTRAS_RESPONSE,
	DNAFX_TASK_CHANGE_PRESET,
	DNAFX_TASK_RENAME_PRESET,
	DNAFX_TASK_UPLOAD_PRESET_1,
	DNAFX_TASK_UPLOAD_PRESET_2,
	DNAFX_TASK_UPLOAD_PRESET_3,
	DNAFX_TASK_UPLOAD_PRESET_4,
	DNAFX_TASK_UPLOAD_PRESET_RESPONSE,
	DNAFX_TASK_INTERRUPT,
	DNAFX_TASK_LIST_PRESETS,
	DNAFX_TASK_IMPORT_PRESET,
	DNAFX_TASK_PARSE_PRESET,
	DNAFX_TASK_EXPORT_PRESET,
	DNAFX_TASK_QUIT,
} dnafx_task_type;
const char *dnafx_task_type_str(dnafx_task_type type);

/* Task */
typedef struct dnafx_task {
	/* What we should do */
	dnafx_task_type type;
	/* Numbers, if needed */
	int number[4];
	/* Strings, if needed */
	char *text[4];
} dnafx_task;
/* Create a new task out of a command */
dnafx_task *dnafx_task_new(int argc, char **argv);
/* Free a task */
void dnafx_task_free(dnafx_task *task);

/* Tasks management */
void dnafx_task_show_help(void);
void dnafx_tasks_init(void);
void dnafx_tasks_add(dnafx_task *task);
gboolean dnafx_tasks_is_empty(void);
dnafx_task *dnafx_tasks_next(void);
void dnafx_tasks_deinit(void);

#endif
