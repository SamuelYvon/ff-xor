#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>

#define DEFAULT_RESULT_BUFF_SZ 32

typedef struct {
    size_t offset;
    uint8_t byte_key;
} search_result;

typedef struct {
    uint8_t **keys;
    size_t no_of_keys;
} search_string_set;

/**
 * Destroy the search string table
 *
 * @param search_string
 */
void search_string_destroy(search_string_set *search_string) {
    if (!search_string) {
        return;
    }

    if (!search_string->no_of_keys || !search_string->keys) {
        return;
    }

    for (size_t i = 0; i < search_string->no_of_keys; ++i) {
        if (!search_string->keys[i]) {
            continue;
        }
        free(search_string->keys[i]);
    }

    free(search_string->keys);
}

search_result *search_with_set(FILE *fd, char *search_string, search_string_set *set, size_t *no_results) {
    size_t nb_results = *no_results = 0;
    size_t result_arr_max_l = DEFAULT_RESULT_BUFF_SZ;
    search_result *results = malloc(sizeof(search_result) * result_arr_max_l);

    const size_t search_string_len = strlen(search_string);

    uint8_t *window = malloc(search_string_len * sizeof(uint8_t));

    rewind(fd);

    if (fseek(fd, 0, SEEK_END)) {
        printf("Failed to jump to end of file: error;\n");
        goto sws_clean;
    }

    size_t file_len = ftell(fd);
    rewind(fd);

    // Fill the window once
    if (search_string_len != fread(window, sizeof(uint8_t), search_string_len, fd)) {
        printf("Failed to fill the first initial window buffer. Error.\n");
        goto sws_clean;
    }
    rewind(fd);

    // Go in position so the next character fits in the buffer
    fseek(fd, (long) search_string_len, SEEK_SET);

    int c;
    for (size_t i = 0; i < file_len - search_string_len; i++) {

        for (size_t key_idx = 0; key_idx < set->no_of_keys; ++key_idx) {
            uint8_t *coded_string = set->keys[key_idx];

            // Match found
            if (0 == memcmp(coded_string, window, search_string_len)) {

                // Extend the array: running out of space
                if (nb_results >= result_arr_max_l) {
                    result_arr_max_l <<= 1;
                    search_result *results_tmp = malloc(sizeof(search_result) * result_arr_max_l);
                    if (!results_tmp) {
                        printf("Running out of memory, cannot proceed to find all the results.\n");
                        goto sws_clean;
                    }
                    memcpy(results_tmp, results, sizeof(search_result) * nb_results);
                    free(results);
                    results = results_tmp;
                }

                results[nb_results].offset = i;
                results[nb_results].byte_key = key_idx;
                nb_results++;
            }
        }

        // Move the window for one character (roll left)
        for (size_t j = 0; j < search_string_len - 1; j++) {
            window[j] = window[j + 1];
        }

        if (EOF == (c = fgetc(fd))) {
            goto sws_clean;
        }

        window[search_string_len - 1] = (uint8_t) c;
    }

    sws_clean:
    *no_results = nb_results;
    free(window);

    return results;
}

/**
 * Generate all variants of the search strings
 *
 * @param search_string  the original search string
 * @param bytes_per_xor  the number of bytes per XOR key
 * @return
 */
search_string_set *generate_xor_search_strings(char *search_string, size_t bytes_per_xor) {
    if (bytes_per_xor != 1) {
        printf("ERR: bytes per key is not 1, forcing 1.\n");
    }
    bytes_per_xor = 1;

    size_t no_of_keys = 256;
    size_t search_string_len = strlen(search_string);

    uint8_t **keys = malloc(sizeof(uint8_t *) * no_of_keys);
    for (int i = 0; i <= 255; ++i) {
        uint8_t key = (uint8_t) i;
        keys[i] = (uint8_t *) malloc(search_string_len);
        memcpy(keys[i], (uint8_t *) search_string, search_string_len);
        for (size_t j = 0; j < search_string_len; ++j) {
            keys[i][j] = keys[i][j] ^ key;
        }
    }

    search_string_set *search_set = malloc(sizeof(search_string_set));

    search_set->keys = keys;
    search_set->no_of_keys = no_of_keys;

    return search_set;
}

/**
 *
 * @param fd
 * @param results
 * @param no_results
 * @param after
 */
void search_result_print(FILE *fd, search_result *results, size_t no_results, size_t after) {
    printf("Found %lld result(s).\n", (long long) no_results);
    for (size_t i = 0; i < no_results; ++i) {
        long offset = (long) results[i].offset;
        uint8_t key = results[i].byte_key;

        fseek(fd, offset, SEEK_SET);

        printf("=============================\n");
        printf("Result %lld:\nAddr: %ld\nXOR: %d\n", (long long) i + 1, offset, key);
        printf("Search: ");

        for (size_t j = 0; j < after; ++j) {
            int d = fgetc(fd);
            if (EOF == d) {
                printf("ERR?");
                exit(1);
            }
            printf("%c", ((uint8_t) d) ^ key);
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    int c;
    size_t after = 10;
    int keyspace = 1;
    char *file_path = NULL;
    char *search_string = NULL;

    while (-1 != (c = getopt(argc, argv, "k:A:"))) {
        switch (c) {
            case 'k': {
                long long int keyspace_tmp = strtoll(optarg, NULL, 10);
                if (keyspace < 1 || keyspace > INT_MAX) {
                    printf("Invalid keyspace");
                }
                if (1 != keyspace) {
                    printf("[WARN] No support for keys larger than 1 byte right now.\n");
                }
                keyspace = (int) keyspace_tmp;
                break;
            }
            case 'A': {
                long long int after_tmp = strtoll(optarg, NULL, 10);
                if (after_tmp <= 0 || after_tmp > LONG_MAX) {
                    printf("Invalid after number");
                    abort();
                }
                after = (size_t) after_tmp;
                break;
            }
            default: {
                exit(1);
            }
        }
    }

    if (optind >= argc) {
        printf("No file path specified. Specify a file path and a search flag.\n");
        exit(1);
    }
    file_path = argv[optind];

    optind++;
    if (optind >= argc) {
        printf("No search string specified. Specify a search string.\n");
        exit(1);
    }
    search_string = argv[optind];

    FILE *fd;
    if (NULL == (fd = fopen(file_path, "rb"))) {
        printf("Failed to open the file %s for reading.", file_path);
        exit(1);
    } else {
        printf("Parsing file %s...\n", file_path);
    }

    size_t no_results;

    search_string_set *keyset = generate_xor_search_strings(search_string, keyspace);
    search_result *results = search_with_set(fd, search_string, keyset, &no_results);
    search_string_destroy(keyset);


    if (!results) {
        printf("ERR: results are inaccessible.");
        exit(1);
    }

    search_result_print(fd, results, no_results, after);
    free(results);
    fclose(fd);

    return 0;
}
