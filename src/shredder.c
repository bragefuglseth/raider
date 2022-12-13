/* shredder.c
 *
 * Copyright 2022 Alan Beveridge
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <stddef.h>
#include <stdint.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif // _XOPEN_SOURCE

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif // _FILE_OFFSET_BITS

#ifndef _TFNG_STREAM_CIPHER_DEFS
#define _TFNG_STREAM_CIPHER_DEFS

#ifndef _THREEFISH_NOISE_GENERATOR_CIPHER_DEFINITIONS_HEADER
#define _THREEFISH_NOISE_GENERATOR_CIPHER_DEFINITIONS_HEADER

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif // _BSD_SOURCE

#undef MACHINE_16BIT
#undef MACHINE_32BIT
#undef MACHINE_64BIT

#if UINTPTR_MAX == UINT32_MAX
#define MACHINE_32BIT
#elif UINTPTR_MAX == UINT64_MAX
#define MACHINE_64BIT
#elif UINTPTR_MAX == UINT16_MAX
#define MACHINE_16BIT
#endif // UINTPTR_MAX == UINT32_MAX

#if defined(MACHINE_64BIT)
#define TFNG_UNIT_TYPE uint64_t
#define TFNG_NR_BLOCK_BITS 256
#define TFNG_NR_KEY_BITS 512
#else
#define TFNG_UNIT_TYPE uint32_t
#define TFNG_NR_BLOCK_BITS 128
#define TFNG_NR_KEY_BITS 256
#endif // defined(MACHINE_64BIT)

#define TFNG_NR_BLOCK_UNITS 4
#define TFNG_NR_KEY_UNITS 8
#define TFNG_BYTE_TYPE uint8_t
#define TFNG_SIZE_UNIT (sizeof(TFNG_UNIT_TYPE))
#define TFNG_BLOCK_SIZE (TFNG_SIZE_UNIT * TFNG_NR_BLOCK_UNITS)
#define TFNG_KEY_SIZE (TFNG_SIZE_UNIT * TFNG_NR_KEY_UNITS)
#define TFNG_TO_BITS(x) ((x)*8)
#define TFNG_FROM_BITS(x) ((x) / 8)
#define TFNG_MAX_BITS TFNG_NR_BLOCK_BITS
#define TFNG_UNIT_BITS (TFNG_SIZE_UNIT * 8)

void tfng_encrypt_rawblk(TFNG_UNIT_TYPE *O, const TFNG_UNIT_TYPE *I, const TFNG_UNIT_TYPE *K);

#endif // _THREEFISH_NOISE_GENERATOR_CIPHER_DEFINITIONS_HEADER

#define ROL(x, s, max) ((x << s) | (x >> (-s & (max - 1))))
#define ROR(x, s, max) ((x >> s) | (x << (-s & (max - 1))))

#define KE_MIX(x, y, k1, k2, sl)        \
    do                                  \
    {                                   \
        x += k1;                        \
        y += x;                         \
        y += k2;                        \
        x = ROL(x, sl, TFNG_UNIT_BITS); \
        x ^= y;                         \
    } while (0)

#define BE_MIX(x, y, sl)                \
    do                                  \
    {                                   \
        x += y;                         \
        y = ROL(y, sl, TFNG_UNIT_BITS); \
        y ^= x;                         \
    } while (0)

#define KD_MIX(x, y, k1, k2, sr)        \
    do                                  \
    {                                   \
        x ^= y;                         \
        x = ROR(x, sr, TFNG_UNIT_BITS); \
        y -= x;                         \
        y -= k2;                        \
        x -= k1;                        \
    } while (0)

#define BD_MIX(x, y, sr)                \
    do                                  \
    {                                   \
        y ^= x;                         \
        y = ROR(y, sr, TFNG_UNIT_BITS); \
        x -= y;                         \
    } while (0)

enum tfng_rotations
{
    TFS_KS01 = 7,
    TFS_KS02 = 25,
    TFS_KS03 = 19,
    TFS_KS04 = 7,
    TFS_BS01 = 5,
    TFS_BS02 = 27,
    TFS_BS03 = 26,
    TFS_BS04 = 6,
    TFS_BS05 = 14,
    TFS_BS06 = 11,
    TFS_BS07 = 24,
    TFS_BS08 = 18,
    TFS_BS09 = 9,
    TFS_BS10 = 24,
    TFS_BS11 = 6,
    TFS_BS12 = 7,
};

#define PROCESS_BLOCKP(x, k1, k2, k3, k4, k5, k6) \
    do                                            \
    {                                             \
        KE_MIX(Y, X, k1 + k2, k3, TFS_KS01);      \
        KE_MIX(T, Z, k4 + x, k5 + k6, TFS_KS02);  \
                                                  \
        BE_MIX(X, T, TFS_BS01);                   \
        BE_MIX(Z, Y, TFS_BS02);                   \
        BE_MIX(X, Y, TFS_BS03);                   \
        BE_MIX(Z, T, TFS_BS04);                   \
        BE_MIX(X, T, TFS_BS05);                   \
        BE_MIX(Z, Y, TFS_BS06);                   \
    } while (0)

#define PROCESS_BLOCKN(x, k1, k2, k3, k4, k5, k6) \
    do                                            \
    {                                             \
        KE_MIX(Y, X, k1 + k2, k3, TFS_KS03);      \
        KE_MIX(T, Z, k4 + x, k5 + k6, TFS_KS04);  \
                                                  \
        BE_MIX(X, T, TFS_BS07);                   \
        BE_MIX(Z, Y, TFS_BS08);                   \
        BE_MIX(X, Y, TFS_BS09);                   \
        BE_MIX(Z, T, TFS_BS10);                   \
        BE_MIX(X, T, TFS_BS11);                   \
        BE_MIX(Z, Y, TFS_BS12);                   \
    } while (0)

void tfng_encrypt_rawblk(TFNG_UNIT_TYPE *O, const TFNG_UNIT_TYPE *I, const TFNG_UNIT_TYPE *K)
{
    TFNG_UNIT_TYPE X, Y, Z, T;
    TFNG_UNIT_TYPE K0, K1, K2, K3;
    TFNG_UNIT_TYPE K4, T0, T1, T2;

    X = I[0];
    Y = I[1];
    Z = I[2];
    T = I[3];

    K0 = K[0];
    K1 = K[1];
    K2 = K[2];
    K3 = K[3];
    K4 = K[4];
    T0 = K[5];
    T1 = K[6];
    T2 = K[7];

    PROCESS_BLOCKP(1, K1, T0, K0, K3, K2, T1);
    PROCESS_BLOCKN(2, K2, T1, K1, K4, K3, T2);
    PROCESS_BLOCKP(3, K3, T2, K2, K0, K4, T0);
    PROCESS_BLOCKN(4, K4, T0, K3, K1, K0, T1);

    PROCESS_BLOCKP(5, K0, T1, K4, K2, K1, T2);
    PROCESS_BLOCKN(6, K1, T2, K0, K3, K2, T0);
    PROCESS_BLOCKP(7, K2, T0, K1, K4, K3, T1);
    PROCESS_BLOCKN(8, K3, T1, K2, K0, K4, T2);

    PROCESS_BLOCKP(9, K4, T2, K3, K1, K0, T0);
    PROCESS_BLOCKN(10, K0, T0, K4, K2, K1, T1);
    PROCESS_BLOCKP(11, K1, T1, K0, K3, K2, T2);
    PROCESS_BLOCKN(12, K2, T2, K1, K4, K3, T0);

    O[0] = X + K3;
    O[1] = Y + K4 + T0;
    O[2] = Z + K0 + T1;
    O[3] = T + K1 + 18;
}

struct tfnge_stream
{
    TFNG_UNIT_TYPE key[TFNG_NR_KEY_UNITS];
    TFNG_UNIT_TYPE iv[TFNG_NR_BLOCK_UNITS];
    TFNG_BYTE_TYPE carry_block[TFNG_BLOCK_SIZE];
    size_t carry_bytes;
};

void tfnge_init_iv(struct tfnge_stream *tfe, const void *key, const void *iv)
{
    memset(tfe, 0, sizeof(struct tfnge_stream));
    memcpy(tfe->key, key, TFNG_KEY_SIZE);
    if (iv)
        memcpy(tfe->iv, iv, TFNG_BLOCK_SIZE);
    tfe->carry_bytes = 0;
}

void tfnge_init(struct tfnge_stream *tfe, const void *key)
{
    tfnge_init_iv(tfe, key, NULL);
}

void tfnge_emit(void *dst, size_t szdst, struct tfnge_stream *tfe)
{
    TFNG_BYTE_TYPE *udst = dst;
    size_t sz = szdst;

    if (!dst && szdst == 0)
    {
        memset(tfe, 0, sizeof(struct tfnge_stream));
        return;
    }

    if (tfe->carry_bytes > 0)
    {
        if (tfe->carry_bytes > szdst)
        {
            memcpy(udst, tfe->carry_block, szdst);
            memmove(tfe->carry_block, tfe->carry_block + szdst, tfe->carry_bytes - szdst);
            tfe->carry_bytes -= szdst;
            return;
        }

        memcpy(udst, tfe->carry_block, tfe->carry_bytes);
        udst += tfe->carry_bytes;
        sz -= tfe->carry_bytes;
        tfe->carry_bytes = 0;
    }

    if (sz >= TFNG_BLOCK_SIZE)
    {
        do
        {
            tfng_encrypt_rawblk(tfe->iv, tfe->iv, tfe->key);
            memcpy(udst, tfe->iv, TFNG_BLOCK_SIZE);
            udst += TFNG_BLOCK_SIZE;
        } while ((sz -= TFNG_BLOCK_SIZE) >= TFNG_BLOCK_SIZE);
    }

    if (sz)
    {
        tfng_encrypt_rawblk(tfe->iv, tfe->iv, tfe->key);
        memcpy(udst, tfe->iv, sz);
        udst = (TFNG_BYTE_TYPE *)tfe->iv;
        tfe->carry_bytes = TFNG_BLOCK_SIZE - sz;
        memcpy(tfe->carry_block, udst + sz, tfe->carry_bytes);
    }
}

#endif // _TFNG_STREAM_CIPHER_DEFS

#define NOSIZE ((size_t)-1)

static char *data_filename = "/dev/urandom";

// What is this??
static char old_filename[PATH_MAX * 2];

// What is this used for??
static struct tfnge_stream tfnge;

#define XRET(x)            \
    if (!xret && xret < x) \
    xret = x

int remove_file(char *filename, int sync);


int shred_file(char *filename, char *data_filename, int force, int iterations, int do_delete_file, int noround, int hide_shredding, int howmanybytes, int sync, int blocksize, int alwaysrand)
{
    struct stat st;
    char *buf, overwrite_data = 0;
    int file_to_shred, random_data_source_file;
    int xret = 0, do_regular_pattern = 0, last = 0, special = 0, iteration_counter = 0;
    size_t block_size = 0, x, y;
    size_t byte_length, ll, howmany = NOSIZE;

    if (data_filename != NULL)
    {
        random_data_source_file = open(data_filename, O_RDONLY | O_LARGEFILE);
        if (random_data_source_file == -1)
        {
            printf("Could not find source file!\n");
            exit(1);
        }
    }

    /* If sync is true, then convert the boolean to the system value. */
    if (sync == 1)
    {
        sync = O_SYNC;
    }

    special = do_regular_pattern = 0;
    iteration_counter = iterations;

    printf("Destroying %s...\n", filename);

    // Retrive information about the file to shred.
    if (stat(filename, &st) == -1)
    {
        perror(filename);
        XRET(1);
        exit(-1);
    }

    // Get its block size.
    block_size = (size_t)st.st_blksize;

    // Set the number of bytes to shred.
    if (howmanybytes != -1)
    {
        byte_length = ll = howmanybytes;
        noround = 1;
    }
    else
    {
        byte_length = ll = st.st_size;
    }

    if (byte_length == 0 && !S_ISREG(st.st_mode))
        special = 1;

    // Zero out the file info struct. Needed so no info about the file is known.
    memset(&st, 0, sizeof(struct stat));

    // Try to get write access to the file. May be impossible if file is only for the root user.
    if (force)
    {
        if (chmod(filename, S_IRUSR | S_IWUSR) == -1)
        {
            perror(filename);
            XRET(1);
        }
    }

    // Open the file finally.
    file_to_shred = open(filename, O_WRONLY | O_LARGEFILE | O_NOCTTY | sync);

    if (file_to_shred == -1)
    {
        XRET(1);
        perror(filename);
        exit(-1);
    }

    /* Get the block size for the file. */
    buf = malloc(block_size);
    if (!buf)
    {
        perror("malloc");
        XRET(2);
        fprintf(stderr, "Continuing with fixed buffer (%zu bytes long)\n", sizeof(old_filename));
        buf = old_filename;
        block_size = sizeof(old_filename);
    }
    memset(buf, 0, block_size);

    if (read(random_data_source_file, buf, block_size) <= 0)
        fprintf(stderr, "%s: read 0 bytes (wanted %zu)\n", data_filename, block_size);

    /* Some weird cryptography stuff. */
    tfnge_init(&tfnge, buf);

    /* Shredding iteration loop. */
    while (iteration_counter)
    {
        lseek(file_to_shred, 0L, SEEK_SET);

        if (iteration_counter <= 1 && hide_shredding)
        {
            do_regular_pattern = 1;
            overwrite_data = 0;
        }
        else if (!alwaysrand)
        {
            if (read(random_data_source_file, &overwrite_data, 1) <= 0)
                fprintf(stderr, "%s: read 0 bytes (wanted 1)\n", data_filename);
            do_regular_pattern = overwrite_data % 2;
            if (read(random_data_source_file, &overwrite_data, 1) <= 0)
                fprintf(stderr, "%s: read 0 bytes (wanted 1)\n", data_filename);
        }
        else
            do_regular_pattern = 0;

        if (do_regular_pattern)
            fprintf(stderr, "iteration %d (%02hhx%02hhx%02hhx) ...\n", iterations - iteration_counter + 1, overwrite_data, overwrite_data, overwrite_data);
        else
            fprintf(stderr, "iteration %d (random) ...\n", iterations - iteration_counter + 1);

        while (1)
        {
            if (!do_regular_pattern)
                tfnge_emit(buf, block_size, &tfnge);
            else
                memset(buf, overwrite_data, block_size);

            /* If this the remaining file length to shred is less than or equal the block size, then it is time to stop. */
            if (byte_length <= block_size && !special)
                last = 1;
            errno = 0;

            /* Shredding code line. */
            byte_length = byte_length - write(file_to_shred, buf, (noround && last) ? byte_length : block_size);
            if (errno)
            {
                perror(filename);
                errno = 0;
                break;
            }
            if (last)
            {
                last = 0;
                break;
            }
        }

        byte_length = ll;
        fdatasync(file_to_shred);
        iteration_counter--;
    }

    /* Deallocation code. d*/
    if (do_delete_file)
    {
        int result = remove_file(filename, sync);
        printf("RESULT: %d\n", result);
    }

    tfnge_emit(NULL, 0, &tfnge);

    if (buf && buf != old_filename)
        free(buf);
    if (file_to_shred != -1)
        close(file_to_shred);

    close(random_data_source_file);

    return xret;
}

