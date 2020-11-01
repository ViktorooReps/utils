#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <ctype.h>
#include <limits.h>

enum AutoFill
{
    NO_AUTOFILL,
    RANDOMFILL,
    ENUMFILL,
};

enum Errors
{
    CREATE_ERROR = -1,
    WRITE_ERROR = -1,
    GETLINE_EOF = -1,
    SUCCESSFUL_READ = 0,
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
    SET_NO_AUTOFILL,
    SET_RANDOMFILL,
    SET_ENUMFILL,
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
        SET_NO_AUTOFILL,
        SET_RANDOMFILL,
        SET_ENUMFILL,
        SET_BIG_ENDIAN,
        SET_LITTLE_ENDIAN,
        PRINT_HELP,
        PRINT_INFO,
        END_SESSION
    };

    char *simple_flags[] = {
        "-noautofill",
        "-randomfill",
        "-enumfill",
        "-big_endian",
        "-little_endian",
        "-help",
        "-info",
        "-quit"
    };

    char *simple_flags_short[] = {
        "-nf",
        "-rf",
        "-ef",
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

void 
swap_bytes(uint8_t *num1, uint8_t *num2)
{
    uint8_t tmp = *num2;
    *num2 = *num1;
    *num1 = tmp;
}

int 
read_numbers(int file_desc, 
             int cnt, 
             const char *format, 
             int byte_cnt, 
             int output_endian)
{
    uint8_t buf[byte_cnt];
    for (int i = 0; i < cnt; ++i) {
        scanf(format, &buf);

        if (output_endian == BIG_ENDIAN) {
            for (int i = 0; i < byte_cnt / 2; ++i) {
                swap_bytes(&buf[i], &buf[byte_cnt - 1 - i]);
            }
        }

        int result = write(file_desc, buf, sizeof(buf));

        if (result == WRITE_ERROR) {
            return WRITE_ERROR;
        }
    }
    return SUCCESSFUL_READ;
}

int
enum_fill(int file_desc, int cnt, int byte_cnt, int output_endian)
{
    union 
    {
        uint64_t num;
        uint8_t buf[8];
    } conv;
    
    conv.num = 0;
    uint64_t maxnum = UINT64_MAX;
    if (byte_cnt != 8) {
        maxnum = 0x1;
        maxnum <<= byte_cnt * CHAR_BIT;
    }

    conv.num = 0;
    for (int i = 0; i < cnt; ++i) {

        if (output_endian == BIG_ENDIAN) {
            for (int i = 0; i < byte_cnt / 2; ++i) {
                swap_bytes(&conv.buf[i], &conv.buf[byte_cnt - 1 - i]);
            }
        }

        int result = write(file_desc, conv.buf, byte_cnt);

        if (result == WRITE_ERROR) {
            return WRITE_ERROR;
        }

        if (byte_cnt != 8) {
            conv.num = (conv.num + 1) % maxnum;
        } else {
            conv.num = conv.num + 1;
        }
    }
    return SUCCESSFUL_READ;
}

int
rand_fill(int file_desc, int cnt, int byte_cnt, int output_endian)
{
    union 
    {
        uint64_t num;
        uint8_t buf[8];
    } conv;
    
    conv.num = 0;
    uint64_t maxnum = UINT64_MAX;
    if (byte_cnt != 8) {
        maxnum = 0x1;
        maxnum <<= byte_cnt * CHAR_BIT;
    }

    for (int i = 0; i < cnt; ++i) {
        conv.num = rand() * rand() * rand();
        if (output_endian == BIG_ENDIAN) {
            for (int i = 0; i < byte_cnt / 2; ++i) {
                swap_bytes(&conv.buf[i], &conv.buf[byte_cnt - 1 - i]);
            }
        }

        int result = write(file_desc, conv.buf, byte_cnt);

        if (result == WRITE_ERROR) {
            return WRITE_ERROR;
        }
    }
    return SUCCESSFUL_READ;
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
    int autofill = NO_AUTOFILL;

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

            case SET_NO_AUTOFILL: {
                autofill = NO_AUTOFILL;
                printf("Autofill was changed to no autofill\n");
                break;
            }

            case SET_ENUMFILL: {
                autofill = ENUMFILL;
                printf("Autofill was changed to no enumfill\n");
                break;
            }

            case SET_RANDOMFILL: {
                autofill = RANDOMFILL;
                printf("Autofill was changed to no randomfill\n");
                break;
            }

            case PRINT_HELP: {
                printf("OPTIONS:\n"
                        "changing output endian: -little_endian (-le) or -big_endian (-be)\n"
                        "autofill options: -noautofill (-nf), -randomfill (-rf) or -enumfill (-ef)\n"
                        "print current filename, endian etc: -info (-i)\n"
                        "end session: -quit (-q)\n"
                        "write numbers to the file: -[s/u][8/16/32/64] [amount]\n"
                        "\ts/u - signed/unsigned\n"
                        "\t8/16/32/64 - number's length in bits\n"
                        "\texample: -s32 3 will write 3 prompted signed 32 bit numbers\n");
                break;
            }

            case PRINT_INFO: { //TODO
                char *endian_str;
                char *autofill_str;

                if (output_endian == LITTLE_ENDIAN) {
                    endian_str = "little_endian";
                }
                if (output_endian == BIG_ENDIAN) {
                    endian_str = "big_endian";
                }

                if (autofill == NO_AUTOFILL) {
                    autofill_str = "no autofill";
                }
                if (autofill == RANDOMFILL) {
                    autofill_str = "randomfill";
                }
                if (autofill == ENUMFILL) {
                    autofill_str = "enumfill";
                }
                                
                printf("INFO:\n"
                        "full filename: %s\n"
                        "output endian: %s\n"
                        "autofill: %s\n", filename, endian_str, autofill_str);
                break;
            }

            case NO_PARAMS:
            case WRONG_FORMAT: {
                fprintf(stderr, "Wrong flag combination. Try -help or -h to see flag options.\n");
                break;
            }

            case WRITE_U8: {
                int byte_cnt = 1;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d unsigned 8 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%hhu", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                break;
            }

            case WRITE_S8: {
                int byte_cnt = 1;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d signed 8 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%hhd", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U16: {
                int byte_cnt = 2;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d unsigned 16 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%hu", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S16: {
                int byte_cnt = 2;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d signed 16 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%hd", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U32: {
                int byte_cnt = 4;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d unsigned 32 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%u", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S32: {
                int byte_cnt = 4;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d signed 32 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%d", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_U64: {
                int byte_cnt = 8;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d unsigned 64 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%lu", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_S64: {
                int byte_cnt = 8;
                int res = SUCCESSFUL_READ;

                if (autofill == NO_AUTOFILL) {
                    printf("Input %d signed 64 bit numbers\n", amount);
                    res = read_numbers(file_desc, amount, "%ld", byte_cnt, output_endian);
                    getline(&line, &alloc_space, stdin);
                }

                if (autofill == ENUMFILL) {
                    res = enum_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (autofill == RANDOMFILL) {
                    res = rand_fill(file_desc, amount, byte_cnt, output_endian);
                }

                if (res == WRITE_ERROR) {
                    fprintf(stderr, "FILE WRITING ERROR: unable to write to a file %s", filename);
                    return 1;
                }

                getline(&line, &alloc_space, stdin);
                break;
            }

            case WRITE_CH: { // add support for enum and autofill
                printf("Input %d char characters\n", amount);
                if (autofill != NO_AUTOFILL) {
                    printf("Autofill isn't supported yet. Autofill changed to no autofill\n");
                    autofill = NO_AUTOFILL;
                }
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
