#include <sys/stat.h>

#include "presets.h"
#include "effects.h"
#include "utils.h"
#include "debug.h"

/* Tables */
static GHashTable *presets_byid = NULL;
static GHashTable *presets_byname = NULL;

/* Presets state */
static char *presets_folder = NULL;
int dnafx_presets_init(const char *folder) {
	presets_byid = g_hash_table_new_full(NULL, NULL,
		NULL, (GDestroyNotify)dnafx_preset_free);
	presets_byname = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)dnafx_preset_free);
	if(folder == NULL) {
		DNAFX_LOG(DNAFX_LOG_INFO, "Presets folder: none (won't save retrieved presets)\n");
		return 0;
	}
	presets_folder = g_strdup(folder);
	/* Check if this directory exists, and create it if needed */
	struct stat s;
	int err = stat(presets_folder, &s);
	if(err == -1) {
		if(ENOENT == errno) {
			/* Directory does not exist, try creating it */
			if(dnafx_mkdir(presets_folder, 0644) < 0) {
				DNAFX_LOG(DNAFX_LOG_ERR, "mkdir (%s) error: %d (%s)\n", presets_folder, errno, g_strerror(errno));
				return -1;
			}
		} else {
			DNAFX_LOG(DNAFX_LOG_ERR, "stat (%s) error: %d (%s)\n", presets_folder, errno, g_strerror(errno));
			return -1;
		}
	} else {
		if(S_ISDIR(s.st_mode)) {
			/* Directory exists */
			DNAFX_LOG(DNAFX_LOG_VERB, "Directory exists: %s\n", presets_folder);
		} else {
			/* File exists but it's not a directory? */
			DNAFX_LOG(DNAFX_LOG_ERR, "Not a directory? %s\n", presets_folder);
			return -1;
		}
	}
	DNAFX_LOG(DNAFX_LOG_INFO, "Presets folder: %s\n", presets_folder);
	return 0;
}

char *dnafx_presets_folder(void) {
	return presets_folder;
}

void dnafx_presets_deinit(void) {
	g_free(presets_folder);
	presets_folder = NULL;
	if(presets_byid != NULL)
		g_hash_table_unref(presets_byid);
	presets_byid = NULL;
	if(presets_byname != NULL)
		g_hash_table_unref(presets_byname);
	presets_byname = NULL;
}

/* Presets structure */
void dnafx_preset_print_debug(dnafx_preset *preset) {
	if(preset == NULL)
		return;
	DNAFX_LOG(DNAFX_LOG_INFO, "ID:   %d\n", preset->id);
	DNAFX_LOG(DNAFX_LOG_INFO, "Name: %s\n", preset->name);
	DNAFX_LOG(DNAFX_LOG_INFO, "Effects\n");
	uint8_t i = 0, j = 0;
	for(i=0; i<9; i++) {
		DNAFX_LOG(DNAFX_LOG_INFO, "  -- %s\n", dnafx_sections[i].name);
		DNAFX_LOG(DNAFX_LOG_INFO, "  -- -- State: %s\n", preset->effects[i].active ? "on" : "off");
		dnafx_effect *f = dnafx_sections[i].effects + preset->effects[i].id;
		DNAFX_LOG(DNAFX_LOG_INFO, "  -- -- Effect: %s\n", f->name);
		for(j=0; j<6; j++) {
			if(f->param_names[j] == NULL)
				continue;
			DNAFX_LOG(DNAFX_LOG_INFO, "  -- -- -- %s: %"SCNu16"\n",
				f->param_names[j], preset->effects[i].values[j]);
		}
	}
	DNAFX_LOG(DNAFX_LOG_INFO, "Expression\n");
	for(i=0; i<6; i++)
		DNAFX_LOG(DNAFX_LOG_INFO, "  -- %s: %"SCNu16"\n", dnafx_expression[i], preset->expressions[i]);
}

void dnafx_preset_free(dnafx_preset *preset) {
	g_free(preset);
}

/* Internal parsing */
static int dnafx_parse_effect(dnafx_preset *preset, uint8_t index, uint8_t *effect);
static int dnafx_parse_expression(dnafx_preset *preset, uint8_t *exp, size_t elen);