int remove_file(char *filename, int sync)
{
    int x, y;
    char* new_filename, *d;

    int file_to_shred = open(filename, O_WRONLY | O_TRUNC | O_LARGEFILE | O_NOCTTY | sync);
    
    printf("removing %s ...\n", filename);

    /* The most obfustcated code in the world. */
    x = strnlen(filename, sizeof(old_filename) / 2);
    new_filename = old_filename + (sizeof(old_filename) / 2);
    memcpy(old_filename, filename, x);
    *(old_filename + x) = 0;
    d = strrchr(old_filename, '/');
    if (d)
    {
        d++;
        y = d - old_filename;
        memset(d, '0', x - (d - old_filename));
    }
    else
    {
        y = 0;
        memset(old_filename, '0', x);
    }

    memcpy(new_filename, old_filename, x);
    *(new_filename + x) = 0;

    /* Check the next filename for access. */
    if (access(new_filename, R_OK) != -1)
    {
        printf("Deleting %s by unlink because %s already exists!\n", filename, new_filename);
        unlink(filename);
        return -1;
    }

    printf("%s -> %s\n", filename, new_filename);

    if (rename(filename, new_filename) == -1)
    {
        perror(new_filename);
        return -1;
    }

    /* And what is is this?? */
    while (x > y + 1)
    {
        *(old_filename + x) = 0;
        x--;
        *(new_filename + x) = 0;

        if (access(new_filename, R_OK) != -1)
        {
            printf("%s already exists!\n", new_filename);
            unlink(old_filename);
            return -1;
        }

        printf("%s -> %s\n", old_filename, new_filename);
        if (rename(old_filename, new_filename) == -1)
        {
            perror(new_filename);
            return -1;
        }
    }

    unlink(new_filename);
    printf("done away with %s.\n", filename);

    return 0;
}