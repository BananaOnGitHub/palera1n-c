#include <errno.h>
#include <fcntl.h>              // open
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>             // exit, strtoull
#include <string.h>             // strlen, strerror, memcpy, memmove
#include <unistd.h>             // close
#include <sys/mman.h>           // mmap, munmap
#include <sys/stat.h>           // fstst

#include "common.h"

int found_pongo = 0, pongo_spin = 1;

void* pongo_helper(void* ptr) {
#if defined(__APPLE__) || defined(__linux__)
	pthread_setname_np("in.palera.pongo-helper");
#endif
	// pthread_cleanup_push(pongo_thr_cleanup, NULL);
	wait_for_pongo();
	while (1) {
		sleep(1);
	}
	return NULL;
}

void *pongo_usb_callback(void *arg) {
	if (found_pongo) {
		LOG(LOG_WARNING, "PongoOS already found, not doing anything");
		return NULL;
	}
#if defined(__APPLE__) || defined(__linux__)
	pthread_setname_np("in.palera.pongo-callback");
#endif
	found_pongo = 1;
	LOG(LOG_INFO, "Found PongoOS USB Device");
	usb_device_handle_t handle = *(usb_device_handle_t *)arg;
	issue_pongo_command(handle, NULL);
	issue_pongo_command(handle, "fuse lock");
	issue_pongo_command(handle, "sep auto");
	upload_pongo_file(handle, **kpf_to_upload, checkra1n_kpf_pongo_len);
	issue_pongo_command(handle, "modload");
	issue_pongo_command(handle, kpf_flags_cmd);
	issue_pongo_command(handle, checkrain_flags_cmd);
	issue_pongo_command(handle, palerain_flags_cmd);
	if (enable_rootful)
	{
		issue_pongo_command(handle, rootfs_cmd);
		issue_pongo_command(handle, dtpatch_cmd);
	}
	strcat(xargs_cmd, " rootdev=md0");
	upload_pongo_file(handle, **ramdisk_to_upload, ramdisk_dmg_len);
	issue_pongo_command(handle, "ramdisk");
	upload_pongo_file(handle, **overlay_to_upload, binpack_dmg_len);
	issue_pongo_command(handle, "overlay");
	issue_pongo_command(handle, xargs_cmd);
	issue_pongo_command(handle, "kpf");
	issue_pongo_command(handle, "bootux");
	LOG(LOG_INFO, "Booting Kernel...");
	device_has_booted = 1;
	if (dfuhelper_thr_running) {
		pthread_cancel(dfuhelper_thread);
	}
	spin = false;
	pongo_spin = 0;
	return NULL;
}