/* Parse a preset from a byte array */
dnafx_preset *dnafx_preset_from_bytes(uint8_t *buf, size_t blen) {
	dnafx_preset *preset = g_malloc0(sizeof(dnafx_preset));
	DNAFX_LOG(DNAFX_LOG_VERB, "Parsing preset (%zu bytes)\n", blen);
	dnafx_print_hex(DNAFX_LOG_HUGE, NULL, buf, blen);
	size_t offset = 0;
	/* Preset ID */
	preset->id = buf[offset];
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- ID: %d (%02x)\n", preset->id, buf[offset]);
	offset++;
	/* Preset name */
	snprintf(preset->name, sizeof(preset->name), "%.*s", DNAFX_PRESET_NAME_SIZE, buf + offset);
	dnafx_trim_string(preset->name);
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- Name: '%s'\n", preset->name);
	dnafx_print_hex(DNAFX_LOG_HUGE, " ", buf + offset, 14);
	offset += DNAFX_PRESET_NAME_SIZE;
	/* Effects */
	size_t s_size = sizeof(dnafx_sections);
	size_t num = s_size / sizeof(dnafx_section), i = 0;
	for(i=0; i<num; i++) {
		if(dnafx_parse_effect(preset, i, buf + offset) < 0) {
			dnafx_preset_free(preset);
			return NULL;
		}
		offset += dnafx_sections[i].size;
	}
	/* Expression pedal */
	if(dnafx_parse_expression(preset, buf + offset, 12) < 0) {
		dnafx_preset_free(preset);
		return NULL;
	}
	offset += 12;
	/* Done? Padding? */
	DNAFX_LOG(DNAFX_LOG_VERB, "Parsed %zu/%zu bytes\n", offset, blen);
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- Remaining %zu bytes\n", blen - offset);
	dnafx_print_hex(DNAFX_LOG_HUGE, NULL, buf + offset, blen - offset);
	return preset;
}

/* Parse an effect in a preset */
static int dnafx_parse_effect(dnafx_preset *preset, uint8_t index, uint8_t *effect) {
	dnafx_section *section = &dnafx_sections[index];
	preset->effects[index].type = section->id;
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- %s\n", section->name);
	dnafx_print_hex(DNAFX_LOG_HUGE, " ", effect, section->size);
	size_t offset = 0;
	uint16_t value = 0;
	memcpy(&value, effect + offset, 2);
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- -- State: %s (%02x%02x)\n", (value ? "on" : "off"), *effect, *(effect + 1));
	preset->effects[index].active = (value ? TRUE : FALSE);
	offset += 2;
	memcpy(&value, effect + offset, 2);
	preset->effects[index].id = value;
	dnafx_effect *f = section->effects ? (section->effects + preset->effects[index].id) : NULL;
	if(f == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Unknown effect\n");
		return -1;
	}
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- -- Effect: %s (%d, %02x%02x)\n", f->name,
		f->id, *(effect + offset), *(effect + offset + 1));
	offset += 2;
	uint8_t i = 0;
	for(i=0; i<f->params; i++) {
		memcpy(&value, effect + offset, 2);
		DNAFX_LOG(DNAFX_LOG_VERB, "  -- -- -- %s: %"SCNu16" (%02x%02x)\n", f->param_names[i],
			value, *(effect + offset), *(effect + offset + 1));
		preset->effects[index].values[i] = value;
		offset += 2;
	}
	return 0;
}

/* Parse the expressions in a preset */
static int dnafx_parse_expression(dnafx_preset *preset, uint8_t *exp, size_t elen) {
	DNAFX_LOG(DNAFX_LOG_VERB, "  -- Expression pedal\n");
	dnafx_print_hex(DNAFX_LOG_HUGE, " ", exp, elen);
	size_t offset = 0;
	uint16_t value = 0;
	size_t s_size = sizeof(dnafx_expression);
	size_t num = s_size / sizeof(const char *), i = 0;
	for(i=0; i<num; i++) {
		memcpy(&value, exp + offset, 2);
		DNAFX_LOG(DNAFX_LOG_VERB, "  -- -- %s: %"SCNu16" (%02x%02x)\n", dnafx_expression[i],
			value, *(exp + offset), *(exp + offset + 1));
		preset->expressions[i] = value;
		offset += 2;
	}
	return 0;
}

