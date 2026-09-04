#ifndef KMOD_UTILS_H
#define KMOD_UTILS_H

/* return 1 loaded, 0 not loaded, -1 error */
int kmod_is_loaded(const char *module_name);

/* load a .ko file; return 0 success, -1 fail */
int kmod_insmod_file(const char *ko_path, const char *params);

/* wait for device node; return 0 ok, -1 timeout */
int wait_devnode(const char *path, int timeout_ms);

#endif
