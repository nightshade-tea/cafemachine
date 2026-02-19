#include "mon.h"
#include "irq.h"
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <stdio.h>

mem_region_list surveillance_list;
EVP_MD_CTX *mdctx;

static unsigned char *hash_buffer(char *buf, hwaddr len)
{
    unsigned int hash_len;
    unsigned char *hash = calloc(EVP_MAX_MD_SIZE, sizeof *hash);
    if (!hash) {
        latte_log("failed to malloc buffer for hash\n");
        return NULL;
    }

    // init message digest
    if (EVP_DigestInit_ex(mdctx, LATTE_HASH_ALGO(), NULL) != 1) {
        latte_log("message digest initialization failed.\n");
        goto error;
    }

    // hash
    if (EVP_DigestUpdate(mdctx, buf, len) != 1) {
        latte_log("failed to hash buffer.\n");
        goto error;
    }

    // retrieve hash
    if (EVP_DigestFinal(mdctx, hash, &hash_len) != 1) {
        latte_log("failed to retrieve hash.\n");
        goto error;
    }

    /* for (unsigned int i = 0; i < hash_len; i++) {
      qemu_log("%02x", hash[i]);
    }
    qemu_log("\n"); */

    return hash;
error:
    free(hash);
    return NULL;
}

static struct mem_region *create_memory_region(hwaddr start_addr,
                                               hwaddr end_addr)
{
    struct mem_region *mr = calloc(1, sizeof *mr);
    if (!mr) {
        latte_log("failed to allocate mem_region: %s\n", strerror(errno));
        return NULL;
    }

    mr->start_addr = start_addr;
    mr->end_addr = end_addr;

    mr->status = LATTE_MR_STATUS_UNKNOWN;

    return mr;
}

int put_under_surveillance(hwaddr start_addr, hwaddr end_addr)
{
    errno = 0;

    struct mem_region *mr = create_memory_region(start_addr, end_addr);
    if (!mr) {
        latte_log("failed to create mem_region\n");
        return errno;
    }

    LIST_INSERT_HEAD(&surveillance_list, mr, regions);

    return 0;
}

/*
static void mock_addr() {
  // put_under_surveillance(0x8000, 0x18000);
  put_under_surveillance(0x1000, 0x2000);
}
*/

static void surveil(LatteState *dev, struct mem_region *mr)
{
    unsigned char *hash;
    char *buf;
    hwaddr len = mr->end_addr - mr->start_addr;
    hwaddr buf_len = len < LATTE_MIN_DUMP_SIZE ? LATTE_MIN_DUMP_SIZE : len;

    latte_log("surveilling [%#" HWADDR_PRIx " -  %#" HWADDR_PRIx "]\n",
              mr->start_addr, mr->end_addr);

    if (!(buf = malloc(buf_len))) {
        latte_log("failed to malloc buffer for dump");
        return;
    }

    latte_dump_mem_relaxed(dev, buf, mr->start_addr, mr->start_addr + buf_len,
                           1);

    if (!(hash = hash_buffer(buf, len))) {
        latte_log("unable to hash buffer\n");
        goto free_buf;
    }

    switch (mr->status) {
    case LATTE_MR_STATUS_UNKNOWN:
        memcpy(mr->hash, hash, EVP_MAX_MD_SIZE);
        mr->status = LATTE_MR_STATUS_NORMAL;
        break;
    case LATTE_MR_STATUS_NORMAL:
        if (memcmp(mr->hash, hash, EVP_MAX_MD_SIZE)) {
            mr->status = LATTE_MR_STATUS_CORRUPTED;
        }
        break;
    default:
        break;
    }

    free(hash);
free_buf:
    free(buf);

    return;
}

void *mon_thread(void *pci_dev)
{
    LatteState *dev = LATTE_DEVICE(pci_dev);
    LIST_INIT(&surveillance_list); // race condition maybe

    mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        latte_log("failed to get new message digest for hashing\n");
        return NULL;
    }

    latte_log("surveillance thread started\n");

    for (;;) {
        struct mem_region *mr = NULL;

        sleep(LATTE_SURVEILLANCE_RATE); // maracutaia

        if (LIST_EMPTY(&surveillance_list)) {
            continue;
        }

        latte_log("surveillance round starting...\n");

        LIST_FOREACH (mr, &surveillance_list, regions) {
            surveil(dev, mr);
            if (mr->status == LATTE_MR_STATUS_CORRUPTED) {
                // TODO raise nmi
                latte_nmi_raise(dev, 0);
                latte_nmi_raise(dev, 0); // first is dropped
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] corrupted!!!\n",
                          mr->start_addr, mr->end_addr);
            } else {
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] intact !!!\n",
                          mr->start_addr, mr->end_addr);
            }
        }

        latte_log("surveillance round finished. %ds until next round\n",
                  LATTE_SURVEILLANCE_RATE);
    }

    return NULL;
}
