#include <stdio.h>
#include <stdlib.h>

#include <harfbuzz/hb.h>

/**
 * hb_blob_create_from_file:
 *
 * This is a "backport" of just `hb_blob_create_from_file` to allow `emojivur`
 * to build on Ubuntu 18.04 where the version of HarfBuzz packaged is just v1.7.2.
 *
 * @file_name: font filename.
 *
 * Returns: A hb_blob_t pointer with the content of the file
 *
 * Since: 1.7.7
 **/
hb_blob_t *
hb_blob_create_from_file(const char *file_name)
{
    /* The following tries to read a file without knowing its size beforehand
     It's used as a fallback for systems without mmap or to read from pipes */
    unsigned long len = 0;
    unsigned long allocated = BUFSIZ * 16;
    char *data = (char *)malloc(allocated);
    if (unlikely(!data))
        return hb_blob_get_empty();

    FILE *fp = fopen(file_name, "rb");
    if (unlikely(!fp))
        goto fread_fail_without_close;

    while (!feof(fp))
    {
        if (allocated - len < BUFSIZ)
        {
            allocated *= 2;
            /* Don't allocate and go more than ~536MB, our mmap reader still
	           can cover files like that but lets limit our fallback reader */
            if (unlikely(allocated > (2 << 28)))
                goto fread_fail;
            char *new_data = (char *)realloc(data, allocated);
            if (unlikely(!new_data))
                goto fread_fail;
            data = new_data;
        }

        unsigned long addition = fread(data + len, 1, allocated - len, fp);

        int err = ferror(fp);
#ifdef EINTR // armcc doesn't have it
        if (unlikely(err == EINTR))
            continue;
#endif
        if (unlikely(err))
            goto fread_fail;

        len += addition;
    }
    fclose(fp);

    return hb_blob_create(data, len, HB_MEMORY_MODE_WRITABLE, data,
                          (hb_destroy_func_t)free);

fread_fail:
    fclose(fp);
fread_fail_without_close:
    free(data);
    return hb_blob_get_empty();
}