/* Parse preset from a PHB file (JSON) */
dnafx_preset *dnafx_preset_from_phb(const char *phb) {
	if(phb == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return NULL;
	}
	/* Parse the PHB string as JSON text */
	json_error_t error;
	json_t *json = json_loads(phb, 0, &error);
	if(json == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "JSON error: on line %d: %s", error.line, error.text);
		return NULL;
	}
	if(!json_is_object(json)) {
		json_decref(json);
		DNAFX_LOG(DNAFX_LOG_ERR, "JSON error: not an object");
		return NULL;
	}
	json_t *exp = json_object_get(json, "Exp");
	json_t *em = json_object_get(json, "effectModule");
	json_t *info = json_object_get(json, "fileInfo");
	if(exp == NULL || !json_is_object(exp) || em == NULL || !json_is_object(em) ||
			info == NULL || !json_is_object(info)) {
		json_decref(json);
		DNAFX_LOG(DNAFX_LOG_ERR, "Missing mandatory object (Exp, effectModule and/or fileInfo)");
		return NULL;
	}
	/* Create a preset instance */
	dnafx_preset *preset = g_malloc0(sizeof(dnafx_preset));
	/* Info */
	json_t *name = json_object_get(info, "preset_name");
	if(name == NULL || !json_is_string(name)) {
		json_decref(json);
		dnafx_preset_free(preset);
		DNAFX_LOG(DNAFX_LOG_ERR, "Missing preset name");
		return NULL;
	}
	g_snprintf(preset->name, sizeof(preset->name), "%s", json_string_value(name));
	dnafx_trim_string(preset->name);
	/* Effects */
	size_t s_size = sizeof(dnafx_sections);
	size_t num = s_size / sizeof(dnafx_section), i = 0;
	for(i=0; i<num; i++) {
		json_t *je = json_object_get(em, dnafx_sections[i].name);
		if(je == NULL || !json_is_object(je)) {
			json_decref(json);
			dnafx_preset_free(preset);
			DNAFX_LOG(DNAFX_LOG_ERR, "Missing mandatory effect section (%s)", dnafx_sections[i].name);
			return NULL;
		}
		json_t *type = json_object_get(je, "TYPE");
		json_t *sw = json_object_get(je, "SWITCH");
		json_t *data = json_object_get(je, "Data");
		if(type == NULL || !json_is_integer(type) || sw == NULL || !json_is_integer(sw) ||
				data == NULL || !json_is_object(data)) {
			json_decref(json);
			dnafx_preset_free(preset);
			DNAFX_LOG(DNAFX_LOG_ERR, "Missing mandatory effect property in '%s' (Data, SWITCH and/or TYPE)",
				dnafx_sections[i].name);
			return NULL;
		}
		preset->effects[i].type = i;
		preset->effects[i].id = json_integer_value(type);
		preset->effects[i].active = json_integer_value(sw) ? TRUE : FALSE;
		dnafx_effect *f = dnafx_sections[i].effects + preset->effects[i].id;
		if(f == NULL) {
			json_decref(json);
			dnafx_preset_free(preset);
			DNAFX_LOG(DNAFX_LOG_ERR, "Unknown effect\n");
			return NULL;
		}
		uint8_t j = 0;
		for(j=0; j<f->params; j++) {
			json_t *fval = json_object_get(data, f->param_names[j]);
			if(fval == NULL || !json_is_integer(fval)) {
				json_decref(json);
				dnafx_preset_free(preset);
				DNAFX_LOG(DNAFX_LOG_ERR, "Missing mandatory effect property (%s)", f->param_names[j]);
				return NULL;
			}
			preset->effects[i].values[j] = json_integer_value(fval);
		}
	}
	/* Expression pedal */
	s_size = sizeof(dnafx_expression);
	num = s_size / sizeof(const char *);
	for(i=0; i<num; i++) {
		json_t *expval = json_object_get(exp, dnafx_expression[i]);
		if(expval == NULL || !json_is_integer(expval)) {
			json_decref(json);
			dnafx_preset_free(preset);
			DNAFX_LOG(DNAFX_LOG_ERR, "Missing mandatory expression property (%s)", dnafx_expression[i]);
			return NULL;
		}
		preset->expressions[i] = json_integer_value(expval);
	}
	/* Done */
	json_decref(json);
	return preset;
}

