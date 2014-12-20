#include "memcachedstore.h"

int main()
{
  MemcachedStore store(false, "./cluster_settings", NULL, NULL);
  return 0;
}
