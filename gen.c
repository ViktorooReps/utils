#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <ctype.h>

enum
{
    CREATE_ERROR = -1,
    WRITE_ERROR = -1,
};

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "INPUT ERROR: require filename as first command-line argument\n");
        return 1;
    }

    char *filename = argv[1];
    int file_desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (file_desc == CREATE_ERROR) {
        fprintf(stderr, "FILE CREATION ERROR: unable to create a file %s\n", filename);
        return 1;
    }

    char code = 0;
    while (code != 'q') {
        while (isspace(code = getchar())) {};

        int num;
        if (code != 'q') {
            scanf("%d", &num);
            switch (code) {
                case 'c': {
                    printf("Input %d (with spaces) char's\n", num);
                    getchar();
                    for (int i = 0; i < num; ++i) {
                        char c = getchar();
                        int result = write(file_desc, &c, sizeof(c));
                        if (result == WRITE_ERROR) {
                            fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                            return 1;
                        }
                    }
                    break;
                }
                case 'd': {
                    printf("Input %d int's\n", num);
                    for (int i = 0; i < num; ++i) {
                        int d;
                        scanf("%d", &d);
                        int result = write(file_desc, &d, sizeof(d));
                        if (result == WRITE_ERROR) {
                            fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                            return 1;
                        }
                    }
                    break;
                }
                case 'l': {
                    printf("Input %d long long's\n", num);
                    for (int i = 0; i < num; ++i) {
                        long long lld;
                        scanf("%lld", &lld);
                        int result = write(file_desc, &lld, sizeof(lld));
                        if (result == WRITE_ERROR) {
                            fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                            return 1;
                        }
                    }
                    break;
                }
                
                default:
                    break;
            }
        }
    }

    close(file_desc);
    return 0;
}
