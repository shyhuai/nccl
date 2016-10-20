#include "core.h"
#include "bootstrap.h"

#define NBOOTSTRAPS 1

extern struct ncclBootstrap bootstrapSocket;

struct ncclBootstrap bootstraps[NBOOTSTRAPS] = {
  bootstrapSocket
};

ncclResult_t bootstrapGetUniqueId(ncclUniqueId* out) {
  for (int b=0; b<NBOOTSTRAPS; b++) {
    if (bootstraps[b].getUniqueId(out) == ncclSuccess)
      return ncclSuccess;
  }
  return ncclInternalError;
}

ncclResult_t bootstrapInit(ncclUniqueId* id, int rank, int nranks, struct ncclBootstrap** bootstrap, void** commState) {
  for (int b=0; b<NBOOTSTRAPS; b++) {
    ncclResult_t res = bootstraps[b].init(id, rank, nranks, commState);
    if (res != ncclSuccess) {
      *bootstrap = NULL;
      *commState = NULL;
      return res;
    }
    if (*commState != NULL) {
      *bootstrap = bootstraps+b;
      return ncclSuccess;
    }
  }
  WARN("bootstrapInit : no bootstrap found");
  return ncclInternalError;
}

