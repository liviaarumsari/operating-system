#include "../../include/fat32.h"

#include "../../lib/lib-header/stdmem.h"
#include "../../lib/lib-header/stdtype.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

struct FAT32DriverState fat32_driver_state;

uint32_t cluster_to_lba(uint32_t cluster) {
    return CLUSTER_BLOCK_COUNT * cluster;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name,
                          uint32_t parent_dir_cluster) {
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
    struct ClusterBuffer boot_sector;
    read_blocks(boot_sector.buf, BOOT_SECTOR, 1);
    return memcmp(boot_sector.buf, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32() {
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    struct FAT32FileAllocationTable fat_table = {
        .cluster_map = {CLUSTER_0_VALUE, CLUSTER_1_VALUE, FAT32_FAT_END_OF_FILE,
                        [3 ... CLUSTER_MAP_SIZE - 1] = FAT32_FAT_EMPTY_ENTRY}};

    struct FAT32DirectoryTable root_directory;
    init_directory_table(&root_directory, "root\0\0\0\0", cluster_to_lba(ROOT_CLUSTER_NUMBER));

    fat32_driver_state.fat_table = fat_table;
    write_clusters(&fat_table, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&root_directory, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32() {
    bool is_empty = is_empty_storage();

    if (is_empty) {
        create_fat32();
    } else {
        read_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

int8_t read_directory(struct FAT32DriverRequest request) {
    // Perform linear search to find a directory with request.name
    struct FAT32DirectoryEntry *entry_p = dir_table_linear_search(request.name, request.ext, request.parent_cluster_number);
    struct FAT32DirectoryEntry dir_entry = *entry_p;

    // If there is no directory entry found
    if (entry_p == 0) {
        return 2;
    }

    // If the directory entry is not a directory
    if (dir_entry.attribute != ATTR_SUBDIRECTORY) {
        return 1;
    }

    // Get the cluster number of the directory entry
    uint32_t cluster_number = (dir_entry.cluster_high << 16) | dir_entry.cluster_low;

    if (cluster_number) {
        read_clusters(request.buf, cluster_number, 1);  // Do the reading
        return 0;
    }

    return -1;
}

int8_t read(struct FAT32DriverRequest request) {
    // Perform linear search to find a directory with request.name
    struct FAT32DirectoryEntry *entry_p = dir_table_linear_search(request.name, request.ext, request.parent_cluster_number);
    struct FAT32DirectoryEntry dir_entry = *entry_p;

    // If there is no directory entry found
    if (entry_p == 0) {
        return 3;
    }

    // If there is not enough buffer to hold the file
    if (request.buffer_size < dir_entry.filesize) {
        return 2;
    }

    // If the directory entry is not a file
    if (dir_entry.attribute == ATTR_SUBDIRECTORY) {
        return 1;
    }

    // Get the cluster number of the directory entry
    uint32_t cluster_number = (dir_entry.cluster_high << 16) | dir_entry.cluster_low;

    // Initialize buffer to hold the file
    struct ClusterBuffer* buffer = request.buf;

    if (cluster_number) {
        // Do the reading
        while (cluster_number != FAT32_FAT_END_OF_FILE) {
            read_clusters(buffer, cluster_number, 1);
            buffer++;
            cluster_number = fat32_driver_state.fat_table.cluster_map[cluster_number];
        }
        
        return 0;
    }

    return -1;
}

int8_t write(struct FAT32DriverRequest request) {
    // If the directory entry at parent_cluster_number is not a directory
    if (!is_cluster_directory(request.parent_cluster_number)) {
        return 2;
    }

    // Perform linear search to find a directory/file with request.name (and request.ext)
    struct FAT32DirectoryEntry *entry_p = dir_table_linear_search(request.name, request.ext, request.parent_cluster_number);

    // If the entry already exists
    if (entry_p != 0) {
        return 1;
    }

    // If we want to write a subdirectory
    if (request.buffer_size == 0) {
        uint32_t empty_cluster_number = 3;
        uint32_t current_fat_cluster;

        // Loop until find an empty entry
        while (TRUE) {
            if (empty_cluster_number == CLUSTER_MAP_SIZE) {
                return -1;
            }

            current_fat_cluster = fat32_driver_state.fat_table.cluster_map[empty_cluster_number];

            if (current_fat_cluster == FAT32_FAT_EMPTY_ENTRY) {
                fat32_driver_state.fat_table.cluster_map[empty_cluster_number] =
                    FAT32_FAT_END_OF_FILE;

                init_directory_table(&fat32_driver_state.dir_table_buf, request.name,
                                     request.parent_cluster_number);

                write_clusters(&fat32_driver_state.dir_table_buf, empty_cluster_number, 1);

                // Add entry to parent directory table
                add_entry(request, empty_cluster_number);
                write_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

                return 0;
            }

            empty_cluster_number++;
        }
    }

    // We are writing a file
    uint32_t remaining_size = request.buffer_size;
    uint32_t empty_cluster_number = 3;
    struct ClusterBuffer *buf_p = request.buf;
    uint32_t last_cluster_number = 0;

    while (remaining_size > 0) {
        if (empty_cluster_number == CLUSTER_MAP_SIZE) {
            return -1;
        }

        if (fat32_driver_state.fat_table.cluster_map[empty_cluster_number] ==
            FAT32_FAT_EMPTY_ENTRY) {
            if (last_cluster_number != 0) {
                fat32_driver_state.fat_table.cluster_map[last_cluster_number] =
                    empty_cluster_number;
            } else {
                // Add entry to parent directory table
                add_entry(request, empty_cluster_number);
            }

            while (fat32_driver_state.fat_table.cluster_map[empty_cluster_number] != FAT32_FAT_EMPTY_ENTRY)
                empty_cluster_number++;

            fat32_driver_state.fat_table.cluster_map[empty_cluster_number] = FAT32_FAT_END_OF_FILE;
            write_clusters(buf_p, empty_cluster_number, 1);

            buf_p++;
            remaining_size -= CLUSTER_SIZE;
            last_cluster_number = empty_cluster_number;
        }

        empty_cluster_number++;
    }

    if (empty_cluster_number) {
        write_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
        return 0;
    }

    return -1;
}

int8_t delete(struct FAT32DriverRequest request) {
    if (!is_cluster_directory(request.parent_cluster_number)) {
        return -1;
    }

    struct FAT32DirectoryEntry *entry_p = dir_table_linear_search(request.name, request.ext, request.parent_cluster_number);
    uint32_t cluster_number = (entry_p->cluster_high << 16) | entry_p->cluster_low;

    if (entry_p == 0)
        return 1;

    if (entry_p->attribute == ATTR_SUBDIRECTORY) {
        bool is_dir_empty = 1;
        uint32_t dir_cluster_number = cluster_number;

        while (dir_cluster_number != FAT32_FAT_EMPTY_ENTRY) {
            struct FAT32DirectoryTable dir_table;
            read_clusters(&dir_table, dir_cluster_number, 1);

            for (size_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
                if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY)
                    is_dir_empty = 0;
            
            dir_cluster_number = fat32_driver_state.fat_table.cluster_map[dir_cluster_number];
        }

        if (!is_dir_empty)
            return 2;
    }

    memset(entry_p, 0, sizeof(struct FAT32DirectoryEntry));
    write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

    uint32_t next_cluster_number = cluster_number;
    while (TRUE) {
        cluster_number = next_cluster_number;
        if (fat32_driver_state.fat_table.cluster_map[cluster_number] == FAT32_FAT_END_OF_FILE) {
            fat32_driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;
            break;
        } else {
            next_cluster_number = fat32_driver_state.fat_table.cluster_map[cluster_number];
            fat32_driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;
        }
    }
    write_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    return 0;
}

struct FAT32DirectoryEntry *dir_table_linear_search(char *name, char *ext, uint32_t parent_dir_cluster) {
    while (parent_dir_cluster != FAT32_FAT_EMPTY_ENTRY) {
        read_clusters(&fat32_driver_state.dir_table_buf.table, parent_dir_cluster, 1);

        // Iterate through the directory table
        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            // If we find a directory entry with matching name and ext, return its index
            if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, name, 8) == 0 &&
                memcmp(fat32_driver_state.dir_table_buf.table[i].ext, ext, 3) == 0) {
                return fat32_driver_state.dir_table_buf.table + i;
            }
        }

        parent_dir_cluster = fat32_driver_state.fat_table.cluster_map[parent_dir_cluster];
    } 

    return 0;
}

bool is_cluster_directory(uint32_t cluster_number) {
    // Initialize is_directory
    bool is_directory[CLUSTER_MAP_SIZE] = {
        [0 ... 1] = 0, [2] = 1, [3 ... CLUSTER_MAP_SIZE - 1] = 0};
    
    bool visited[CLUSTER_MAP_SIZE] = {
        [0 ... CLUSTER_MAP_SIZE - 1] = 0};

    // Fill up is_directory
    uint32_t i = 2;
    while (i < CLUSTER_MAP_SIZE) {
        if (is_directory[i] && !visited[i]) {
            visited[i] = 1;
            read_clusters(&fat32_driver_state.dir_table_buf, i, 1);

            uint32_t j = 0;
            struct FAT32DirectoryEntry entry = fat32_driver_state.dir_table_buf.table[j];

            while (entry.user_attribute == UATTR_NOT_EMPTY) {
                uint32_t entry_cluster_number = (entry.cluster_high << 16) | entry.cluster_low;

                if (entry.attribute == ATTR_SUBDIRECTORY) {
                    is_directory[entry_cluster_number] = 1;
                }

                entry = fat32_driver_state.dir_table_buf.table[++j];
            }

            uint32_t next_cluster = fat32_driver_state.fat_table.cluster_map[i];
            if (next_cluster != FAT32_FAT_END_OF_FILE){
                is_directory[next_cluster] = 1;
                if (next_cluster < i)
                    i = next_cluster - 1;
            }
        }

        i++;
    }

    return is_directory[cluster_number];
}

int8_t add_entry(struct FAT32DriverRequest request, uint32_t cluster_number) {
    read_clusters(&fat32_driver_state.dir_table_buf.table, request.parent_cluster_number, 1);

    uint32_t entry_i = 0;

    while (entry_i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry) &&
           fat32_driver_state.dir_table_buf.table[entry_i].user_attribute == UATTR_NOT_EMPTY) {
        entry_i++;
        if (entry_i == CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)) {
            request.parent_cluster_number = extend_dir_table(request.parent_cluster_number);
            entry_i = 0;
            read_clusters(&fat32_driver_state.dir_table_buf.table, request.parent_cluster_number, 1);
        }
    }

    for (size_t i = 0; i < 8; i++) {
        fat32_driver_state.dir_table_buf.table[entry_i].name[i] = request.name[i];
    }

    fat32_driver_state.dir_table_buf.table[entry_i].user_attribute = UATTR_NOT_EMPTY;
    if (request.buffer_size == 0)
        fat32_driver_state.dir_table_buf.table[entry_i].attribute = ATTR_SUBDIRECTORY;
    fat32_driver_state.dir_table_buf.table[entry_i].cluster_high = (cluster_number >> 16) & 0xFFFF;
    fat32_driver_state.dir_table_buf.table[entry_i].cluster_low = cluster_number & 0xFFFF;
    fat32_driver_state.dir_table_buf.table[entry_i].filesize = request.buffer_size;

    write_clusters(&fat32_driver_state.dir_table_buf.table, request.parent_cluster_number, 1);

    return 0;
}

uint32_t extend_dir_table(uint32_t dir_cluster_number) {
    if (fat32_driver_state.fat_table.cluster_map[dir_cluster_number] != FAT32_FAT_END_OF_FILE)
        return fat32_driver_state.fat_table.cluster_map[dir_cluster_number];

    uint32_t empty_cluster_number = 3;

    while (fat32_driver_state.fat_table.cluster_map[empty_cluster_number] !=
           FAT32_FAT_EMPTY_ENTRY) {
        empty_cluster_number++;
    }

    fat32_driver_state.fat_table.cluster_map[dir_cluster_number] = empty_cluster_number;
    fat32_driver_state.fat_table.cluster_map[empty_cluster_number] = FAT32_FAT_END_OF_FILE;

    clear_cluster(empty_cluster_number);

    return empty_cluster_number;
}

void clear_cluster(uint32_t cluster_number) {
    struct ClusterBuffer empty_cluster_buffer = {.buf = {[0 ... CLUSTER_SIZE - 1] = 0}};

    write_clusters(empty_cluster_buffer.buf, cluster_number, 1);
}