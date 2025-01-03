#include <ctype.h>
#include <sys/stat.h>

#include "utils.h"
#include "debug.h"

/* Print the contents of a buffer as hex values */
void dnafx_print_hex(int level, const char *separator, uint8_t *buf, size_t buflen) {
	DNAFX_LOG(level, "\t");
	for(size_t i=0; i<buflen; ++i)
		DNAFX_LOG(level, "%02x%s", buf[i], (separator ? separator : ""));
	DNAFX_LOG(level, "\n");
}

/* Trim a string */
void dnafx_trim_string(char *str) {
	int start = 0, end = strlen(str) - 1;
	while(isspace((unsigned char)str[start]))
		start++;
	while((end >= start) && isspace((unsigned char)str[end]))
		end--;
	str[end + 1] = '\0';
	memmove(str, str + start, end - start + 1);
}

/* Create a directory */
int dnafx_mkdir(const char *dir, mode_t mode) {
	char tmp[256];
	char *p = NULL;
	int res = 0;
	g_snprintf(tmp, sizeof(tmp), "%s", dir);
	size_t len = strlen(tmp);
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++) {
		if(*p == '/') {
			*p = 0;
			res = mkdir(tmp, mode);
			if(res != 0 && errno != EEXIST) {
				DNAFX_LOG(DNAFX_LOG_ERR, "Error creating folder %s\n", tmp);
				return res;
			}
			*p = '/';
		}
	}
	res = mkdir(tmp, mode);
	if(res != 0 && errno != EEXIST)
		return res;
	return 0;
}

/* Read a file, and put its contents in the provided buffer */
int dnafx_read_file(const char *filename, gboolean text, uint8_t *buffer, size_t blen) {
	if(filename == NULL || buffer == NULL || blen < 1) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	/* Open the file for reading */
	DNAFX_LOG(DNAFX_LOG_INFO, "Opening %s file '%s'...\n", (text ? "text" : "binary"), filename);
	FILE *file = fopen(filename, (text ? "rt" : "rb"));
	if(file == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Error opening file '%s' for reading: %d (%s)\n",
			filename, errno, g_strerror(errno));
		return -1;
	}
	/* Check if the buffer is large enough */
	fseek(file, 0L, SEEK_END);
	size_t fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);
	if(fsize > blen) {
		DNAFX_LOG(DNAFX_LOG_WARN, "Provided buffer is smaller than the file contents (%zu < %zu), it will be truncated\n",
			blen, fsize);
	}
	/* Read the content */
	size_t bytes = fread(buffer, 1, blen, file);
	if(bytes == 0) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Error reading data: %d (%s)\n", errno, g_strerror(errno));
		fclose(file);
		return -1;
	}
	fclose(file);
	DNAFX_LOG(DNAFX_LOG_INFO, "Read %zu bytes\n", bytes);
	/* Done */
	if(text)
		buffer[bytes] = '\0';
	return bytes;
}

/* Open a file, and write the provided buffer to it */
int dnafx_write_file(const char *filename, gboolean text, uint8_t *buffer, size_t blen) {
	if(filename == NULL || buffer == NULL || blen < 1) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Invalid arguments\n");
		return -1;
	}
	/* Open the file for writing */
	DNAFX_LOG(DNAFX_LOG_INFO, "Opening %s file '%s'...\n", (text ? "text" : "binary"), filename);
	FILE *file = fopen(filename, (text ? "wt" : "wb"));
	if(file == NULL) {
		DNAFX_LOG(DNAFX_LOG_ERR, "Error opening file '%s' for writing: %d (%s)\n",
			filename, errno, g_strerror(errno));
		return -1;
	}
	/* Write the content of the buffer */
	size_t tot = blen, temp = 0;
	while(tot > 0) {
		temp = fwrite(buffer + blen - tot, 1, tot, file);
		if(temp == 0) {
			DNAFX_LOG(DNAFX_LOG_ERR, "Error writing data to file '%s: %d (%s)\n",
				filename, errno, g_strerror(errno));
			fclose(file);
			return -1;
		}
		tot -= temp;
	}
	if(text) {
		/* Add a newline at the end */
		char c = '\n';
		fwrite(&c, 1, 1, file);
	}
	fflush(file);
	fseek(file, 0L, SEEK_END);
	size_t fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);
	DNAFX_LOG(DNAFX_LOG_INFO, "Saved buffer to file '%s' (%zu bytes)\n", filename, fsize);
	fclose(file);
	/* Done */
	return fsize;
}
