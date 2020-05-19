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
HB_EXTERN hb_blob_t *
hb_blob_create_from_file(const char *file_name);
