#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void tree(char *path, int depth, int *last, int *dir_count);

// Helper function to print the tree lines
void print_tree_prefix(int depth, int *last) {
    for (int i = 0; i < depth - 1; i++) {
        if (last[i])
            printf("    "); // No vertical bar if this branch is done
        else
            printf("│   "); // Vertical bar to show continuing branches
    }
    if (depth > 0) {
        if (last[depth - 1])
            printf("└── "); // Last entry in this directory
        else
            printf("├── "); // Middle entries
    }
}

// Function to manually concatenate two strings
void my_strcat(char *dest, const char *src) {
    while (*dest) dest++;  // Move to the end of dest
    while ((*dest++ = *src++)); // Copy src to dest
}

// Tree function to recursively traverse directories
void tree(char *path, int depth, int *last, int *dir_count) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // Open the directory or file
    if ((fd = open(path, 0)) < 0) {
        printf("tree: cannot open %s\n", path);
        return;
    }

    printf("Path: %s\n", path);

    // Get the status (file/directory)
    if (fstat(fd, &st) < 0) {
        printf("tree: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type == T_DIR) {
// 
    		(*dir_count)++;
//             // Print directory name with tree structure
//             print_tree_prefix(depth, last);
//             printf("%s/\n", path);
    
            // Prepare buffer to traverse directory entries
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("tree: path too long\n");
                close(fd);
                return;
            }
    
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
    
            // Count number of entries in the directory to track the last entry
            int entry_count = 0;
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                entry_count++;
            }

           close(fd);
           fd = open(path, 0);
           
            int i = 0;
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
    
                // Copy the directory entry name to the buffer
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
    
                char new_path[512];
            strcpy(new_path, path);  // Copy the base path
            new_path[strlen(path)] = '/';  // Add the separator
            new_path[strlen(path) + 1] = '\0';  // Ensure proper termination
            my_strcat(new_path, de.name); // Append the directory/file name
                    
                // Mark this entry as the last in the directory
                last[depth] = (i == entry_count - 1);
    
                // Recursively call tree on the new path
                tree(new_path, depth + 1, last, dir_count);
    
                i++;
            }
       } 
    close(fd);
}


int main(int argc, char *argv[]) {
    char *start_dir = ".";  // Default directory is the current directory
    if (argc > 1) {
        start_dir = argv[1];  // Use provided directory if given
    }

    int last[128];  // Array to keep track of the last entry in each level of the tree
    memset(last, 0, sizeof(last));  // Initialize all to 0

    int dir_count = 0;  // Initialize the directory counter to 0

    // Call the tree function with the starting directory and depth 0
    tree(start_dir, 0, last, &dir_count);

    // Print the total number of directories
    printf("\nTotal directories: %d\n", dir_count);

    exit(0);
}


