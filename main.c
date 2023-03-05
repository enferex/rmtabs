#include <fcntl.h>
#include <popcntintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static inline size_t get_filesize(int fd) {
  struct stat st;
  int err = fstat(fd, &st);
  return err ? 0 : st.st_size;
}

static ssize_t rmtabs(const char *fname, bool edit) {
  // Open the file.
  if (!fname) return -1;
  const int fd = open(fname, edit ? O_RDWR : O_RDONLY);
  if (fd < 0) return -1;

  // Get the bytesize.
  const size_t sz = get_filesize(fd);
  if (!sz) return -1;

  // mmap the file contents.
  void *addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
  if (!addr) return -1;

  // Scan and count.
  ssize_t count = 0;
  const int stride = 8;
  uint8_t *itr = (uint8_t *)addr;
  const uint8_t *end = ((uint8_t *)addr) + sz;
  while ((itr + stride) <= end) {
    const uint64_t val = *(uint64_t *)itr;
    // If the byte at bit offset _shift match _val.
#define BITS_MATCH(_var, _shift, _val) \
  ((((_var) >> ((_shift))) & 0xFF) == (_val))

    // mask will have '1' when the bytes match and 0 if the bytes do not match.
    //
    // If there is a byte match, the result will be '0' (1 - 1).
    // If there is not a byte match, the result will be '-1' (0 - 1, all ones).
    // The mask is inverted so matching bytes will be all set and non-matches 0.
    const uint64_t mask =
        ~(((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 0, 0x09) - 1)) << 0) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 8, 0x09) - 1)) << 8) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 16, 0x09) - 1)) << 16) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 24, 0x09) - 1)) << 24) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 32, 0x09) - 1)) << 32) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 40, 0x09) - 1)) << 40) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 48, 0x09) - 1)) << 48) |
          ((uint64_t)(0xFF & (uint8_t)(BITS_MATCH(val, 56, 0x09) - 1)) << 56));
    count += (__builtin_popcountll(mask) >> 3);
    // Clear the matches in val and replace with spaces (0x20).
    if (edit) *(uint64_t *)itr = (val & ~mask) | 0x2020202020202020;
    itr += stride;
  }

  // Count the remaining values worse case O(7).
  for (; itr < end; ++itr) {
    if (*(uint8_t *)itr == '\t') {
      ++count;
      if (edit) {
      }
    }
  }
  munmap(addr, sz);
  close(fd);
  return count;
}

int main(int argc, char **argv) {
  bool edit = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp("-i", argv[i]) == 0) {
      edit = true;
      continue;
    }
    const char *fname = argv[i];
    ssize_t n = rmtabs(fname, edit);
    if (n < 0)
      printf("[+] (skipped):\t%s\n", fname);
    else
      printf("[+] %zu tabs:\t%s%s\n", n, fname, (edit ? " (edited)" : ""));
  }
  return 0;
}
