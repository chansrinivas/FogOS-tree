#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void tree(char *path, int depth, int *last);

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
            printf("├── "); // Middle entries.
    }
}

// Tree function to recursively traverse directories
void tree(char *path, int depth, int *last) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // Open the directory or file
    if ((fd = open(path, 0)) < 0) {
        printf("tree: cannot open %s\n", path);
        return;
    }

    // Get the status (file/directory)
    if (fstat(fd, &st) < 0) {
        printf("tree: cannot stat %s\n", path);
        close(fd);
        return;
    }

	// if the path is a directory
    if (st.type == T_DIR) {
        // Print directory name with tree structure
        print_tree_prefix(depth, last);
        printf("%s/\n", path);

        // Prepare buffer to traverse directory entries
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("tree: path too long\n");
            close(fd);
            return;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        printf("buf: %s\n", buf);

        // Count number of entries in the directory to track the last entry
        int entry_count = 0;
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            entry_count++;
        }
        close(fd);
        fd = open(path, 0);

        // Traverse directory entries and recursively call tree
        int i = 0;


        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

	           memmove(p, de.name, strlen(de.name));
	           p[strlen(de.name)] = 0;

	           // Mark the current entry as the last one in this directory
	           last[depth] = (i == entry_count - 1);

	           // Recursively call tree for this entry
	           tree(buf, depth + 1, last);
	           i++;
	       }
	    } else {

	    // Print file name with tree structure
		 print_tree_prefix(depth, last);
		 printf("%s\n", path);
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

    // Call the tree function with the starting directory and depth 0
    tree(start_dir, 0, last);

    exit(0);
}
