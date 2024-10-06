#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/timing/timing.h>

LOG_MODULE_REGISTER(main);

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255
#define TEST_FILE_SIZE 547

#ifdef CONFIG_APP_LITTLEFS_STORAGE_FLASH
static int littlefs_flash_erase(unsigned int id)
{
	const struct flash_area *pfa;
	int rc;

	rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d\n",
			id, rc);
		return rc;
	}

	LOG_PRINTK("Area %u at 0x%x on %s for %u bytes\n",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	/* Optional wipe flash contents */
	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		LOG_ERR("Erasing flash area ... %d", rc);
	}

	flash_area_close(pfa);
	return rc;
}

int littlefs_write(const char *path, const void *data, size_t size)
{
    struct fs_file_t file;
    int rc;

    fs_file_t_init(&file);

    rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot open file %s: %d\n", path, rc);
        return rc;
    }

    rc = fs_write(&file, data, size);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot write to file %s: %d\n", path, rc);
        fs_close(&file);
        return rc;
    }

    LOG_PRINTK("Wrote %d bytes to %s\n", size, path);

    rc = fs_close(&file);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot close file %s: %d\n", path, rc);
        return rc;
    }

    return 0;
}

int littlefs_read(const char *path, void *buffer, size_t size)
{
    struct fs_file_t file;
    int rc;

    fs_file_t_init(&file);

    rc = fs_open(&file, path, FS_O_READ);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot open file %s: %d\n", path, rc);
        return rc;
    }

    rc = fs_read(&file, buffer, size);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot read from file %s: %d\n", path, rc);
        fs_close(&file);
        return rc;
    }

    LOG_PRINTK("Read %d bytes from %s\n", size, path);

    rc = fs_close(&file);
    if (rc < 0) {
        LOG_PRINTK("FAIL: cannot close file %s: %d\n", path, rc);
        return rc;
    }

    return 0;
}

#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};
#endif /* PARTITION_NODE */

	struct fs_mount_t *mountpoint =
#if DT_NODE_EXISTS(PARTITION_NODE)
		&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
		&lfs_storage_mnt
#endif
		;

static int littlefs_mount(struct fs_mount_t *mp)
{
	int rc;

	rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
	if (rc < 0) {
		return rc;
	}

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||						\
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_PRINTK("FAIL: mount id %" PRIuPTR " at %s: %d\n",
		       (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return rc;
	}
	LOG_PRINTK("%s mount: %d\n", mp->mnt_point, rc);
#else
	LOG_PRINTK("%s automounted\n", mp->mnt_point);
#endif

	return 0;
}
#endif /* CONFIG_APP_LITTLEFS_STORAGE_FLASH */

#ifdef CONFIG_APP_LITTLEFS_STORAGE_BLK_SDMMC

#if defined(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME CONFIG_SDMMC_VOLUME_NAME
#elif IS_ENABLED(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME CONFIG_MMC_VOLUME_NAME
#else
#error "No disk device defined, is your board supported?"
#endif

struct fs_littlefs lfsfs;
static struct fs_mount_t __mp = {
	.type = FS_LITTLEFS,
	.fs_data = &lfsfs,
	.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};
struct fs_mount_t *mountpoint = &__mp;

static int littlefs_mount(struct fs_mount_t *mp)
{
	static const char *disk_mount_pt = "/"DISK_NAME":";
	static const char *disk_pdrv = DISK_NAME;

	mp->storage_dev = (void *)disk_pdrv;
	mp->mnt_point = disk_mount_pt;

	return fs_mount(mp);
}
#endif /* CONFIG_APP_LITTLEFS_STORAGE_BLK_SDMMC */

static int perform_write_and_read(const char *path, const char *data_to_write, size_t write_size, char *read_buffer, size_t read_size)
{
    int rc;
    uint32_t start_time, end_time;

    // Write operation
    start_time = k_cycle_get_32();
    rc = littlefs_write(path, data_to_write, write_size);
    end_time = k_cycle_get_32();
    uint32_t write_time_us = k_cyc_to_us_floor32(end_time - start_time);
    LOG_PRINTK("Time taken to write: %u microseconds\n", write_time_us);

    if (rc < 0) {
        // Handle error
        LOG_PRINTK("Error during writing to file %s\n", path);
        return rc;
    }
    LOG_PRINTK("Successfully wrote %s to file: %s\n",data_to_write, path);

    // Read operation
    start_time = k_cycle_get_32();
    rc = littlefs_read(path, read_buffer, read_size);
    end_time = k_cycle_get_32();
    uint32_t read_time_us = k_cyc_to_us_floor32(end_time - start_time);
    LOG_PRINTK("Time taken to read: %u microseconds\n", read_time_us);

    if (rc < 0) {
        // Handle error
        LOG_PRINTK("Error during reading from file %s\n", path);
        return rc;
    }
    LOG_PRINTK("Successfully read data from file: %s\n", read_buffer);

    return 0;
}

int main(void)
{
	char fname1[MAX_PATH_LEN];
	char fname2[MAX_PATH_LEN];
	struct fs_statvfs sbuf;
	int rc;

	LOG_PRINTK("Sample program to r/w files on littlefs\n");

	rc = littlefs_mount(mountpoint);
	if (rc < 0) {
		return 0;
	}

	snprintf(fname1, sizeof(fname1), "%s/boot_count", mountpoint->mnt_point);
	snprintf(fname2, sizeof(fname2), "%s/pattern.bin", mountpoint->mnt_point);

	rc = fs_statvfs(mountpoint->mnt_point, &sbuf);
	if (rc < 0) {
		LOG_PRINTK("FAIL: statvfs: %d\n", rc);
		goto out;
	}

	LOG_PRINTK("%s: bsize = %lu ; frsize = %lu ;"
		   " blocks = %lu ; bfree = %lu\n",
		   mountpoint->mnt_point,
		   sbuf.f_bsize, sbuf.f_frsize,
		   sbuf.f_blocks, sbuf.f_bfree);

    char read_buffer1[64], read_buffer2[64];
	const char *data1 = "Hello, LittleFS!";
    rc = perform_write_and_read("/lfs1/myfile.txt", data1, sizeof(data1), read_buffer1, sizeof(read_buffer1));
    if (rc < 0) {
        LOG_PRINTK("Error performing write/read for file 1\n");
        goto out;
    }

    // Perform write and read for the second file
    const char *data2 = "MY name is Farhan";
    rc = perform_write_and_read("/lfs1/name.txt", data2, sizeof(data2), read_buffer2, sizeof(read_buffer2));
    if (rc < 0) {
        LOG_PRINTK("Error performing write/read for file 2\n");
        goto out;
    }
out:
	rc = fs_unmount(mountpoint);
	LOG_PRINTK("%s unmount: %d\n", mountpoint->mnt_point, rc);
	return 0;
}
