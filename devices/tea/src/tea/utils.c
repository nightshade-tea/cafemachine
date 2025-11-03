#include "utils.h"

void mount_filename(const char basename[TEA_MAX_BASENAME_LEN], char filename[])
{
    size_t basename_len;

    basename_len = strnlen(basename, TEA_MAX_BASENAME_LEN);

    /* if unset, use default value */
    if (basename_len == 0) {
        basename = TEA_DEFAULT_DUMP_BASENAME;
        basename_len = sizeof(TEA_DEFAULT_DUMP_BASENAME) - 1;
        tea_log("dump filename not set, defaulting to %s%s\n", basename,
                TEA_DUMP_EXT);
    }

    /* assemble filename */
    memcpy(filename, basename, basename_len);
    memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof(TEA_DUMP_EXT));
}