/* Encode a preset to a byte array */
int dnafx_preset_to_bytes(dnafx_preset *preset, uint8_t *buf, size_t blen) {
	if(preset == NULL || buf == NULL || blen != DNAFX_PRESET_SIZE) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	/* Serialize the preset to its binary format */
	memset(buf, 0, blen);
	/* Write ID and name first */
	size_t offset = 0;
	buf[offset] = (uint8_t)preset->id;
	offset++;
	memcpy(&buf[offset], preset->name, DNAFX_PRESET_NAME_SIZE);
	offset += DNAFX_PRESET_NAME_SIZE;
	/* Effects */
	uint8_t i = 0, j = 0;
	uint8_t *ebuf = NULL;
	size_t eoff = 0;
	uint16_t value = 0;
	for(i=0; i<9; i++) {
		ebuf = buf + offset;
		eoff = 0;
		value = preset->effects[i].active;
		memcpy(&ebuf[eoff], &value, 2);
		eoff += 2;
		memcpy(&ebuf[eoff], &preset->effects[i].id, 2);
		eoff += 2;
		for(j=0; j<dnafx_sections[i].max_params; j++) {
			memcpy(&ebuf[eoff], &preset->effects[i].values[j], 2);
			eoff += 2;
		}
		offset += dnafx_sections[i].size;
	}
	/* Expression pedals */
	for(i=0; i<6; i++) {
		memcpy(&buf[offset], &preset->expressions[i], 2);
		offset += 2;
	}
	/* Done */
	DNAFX_LOG(DNAFX_LOG_VERB, "Preset '%s' exported to bytes\n", preset->name);
	dnafx_print_hex(DNAFX_LOG_HUGE, NULL, buf, blen);
	return offset;
}

/* Encode a preset to a PHB/JSON string */
char *dnafx_preset_to_phb(dnafx_preset *preset) {
	if(preset == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return NULL;
	}
	/* Convert the preset to JSON */
	json_t *json = json_object();
	json_t *info = json_object();
	json_object_set_new(info, "app", json_string("HB100 Edit"));
	json_object_set_new(info, "app_version", json_string("V1.0.0"));
	json_object_set_new(info, "device", json_string("HB100"));
	json_object_set_new(info, "device_version", json_string("V1.0.0"));
	json_object_set_new(info, "preset_name", json_string(preset->name));
	json_object_set_new(info, "schema", json_string("HB100 Preset"));
	json_object_set_new(json, "fileInfo", info);
	/* Effects */
	json_t *em = json_object();
	uint8_t i = 0, j = 0;
	for(i=0; i<9; i++) {
		json_t *je = json_object();
		json_object_set_new(je, "TYPE", json_integer(preset->effects[i].id));
		json_object_set_new(je, "SWITCH", json_integer(preset->effects[i].active));
		json_t *data = json_object();
		dnafx_effect *f = dnafx_sections[i].effects + preset->effects[i].id;
		for(j=0; j<f->params; j++)
			json_object_set(data, f->param_names[j], json_integer(preset->effects[i].values[j]));
		json_object_set_new(je, "Data", data);
		json_object_set_new(em, dnafx_sections[i].name, je);
	}
	json_object_set_new(json, "effectModule", em);
	/* Expression pedals */
	json_t *exp = json_object();
	for(i=0; i<6; i++)
		json_object_set(exp, dnafx_expression[i], json_integer(preset->expressions[i]));
	json_object_set_new(json, "Exp", exp);
	/* Convert the JSON to a string */
	char *json_text = json_dumps(json, JSON_INDENT(4) | JSON_SORT_KEYS);
	DNAFX_LOG(DNAFX_LOG_VERB, "Preset '%s' exported to PHB\n", preset->name);
	DNAFX_LOG(DNAFX_LOG_HUGE, "%s\n", json_text);
	/* Done */
	return json_text;
}

/* Importing and exporting */
dnafx_preset *dnafx_preset_import(const char *filename, gboolean phb) {
	if(filename == NULL)
		return NULL;
	dnafx_preset *preset = NULL;
	if(!phb) {
		/* Open the provided binary file and parse it */
		uint8_t buf[DNAFX_PRESET_SIZE];
		if(dnafx_read_file(filename, FALSE, buf, sizeof(buf)) <= 0)
			return NULL;
		preset = dnafx_preset_from_bytes(buf, sizeof(buf));
	} else {
		/* Open the provided PHB file and parse it */
		char buf[4096];
		int blen = dnafx_read_file(filename, TRUE, (uint8_t *)buf, sizeof(buf));
		if(blen <= 0)
			return NULL;
		preset = dnafx_preset_from_phb(buf);
	}
	if(preset != NULL)
		dnafx_preset_add_byname(preset);
	return preset;
}

