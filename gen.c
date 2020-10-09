#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <ctype.h>
#include <limits.h>

enum Errors
{
    CREATE_ERROR = -1,
    WRITE_ERROR = -1,
    GETLINE_EOF = -1,
};

enum ParseResult
{
    SET_BIG_ENDIAN,
    SET_LITTLE_ENDIAN,
    PRINT_HELP,
    PRINT_INFO,
    END_SESSION,
    WRITE_U8,
    WRITE_S8,
    WRITE_U16,
    WRITE_S16,
    WRITE_U32,
    WRITE_S32,
    WRITE_U64,
    WRITE_S64,
    WRITE_CH,
    WRONG_FORMAT,
    NO_PARAMS,
};

enum ByteMasks
{
    BYTEMASK_1 = 0xff,
    BYTEMASK_2 = 0xff00,
    BYTEMASK_3 = 0xff0000,
    BYTEMASK_4 = 0xff000000,
    BYTEMASK_5 = 0xff00000000,
    BYTEMASK_6 = 0xff0000000000,
    BYTEMASK_7 = 0xff000000000000,
    BYTEMASK_8 = 0xff00000000000000,
};

int
parse_params(const char *line, int *amount)
{
    if (!line) {
        return NO_PARAMS;
    }

    int simple_res[] = {
        SET_BIG_ENDIAN,
        SET_LITTLE_ENDIAN,
        PRINT_HELP,
        PRINT_INFO,
        END_SESSION
    };

    char *simple_flags[] = {
        "-big_endian",
        "-little_endian",
        "-help",
        "-info",
        "-quit"
    };

    char *simple_flags_short[] = {
        "-be",
        "-le",
        "-h",
        "-i",
        "-q"
    };

    char param[strlen(line)];
    sscanf(line, "%s", param);

    for (int i = 0; i < sizeof(simple_flags) / sizeof(*simple_flags); ++i) {
        if (!strcmp(param, simple_flags[i]) 
                || !strcmp(param, simple_flags_short[i])) {
            return simple_res[i];
        }
    }

    int duo_res[] = {
        WRITE_U8,
        WRITE_S8,
        WRITE_U16,
        WRITE_S16,
        WRITE_U32,
        WRITE_S32,
        WRITE_U64,
        WRITE_S64,
        WRITE_CH
    };

    char *duo_flags[] = {
        "-u8",
        "-s8",
        "-u16",
        "-s16",
        "-u32",
        "-s32",
        "-u64",
        "-s64",
        "-ch"
    };

    sscanf(line, "%s%d", param, amount);
    for (int i = 0; i < sizeof(duo_flags) / sizeof(*duo_flags); ++i) {
        if (!strcmp(param, duo_flags[i])) {
            return duo_res[i];
        }
    }

    return WRONG_FORMAT;
}

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

    size_t alloc_space = 0;
    char *line = NULL;
    ssize_t gl_res = getline(&line, &alloc_space, stdin);

    int amount = 0;
    int output_endian = LITTLE_ENDIAN;

    enum ParseResult res;
    while ((res = parse_params(line, &amount)) != END_SESSION) {
        if (gl_res == GETLINE_EOF) {
            sleep(1);
            continue;
        }

        switch (res) {
            case SET_BIG_ENDIAN: {
                output_endian = BIG_ENDIAN;
                printf("Output endian was changed to big endian\n");
                break;
            }

            case SET_LITTLE_ENDIAN: {
                output_endian = LITTLE_ENDIAN;
                printf("Output endian was changed to little endian\n");
                break;
            }

            case PRINT_HELP: {
                printf("OPTIONS:\n"
                        "changing output endian: -little_endian (-le) or -big_endian (-be)\n"
                        "print current filename, endian etc: -info (-i)\n"
                        "end session: -quit (-q)\n"
                        "write numbers to the file: -[s/u][8/16/32/64] [amount]\n"
                        "\ts/u - signed/unsigned\n"
                        "\t8/16/32/64 - number's length in bits\n"
                        "\texample: -s32 3 will write 3 prompted signed 32 bit numbers\n");
                break;
            }

            case PRINT_INFO: { //TODO
                printf("INFO:\n"
                        "full filename: %s\n"
                        "output endian: %s\n", filename,
                                             ((output_endian == LITTLE_ENDIAN)?
                                                "little endian" : "big endian"));
                break;
            }

            case NO_PARAMS:
            case WRONG_FORMAT: {
                fprintf(stderr, "Wrong flag combination. Try -help or -h to see flag options.\n");
                break;
            }

            case WRITE_U8: {
                printf("Input %d unsigned 8 bit numbers\n", amount);
                uint8_t num;
                for (int i = 0; i < amount; ++i) {
                    scanf("%hhu", &num);

                    int result = write(file_desc, &num, sizeof(num));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S8: {
                printf("Input %d signed 8 bit numbers\n", amount);
                int8_t num;
                for (int i = 0; i < amount; ++i) {
                    scanf("%hhd", &num);
                    
                    int result = write(file_desc, &num, sizeof(num));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U16: {
                printf("Input %d unsigned 16 bit numbers\n", amount);
                uint16_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%hu", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                    } else {
                        buf[1] = num & BYTEMASK_1;
                        buf[0] = (num & BYTEMASK_2) >> CHAR_BIT;
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S16: {
                printf("Input %d signed 16 bit numbers\n", amount);
                int16_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%hd", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                    } else {
                        buf[1] = num & BYTEMASK_1;
                        buf[0] = (num & BYTEMASK_2) >> CHAR_BIT;
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U32: {
                printf("Input %d unsigned 32 bit numbers\n", amount);
                uint32_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%u", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[2] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[3] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                    } else {
                        buf[3] = num & BYTEMASK_1;
                        buf[2] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[1] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[0] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S32: {
                printf("Input %d signed 32 bit numbers\n", amount);
                int32_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%d", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[2] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[3] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                    } else {
                        buf[3] = num & BYTEMASK_1;
                        buf[2] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[1] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[0] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U64: {
                printf("Input %d unsigned 64 bit numbers\n", amount);
                uint64_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%lu", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[2] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[3] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                        buf[4] = (num & BYTEMASK_5) >> (CHAR_BIT * 4);
                        buf[5] = (num & BYTEMASK_6) >> (CHAR_BIT * 5);
                        buf[6] = (num & BYTEMASK_7) >> (CHAR_BIT * 6);
                        buf[7] = (num & BYTEMASK_8) >> (CHAR_BIT * 7);
                    } else {
                        buf[7] = num & BYTEMASK_1;
                        buf[6] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[5] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[4] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                        buf[3] = (num & BYTEMASK_5) >> (CHAR_BIT * 4);
                        buf[2] = (num & BYTEMASK_6) >> (CHAR_BIT * 5);
                        buf[1] = (num & BYTEMASK_7) >> (CHAR_BIT * 6);
                        buf[0] = (num & BYTEMASK_8) >> (CHAR_BIT * 7);
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S64: {
                printf("Input %d signed 64 bit numbers\n", amount);
                int64_t num;
                char buf[sizeof(num)] = { 0 };
                for (int i = 0; i < amount; ++i) {
                    scanf("%ld", &num);

                    if (output_endian == LITTLE_ENDIAN) {
                        buf[0] = num & BYTEMASK_1;
                        buf[1] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[2] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[3] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                        buf[4] = (num & BYTEMASK_5) >> (CHAR_BIT * 4);
                        buf[5] = (num & BYTEMASK_6) >> (CHAR_BIT * 5);
                        buf[6] = (num & BYTEMASK_7) >> (CHAR_BIT * 6);
                        buf[7] = (num & BYTEMASK_8) >> (CHAR_BIT * 7);
                    } else {
                        buf[7] = num & BYTEMASK_1;
                        buf[6] = (num & BYTEMASK_2) >> CHAR_BIT;
                        buf[5] = (num & BYTEMASK_3) >> (CHAR_BIT * 2);
                        buf[4] = (num & BYTEMASK_4) >> (CHAR_BIT * 3);
                        buf[3] = (num & BYTEMASK_5) >> (CHAR_BIT * 4);
                        buf[2] = (num & BYTEMASK_6) >> (CHAR_BIT * 5);
                        buf[1] = (num & BYTEMASK_7) >> (CHAR_BIT * 6);
                        buf[0] = (num & BYTEMASK_8) >> (CHAR_BIT * 7);
                    }

                    int result = write(file_desc, buf, sizeof(buf));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_CH: {
                printf("Input %d char characters\n", amount);
                for (int i = 0; i < amount; ++i) {
                    char c = getchar();

                    int result = write(file_desc, &c, sizeof(c));

                    if (result == WRITE_ERROR) {
                        fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                        return 1;
                    }
                }
                getline(&line, &alloc_space, stdin);
                break;
            }
        }

        gl_res = getline(&line, &alloc_space, stdin);
    }

    close(file_desc);
    free(line);

    printf("File %s was successfully created.\n"
            "You can now check it's contents with hexdump -C %s\n", filename, filename);
    return 0;
}
