#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void print_tree_prefix(int depth, int *last);

void tree(char *path, int depth, int *last, char *file_ext, int show_size) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

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
        print_tree_prefix(depth, last);
        printf("%s/\n", path);

        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("tree: path too long\n");
            close(fd);
            return;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

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

            char *saved_p = p; 

            memmove(p, de.name, strlen(de.name));
            p[strlen(de.name)] = 0;

            last[depth] = (i == entry_count - 1);

            tree(buf, depth + 1, last, file_ext, show_size);

            p = saved_p; 
            i++;
        }
        
    } else {
		print_tree_prefix(depth, last);

        // If the show_size flag is enabled, print the file size
        if (show_size) {
            printf("%s (size: %d bytes)\n", path, st.size);
        } else {
            printf("%s\n", path);
        }
	}

	close(fd);
}


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


int main(int argc, char *argv[]) {
    char *start_dir = "."; 
    char *file_ext = 0;
    int show_size = 0;
    
    for (int i = 1; i < argc; i++) {
    	if (strcmp(argv[i], "-F") == 0) {
    		i++;
    	 	file_ext = argv[i];
    	 	printf("File extension: %s\n", file_ext);	
    	}
    	if (strcmp(argv[i], "-S") == 0) {
    		show_size = 1;
    	} else {
    		start_dir = argv[i];
    	}
    }
	
    int last[128]; 
    memset(last, 0, sizeof(last));  

    tree(start_dir, 0, last, file_ext, show_size);

    exit(0);
}
