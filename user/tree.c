#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

/**
 * prints the prefix for the current tree level based on depth 
 *
 * @param depth - current depth in the directory tree
 * @param last - array indicating if the current node is the last in its level
 */
void 
print_tree_prefix(int depth, int *last) {
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
 * checks if the directory name is a special directory like "." or ".."
 *
 * @param name - name of dir to check
 * @return 1 - if it is a special dir
 */
int 
is_special_dir(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/**
 * opens the specified dir to read
 *
 * @param path - path of the dir to open
 * @return - file descriptor for the opened dir
 */
int 
open_directory(const char *path) {
    int fd = open(path, 0);
    if (fd < 0) {
        fprintf(2, "tree: cannot open %s\n", path);
    }
    return fd;
}

/**
 * finds the last occurrence of a char in a string
 *
 * @param s - string to be searched in
 * @param c - character to find
 * @return - pointer to the last occurrence of the char in the string
 */
char* 
strrchr(const char* s, int c) {
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
int 
contains_valid_file(char *path, char *file_ext) {
    char *buf = malloc(512);
    if (!buf) {
        fprintf(2, "tree: memory allocation failed\n");
        return 0;
    }
    char *p;
    int fd = open_directory(path);
    if (fd < 0) {
        free(buf);
        return 0;
    }
    
    struct dirent de;
    struct stat st;
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0 || is_special_dir(de.name))
            continue;

        memmove(p, de.name, strlen(de.name));
        p[strlen(de.name)] = 0;

        if (stat(buf, &st) < 0) continue;

        if (st.type != T_DIR) {
            if (file_ext) {
                char *dot = strrchr(buf, '.');
                if (dot && strcmp(dot, file_ext) == 0) {
                    close(fd);
                    return 1;
                }
            } else {
                close(fd);
                return 1;
            }
        } else if (contains_valid_file(buf, file_ext)) {
            close(fd);
            return 1;
        }
    }
    close(fd);
    return 0;
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
void 
tree(char *path, int depth, int *last, char *file_ext, int show_size, int show_count, int limit_depth) {
    if (limit_depth != -1 && depth > limit_depth) return;

    int fd = open_directory(path);
    if (fd < 0) return;

    struct dirent de;
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(2, "tree: cannot stat %s\n", path);
        close(fd);
        return;
    }

	
    if (st.type == T_DIR) {
        int valid_for_print = (file_ext == 0 || contains_valid_file(path, file_ext));
        if (!show_count && valid_for_print) {
            print_tree_prefix(depth, last);
            printf("%s/\n", strrchr(path, '/'));
        }

        char *buf = malloc(512);
	    if (!buf) {
	        fprintf(2, "tree: memory allocation failed\n");
	        close(fd);
	        return;
	    }
        char *p;
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        int file_count = 0, dir_count = 0;

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || is_special_dir(de.name)) continue;

            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;

            if (stat(buf, &st) < 0) continue;

            if (st.type == T_DIR) dir_count++;
            else if (!file_ext || (strrchr(buf, '.') && strcmp(strrchr(buf, '.'), file_ext) == 0)) file_count++;
        }

        if (show_count && show_size) {
            print_tree_prefix(depth, last);
            printf("%s/ [%d directories, %d files]\n", path, dir_count, file_count);

            close(fd);
            fd = open_directory(path);
            if (fd < 0) return;

            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || is_special_dir(de.name)) continue;

                memmove(p, de.name, strlen(de.name));
                p[strlen(de.name)] = 0;

                if (stat(buf, &st) < 0) continue;

                if (st.type != T_DIR) {
                    print_tree_prefix(depth + 1, last);
                    printf("└── %s (size: %d bytes)\n", de.name, st.size);
                }
            }
        } else if (show_count) {
            print_tree_prefix(depth, last);
            printf("%s/ [%d directories, %d files]\n", path, dir_count, file_count);
        } else if (show_size) {
            print_tree_prefix(depth, last);
            printf("%s (size: %d bytes)\n", strrchr(path, '/'), st.size);
        }

        close(fd);
        fd = open_directory(path);
        if (fd < 0) return;

        int i = 0;
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || is_special_dir(de.name)) continue;

            char *saved_p = p;
            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;

            last[depth] = (i == dir_count + file_count - 1);
            tree(buf, depth + 1, last, file_ext, show_size, show_count, limit_depth);
            p = saved_p;
            i++;
        }
    } else {
        if (file_ext) {
            char *dot = strrchr(path, '.');
            if (!dot || strcmp(dot, file_ext) != 0) {
                close(fd);
                return;
            }
        }

        if (!show_count) {
            print_tree_prefix(depth, last);
            if (show_size) {
                printf("%s (size: %d bytes)\n", strrchr(path, '/'), st.size);  
            } else {
                printf("%s/\n", strrchr(path, '/') ); 
            }
        }
    }

    close(fd);
}


/**
 * main function to parse command-line arguments and initiate the tree traversal
 *
 * @param argc - number of command-line arguments
 * @param argv - array of command-line argument strings
 * @return - 0 on successful tree traversal and printing
 */
int 
main(int argc, char *argv[]) {
    char *start_dir = "."; 
    char *file_ext = 0;
    int show_size = 0;
    int show_count = 0;
    int limit_depth = -1; 


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-F") == 0) {
            if (i + 1 < argc) {
            	if (argv[i+1][0] == '.') {
            		file_ext = argv[++i];
            	} else {
            		fprintf(2, "tree: invalid value for -F\n");
            		exit(1);
            	}
            } else {
                fprintf(2, "tree: missing argument for -F\n");
                exit(1);
            }
        } else if (strcmp(argv[i], "-S") == 0) {
            show_size = 1;
        } else if (strcmp(argv[i], "-C") == 0) {
            show_count = 1;
        } else if (strcmp(argv[i], "-L") == 0) {
            if (i + 1 < argc) {
                limit_depth = atoi(argv[++i]);
            } else {
                fprintf(2, "tree: missing argument for -L\n");
                exit(1);
            }
        } else if (argv[i][0] == '-') {
        	fprintf(2, "tree: invalid flag %s\n", argv[i]);
        	exit(1);
        } else {
            start_dir = argv[i];
        }
    }
    
    int last[128]; 
    memset(last, 0, sizeof(last));  
    tree(start_dir, 0, last, file_ext, show_size, show_count, limit_depth);  
    exit(0);
}
