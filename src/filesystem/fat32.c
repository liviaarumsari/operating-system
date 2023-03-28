#include "../../include/fat32.h"
#include "../../lib/lib-header/stdtype.h"
#include "../../lib/lib-header/stdmem.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

struct FAT32DriverState driver_state;

uint32_t cluster_to_lba(uint32_t cluster) {
    return CLUSTER_BLOCK_COUNT * cluster;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    for (size_t i = 0; i < 8; i++) {
        dir_table->table[0].name[i] = name[i];
    }
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].cluster_high = (parent_dir_cluster >> 16) & 0xFFFF;
    dir_table->table[0].cluster_low = parent_dir_cluster & 0xFFFF;
    dir_table->table[0].filesize = 0;
}

bool is_empty_storage() {
    return !memcmp(BOOT_SECTOR, fs_signature, BLOCK_SIZE);
}

void create_fat32() {
    memcpy(BOOT_SECTOR, fs_signature, BLOCK_SIZE);
    
    struct FAT32FileAllocationTable fat_table = {
        .cluster_map = {
            CLUSTER_0_VALUE,
            CLUSTER_1_VALUE,
            FAT32_FAT_END_OF_FILE,
            [3 ... CLUSTER_MAP_SIZE - 1] = FAT32_FAT_EMPTY_ENTRY
        }
    };

    struct FAT32DirectoryTable root_directory;
    init_directory_table(&root_directory, "root", FAT_CLUSTER_NUMBER);

    write_clusters(&fat_table, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&root_directory, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32() {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