int dnafx_preset_export(dnafx_preset *preset, const char *filename, gboolean phb) {
	if(preset == NULL || filename == NULL)
		return -1;
	if(!phb) {
		/* Convert the preset to the binary format */
		uint8_t buf[DNAFX_PRESET_SIZE];
		if(dnafx_preset_to_bytes(preset, buf, sizeof(buf)) < 0)
			return -1;
		if(dnafx_write_file(filename, FALSE, buf, sizeof(buf)) <= 0)
			return -1;
	} else {
		/* Convert the preset to the PHB (JSON) format */
		char *json_text = dnafx_preset_to_phb(preset);
		if(json_text == NULL)
			return -1;
		if(dnafx_write_file(filename, TRUE, (uint8_t *)json_text, strlen(json_text)) <= 0) {
			free(json_text);
			return -1;
		}
		free(json_text);
	}
	return 0;
}

/* Presets management */
int dnafx_preset_add_byid(dnafx_preset *preset, int id) {
	if(presets_byid == NULL || preset == NULL || id < 1 || id > 200) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	preset->id = id;
	if(!g_hash_table_insert(presets_byid, GINT_TO_POINTER(id), preset)) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Error adding preset %d to the list\n", id);
		return -1;
	}
	return 0;
}

dnafx_preset *dnafx_preset_find_byid(int id) {
	if(presets_byid == NULL || id < 1 || id > 200) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return NULL;
	}
	return g_hash_table_lookup(presets_byid, GINT_TO_POINTER(id));
}

int dnafx_preset_remove_byid(int id, gboolean unref) {
	if(presets_byid == NULL || id < 1 || id > 200) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	gboolean done = FALSE;
	if(unref) {
		done = g_hash_table_remove(presets_byid, GINT_TO_POINTER(id));
	} else {
		done = g_hash_table_steal(presets_byid, GINT_TO_POINTER(id));
	}
	return done ? 0 : -1;
}

int dnafx_preset_add_byname(dnafx_preset *preset) {
	if(presets_byname == NULL || preset == NULL || strlen(preset->name) == 0) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	char *name = g_strdup(preset->name);
	if(!g_hash_table_insert(presets_byname, name, preset)) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Error adding preset named '%s' to the list\n", name);
		g_free(name);
		return -1;
	}
	return 0;
}

dnafx_preset *dnafx_preset_find_byname(const char *name) {
	if(presets_byname == NULL || name == NULL || strlen(name) == 0) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return NULL;
	}
	return g_hash_table_lookup(presets_byname, name);
}

int dnafx_preset_remove_byname(const char *name, gboolean unref) {
	if(presets_byname == NULL || name == NULL || strlen(name) == 0) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	gboolean done = FALSE;
	if(unref) {
		done = g_hash_table_remove(presets_byname, name);
	} else {
		done = g_hash_table_steal(presets_byname, name);
	}
	return done ? 0 : -1;
}

/* Listing presets */
void dnafx_presets_print(void) {
	dnafx_preset *preset = NULL;
	DNAFX_LOG(DNAFX_LOG_INFO, "Device presets:\n");
	if(presets_byid == NULL || g_hash_table_size(presets_byid) == 0) {
		DNAFX_LOG(DNAFX_LOG_INFO, " (none)");
	} else {
		DNAFX_LOG(DNAFX_LOG_INFO, " ");
		int i = 0;
		for(i=1; i<=200; i++) {
			preset = g_hash_table_lookup(presets_byid, GINT_TO_POINTER(i));
			DNAFX_LOG(DNAFX_LOG_INFO, "[%03d] %-14s   ",
				preset ? preset->id : 0, preset ? preset->name : NULL);
			if((i % 3) == 0)
				DNAFX_LOG(DNAFX_LOG_INFO, "\n ");
		}
	}
	DNAFX_LOG(DNAFX_LOG_INFO, "\n\n");
	DNAFX_LOG(DNAFX_LOG_INFO, "Named presets:\n");
	GList *names = presets_byname ? g_hash_table_get_keys(presets_byname) : NULL;
	if(names == NULL) {
		DNAFX_LOG(DNAFX_LOG_INFO, " (none)");
	} else {
		DNAFX_LOG(DNAFX_LOG_INFO, " ");
		int i = 0;
		GList *temp = names;
		while(temp != NULL) {
			i++;
			preset = g_hash_table_lookup(presets_byname, (char *)temp->data);
			DNAFX_LOG(DNAFX_LOG_INFO, "[XXX] %-14s   ", preset ? preset->name : NULL);
			if((i % 3) == 0)
				DNAFX_LOG(DNAFX_LOG_INFO, "\n ");
			temp = temp->next;
		}
	}
	DNAFX_LOG(DNAFX_LOG_INFO, "\n\n");
	g_list_free(names);
}
