#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void print_tree_prefix(int depth, int *last);


/**
 * finds the last occurrence of a char in a string
 *
 * @param s - string to be searched in
 * @param c - character to find
 * @return - pointer to the last occurrence of the char in the string
 */
char* strrchr(const char* s, int c) {
    const char* last = 0;
    for (; *s; s++) {
        if (*s == (char)c) {
            last = s;
        }
    }
    return (char*)last;
}


/**
 * checks if a dir contains a valid file matching the ext
 *
 * @param path - path of the dir to check
 * @param file_ext - file ext to look for
 * @return - 1 if a valid file is found and 0 otherwise
 */
int contains_valid_file(char *path, char *file_ext) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        return 0; 
    }

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;

        memmove(p, de.name, strlen(de.name));
        p[strlen(de.name)] = 0;

        if (stat(buf, &st) < 0) {
            continue;
        }

        // Check if it's a valid file with the correct extension
        if (st.type != T_DIR) {
            if (file_ext) {
                char *dot = strrchr(buf, '.');
                if (dot && strcmp(dot, file_ext) == 0) {
                    close(fd);
                    return 1;  // Found a matching file
                }
            } else {
                close(fd);
                return 1;  // No extension filter, so any file is valid
            }
        } else {
            // Recursively check the subdirectories
            if (contains_valid_file(buf, file_ext)) {
                close(fd);
                return 1;
            }
        }
    }

    close(fd);
    return 0;  // No valid files found
}


/**
 * recursively traverses and prints the directory structure
 *
 * @param path - curr directory path
 * @param depth - curr depth in the directory tree
 * @param last - array indicating if the current node is the last in its level
 * @param file_ext - file ext to filter files
 * @param show_size - flag indicating whether to display file sizes
 * @param show_count - flag indicating whether to display counts of directories and files
 */
void tree(char *path, int depth, int *last, char *file_ext, int show_size, int show_count, int limit_depth) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if (limit_depth != -1 && depth > limit_depth) {
        return;
    }

    if ((fd = open(path, 0)) < 0) {
        printf("tree: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        printf("tree: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type == T_DIR) {
        // Check if directory contains valid files when -F is applied and count directories and files when -C is applied
        int valid_for_print = (file_ext == 0 || contains_valid_file(path, file_ext));
        
        if (!show_count && valid_for_print) {
            print_tree_prefix(depth, last);
            printf("%s/\n", path);
        }

        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("tree: path too long\n");
            close(fd);
            return;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        int file_count = 0;
        int dir_count = 0;

        // Count directories and files
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;

            if (stat(buf, &st) < 0) {
                continue;
            }

            // Only count files that match the extension if -F is used
            if (st.type == T_DIR) {
                dir_count++;
            } else if (!file_ext || (strrchr(buf, '.') && strcmp(strrchr(buf, '.'), file_ext) == 0)) {
                file_count++;
            }
        }

        // Display count when -C is applied
        if (show_count) {
            print_tree_prefix(depth, last);
            printf("%s/ [%d directories, %d files]\n", path, dir_count, file_count);
        }

        close(fd);
        fd = open(path, 0);

        int i = 0;
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            char *saved_p = p; 
            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;

            last[depth] = (i == dir_count + file_count - 1);

            // Recursive call to tree
            tree(buf, depth + 1, last, file_ext, show_size, show_count, limit_depth);

            p = saved_p;
            i++;
        }
    } else {
        // Handle file-specific logic
        if (file_ext) {
            char *dot = strrchr(path, '.');
            if (!dot || strcmp(dot, file_ext) != 0) {
                close(fd);  // Skip files that don't match the extension
                return;
            }
        }

        if (!show_count) {
            print_tree_prefix(depth, last);
            if (show_size) {
                printf("%s (size: %d bytes)\n", path, st.size);
            } else {
                printf("%s\n", path);
            }
        }
    }

    close(fd);
}
          	

/**
 * prints the prefix for the current tree level based on depth 
 *
 * @param depth - current depth in the directory tree
 * @param last - array indicating if the current node is the last in its level
 */
void print_tree_prefix(int depth, int *last) {
    for (int i = 0; i < depth - 1; i++) {
        if (last[i])
            printf("    "); 
        else
            printf("│   "); 
    }
    if (depth > 0) {
        if (last[depth - 1])
            printf("└── "); 
        else
            printf("├── ");
    }
}


/**
 * main function to parse command-line arguments and initiate the tree traversal
 *
 * @param argc - number of command-line arguments
 * @param argv - array of command-line argument strings
 * @return - 0 on successful tree traversal and printing
 */
int main(int argc, char *argv[]) {
    char *start_dir = "."; 
    char *file_ext = 0;
    int show_size = 0;
    int show_count = 0;
    int limit_depth = -1; 


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-F") == 0 && i + 1 < argc) {
            file_ext = argv[i + 1]; 
            i++;
        } else if (strcmp(argv[i], "-S") == 0) {
            show_size = 1;
        } else if (strcmp(argv[i], "-C") == 0) {  
            show_count = 1;
        } else if (strcmp(argv[i], "-L") == 0) {
            limit_depth = atoi(argv[i+1]);
            i++;
        } else {
            start_dir = argv[i];
        }
    }
    
    int last[128]; 
    memset(last, 0, sizeof(last));  

    tree(start_dir, 0, last, file_ext, show_size, show_count, limit_depth);  

    exit(0);
}
