/*
This file is part of MarFS, which is released under the BSD license.


Copyright (c) 2015, Los Alamos National Security (LANS), LLC
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-----
NOTE:
-----
MarFS uses libaws4c for Amazon S3 object communication. The original version
is at https://aws.amazon.com/code/Amazon-S3/2601 and under the LGPL license.
LANS, LLC added functionality to the original work. The original work plus
LANS, LLC contributions is found at https://github.com/jti-lanl/aws4c.

GNU licenses can be found at <http://www.gnu.org/licenses/>.


From Los Alamos National Security, LLC:
LA-CC-15-039

Copyright (c) 2015, Los Alamos National Security, LLC All rights reserved.
Copyright 2015. Los Alamos National Security, LLC. This software was produced
under U.S. Government contract DE-AC52-06NA25396 for Los Alamos National
Laboratory (LANL), which is operated by Los Alamos National Security, LLC for
the U.S. Department of Energy. The U.S. Government has rights to use,
reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR LOS
ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is
modified to produce derivative works, such modified software should be
clearly marked, so as not to confuse it with the version available from
LANL.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/




#include "logging.h"
#include "dal.h"
#include "common.h"

#include <stdlib.h>             // malloc()
#include <errno.h>
#include <stdarg.h>             // va_args for open()
#include <sys/types.h>          // the next three are for open & mode_t
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#include <limits.h>             // INT_MAX


// ===========================================================================
// DEFAULT
// ===========================================================================
//

int   default_dal_config(struct DAL*     dal,
                         xDALConfigOpt** opts,
                         size_t          opt_count) {
  dal->global_state = opts;
  return 0;
}



// Use these if you don't need to do anything special to initialize the
// context before-opening / after-closing for file-oriented or
// directory-oriented operations.
//
// open_data() gives us a ptr to the ObjectStream in the FileHandle.  We
// need this, for now, because MarFS expects stream-ops to have
// side-effects on that OS.


// #define OS(CTX)           ((ObjectStream*)((CTX)->data.ptr))
#define FH(CTX)           ((MarFS_FileHandle*)((CTX)->data.ptr))
#define OS(CTX)           (&FH(CTX)->os)

int     default_dal_ctx_init(DAL_Context* ctx, DAL* dal, void* fh) {
   ctx->flags    = 0;
   ctx->data.ptr = (MarFS_FileHandle*)fh;
   return 0;
}
int     default_dal_ctx_destroy(DAL_Context* ctx, DAL* dal) {
   return 0;
}




// ================================================================
// OBJ
//
// This is just a pass-through to the interfaces in object_stream.h, which
// were the original back-end for MarFS to interact with an object-store.
// ================================================================



#if 0
// NOTE: We need an ObjectStream, to use the functions in object_stream.h
//     There is already a statically-allocated one in the file-handle.  If
//     we just re-used that one (e.g. by having open_data() just assign
//     DAL_Context.data.ptr to point to it), it would save us a malloc/free
//     for every stream.  For now, to avoid refactoring, I'm going to
//     dynamically allocate our own ObjectStream.  We could also just
//     statically define on in DAL_Context.

typedef struct {
   ObjectStream os;
} OBJ_DALContextState;


#define STATE_LVAL(CTX)   (CTX)->data.ptr
#define STATE(CTX)        ((OBJ_DALContextState*)(STATE_LVAL(CTX)))
#define OS(CTX)           &(STATE(ctx)->os)





int     obj_dal_ctx_init(DAL_Context* ctx, DAL* dal) {
   ctx->flags   = 0;

   OBJ_DALContextState* state = (OBJ_DALContextState*)malloc(sizeof(OBJ_DALContextState));
   if (! state) {
      LOG(LOG_ERR, "malloc failed\n");
      errno = ENOMEM;
      return -1;
   }
   memset(state, 0, sizeof(OBJ_DALContextState));

   STATE_LVAL(ctx) = state;
   return 0;
}


int     obj_dal_ctx_destroy(DAL_Context* ctx, DAL* dal) {
   if (STATE(ctx)) {
      free(STATE(ctx));
      STATE_LVAL(ctx) = 0;
   }
   return 0;
}

#endif







int     obj_init(DAL_Context* ctx, DAL* dal, void* fh) {
   default_dal_ctx_init(ctx, dal, fh);
   return stream_init(OS(ctx), NULL, FH(ctx));
}



int     obj_open(DAL_Context* ctx,
                 int          is_put,
                 size_t       chunk_offset,
                 size_t       content_length,
                 uint8_t      preserve_write_count,
                 uint16_t     timeout) {

   return stream_open(OS(ctx), is_put,
                      chunk_offset, content_length,
                      preserve_write_count, timeout);
}

int     obj_put(DAL_Context*  ctx,
                const char*   buf,
                size_t        size) {

   return stream_put(OS(ctx), buf, size);
}

ssize_t obj_get(DAL_Context*  ctx,
                char*         buf,
                size_t        size) {

   return stream_get(OS(ctx), buf, size);
}

int     obj_sync(DAL_Context*  ctx) {

   return stream_sync(OS(ctx));
}

int     obj_abort(DAL_Context*  ctx) {

   return stream_abort(OS(ctx));
}

int     obj_close(DAL_Context*  ctx) {

   return stream_close(OS(ctx));
}

int     obj_delete(DAL_Context*  ctx) {

   // return stream_del(OS(ctx));
   return stream_del_components(OS(ctx),
                                FH(ctx)->info.pre.host,
                                FH(ctx)->info.pre.bucket,
                                FH(ctx)->info.pre.objid);
}

int     obj_update_object_location(DAL_Context* ctx) {

   return update_url(OS(ctx), &FH(ctx)->info);
}


static DAL obj_dal = {
   .name                   = "OBJECT",
   .name_len               = 6, // strlen("OBJECT"),

   .global_state           = NULL,
   .config                 = &default_dal_config,
   
#if 0
   .init                   = &obj_dal_ctx_init,
   .destroy                = &obj_dal_ctx_destroy,
#elif 0
   .init                   = &default_dal_ctx_init,
   .destroy                = &default_dal_ctx_destroy,
#else
   .init                   = &obj_init,
   .destroy                = &default_dal_ctx_destroy,
#endif

   .open                   = &obj_open,
   .put                    = &obj_put,
   .get                    = &obj_get,
   .sync                   = &obj_sync,
   .abort                  = &obj_abort,
   .close                  = &obj_close,
   .del                    = &obj_delete,
   .update_object_location = &obj_update_object_location
};


// ================================================================
// NO_OP
//
// Like it says, these perform no data action.  The point would be to allow
// benchmarking the cost of meta-data operations alone.
// ================================================================



// #define OS(CTX)         (ObjectStream*)(CTX)->data.ptr





int     nop_open(DAL_Context* ctx,
                 int          is_put,
                 size_t       chunk_offset,
                 size_t       content_length,
                 uint8_t      preserve_write_count,
                 uint16_t     timeout) {

   return 0;
}

int     nop_put(DAL_Context*  ctx,
                const char*   buf,
                size_t        size) {

   OS(ctx)->written += size;
   return size;
}

ssize_t nop_get(DAL_Context*  ctx,
                char*         buf,
                size_t        size) {

   OS(ctx)->written += size;
   return size;
}


int     nop_sync(DAL_Context*  ctx) { return 0; }

int     nop_abort(DAL_Context*  ctx) { return 0; }

int     nop_close(DAL_Context*  ctx) { return 0; }

int     nop_delete(DAL_Context*  ctx) { return 0; }

int     nop_update_object_location(DAL_Context* ctx) { return 0; }


DAL nop_dal = {
   .name                   = "NO_OP",
   .name_len               = 5, // strlen("NO_OP"),

   .global_state           = NULL,

   .config                 = &default_dal_config,
   .init                   = &default_dal_ctx_init,
   .destroy                = &default_dal_ctx_destroy,
   .open                   = &nop_open,
   .put                    = &nop_put,
   .get                    = &nop_get,
   .sync                   = &nop_sync,
   .abort                  = &nop_abort,
   .close                  = &nop_close,
   .del                    = &nop_delete,
   .update_object_location = &nop_update_object_location
};

// ===========================================================================
// POSIX
// ===========================================================================

#define MAX_POSIX_PREFIX_LEN MARFS_MAX_REPO_NAME

#define MAX_OBJECT_PATH_LEN (MAX_POSIX_PREFIX_LEN               \
                             + MARFS_MAX_REPO_NAME + 1          \
                             + MARFS_MAX_NAMESPACE_NAME + 1     \
                             + MARFS_MAX_OBJID_SIZE + 1)

typedef struct posix_dal_ctx {
   MarFS_FileHandle* fh;
   int fd;
   char file_path[MAX_OBJECT_PATH_LEN];
} PosixDal_Context;

enum posix_dal_flags {
   POSIX_DAL_PATH_GENERATED = (1 << 0)
};

#define POSIX_DAL_CONTEXT(CTX) ((PosixDal_Context*)((CTX)->data.ptr))
#define POSIX_DAL_FH(CTX)      POSIX_DAL_CONTEXT(CTX)->fh
#define POSIX_DAL_FD(CTX)      POSIX_DAL_CONTEXT(CTX)->fd
#define POSIX_DAL_OS(CTX)      (&(POSIX_DAL_CONTEXT(CTX)->fh->os))
#define POSIX_DAL_PATH(CTX)    POSIX_DAL_CONTEXT(CTX)->file_path
#define FLAT_OBJID_SEPARATOR '#'

int posix_dal_ctx_init(DAL_Context* ctx, struct DAL* dal, void* fh /* ? */) {
   ENTRY();
   ctx->data.ptr = malloc(sizeof(PosixDal_Context));
   POSIX_DAL_CONTEXT(ctx)->fd = -1;
   POSIX_DAL_CONTEXT(ctx)->fh = (MarFS_FileHandle*)fh;
   ctx->flags = 0;

   // create repo/namespace directory under the posix-repo from config.
   const MarFS_Repo *repo = POSIX_DAL_FH(ctx)->info.pre.repo;
   const MarFS_Namespace *ns = POSIX_DAL_FH(ctx)->info.pre.ns;
   struct stat st;

   char repo_path[MARFS_MAX_REPO_NAME + MARFS_MAX_HOST_SIZE
                  + MARFS_MAX_NAMESPACE_NAME];
   sprintf(repo_path, "%s/%s", repo->host, repo->name);

   // Check that the directory
   // <repo-host>/<repo-name>/<namespace-name> exists
   if(stat(repo_path, &st) == -1) {
      TRY0( mkdir(repo_path, 0755) ); // XXX: an arbitrary mode.
   }
   sprintf(repo_path, "%s/%s/%s", repo->host, repo->name, ns->name);
   if(stat(repo_path, &st) == -1) {
      TRY0( mkdir(repo_path, 0755) );
   }

   memset(POSIX_DAL_PATH(ctx), '\0', MAX_OBJECT_PATH_LEN);
   
   EXIT();
   return 0;
}

int posix_dal_ctx_destroy(DAL_Context* ctx, struct DAL* dal) {
   free(POSIX_DAL_CONTEXT(ctx));
   ctx->flags = 0;
   return 0;
}

// file-ify an object-id.
static void flatten_objid(char* objid) {
   int i;
   for(i = 0; objid[i]; i++) {
      if(objid[i] == '/')
         objid[i] = FLAT_OBJID_SEPARATOR;
   }
}

// Generate the full path the the object in the POSIX repository.
// This will be used as the ->update_object_location interface
// function.
int generate_path(DAL_Context* ctx) {
   ENTRY();
   const MarFS_Repo*      repo        = POSIX_DAL_FH(ctx)->info.pre.repo;
   const MarFS_Namespace* ns          = POSIX_DAL_FH(ctx)->info.pre.ns;
   char*                  object_path = POSIX_DAL_PATH(ctx);

   sprintf(object_path, "%s/%s/%s/", repo->host, repo->name, ns->name);
   LOG(LOG_INFO, "POSIX_DAL Repo top level dir: %s\n", object_path);

   char* object_id_start = object_path + strlen(object_path);
   strncat(object_path, POSIX_DAL_FH(ctx)->info.pre.objid,
           MARFS_MAX_OBJID_SIZE);

   flatten_objid(object_id_start);

   LOG(LOG_INFO, "generated path: %s\n", object_path);
   ctx->flags |= POSIX_DAL_PATH_GENERATED;

   EXIT();
   return 0;
}

int posix_dal_open(DAL_Context* ctx,
                   int          is_put,
                   size_t       chunk_offset,
                   size_t       content_length,
                   uint8_t      preserve_write_count,
                   uint16_t     timeout) {
   ENTRY();

   // fail if the path has not been generated. This should never
   // happen and would be a logical error.
   if(! (ctx->flags & POSIX_DAL_PATH_GENERATED)) {
      LOG(LOG_ERR, "POSIX_DAL: no previous call to "
          "DAL->update_object_location");
      return -1;
   }

   TRY0( stream_cleanup_for_reopen(POSIX_DAL_OS(ctx), preserve_write_count) );

   char*        object_path = POSIX_DAL_PATH(ctx);
   int          object_flags;
   const mode_t mode        = S_IRUSR|S_IWUSR;

   if(is_put) {
      POSIX_DAL_OS(ctx)->flags |= OSF_WRITING;
      object_flags = O_WRONLY|O_CREAT;
   }
   else {
      POSIX_DAL_OS(ctx)->flags |= OSF_READING;
      object_flags = O_RDONLY;
   }

   int fd = open(object_path, object_flags, mode);
   if(fd < 0) {
      LOG(LOG_ERR, "Failed to open file with posix_dal: %s\n", object_path);
      return -1;
   }

   if (chunk_offset) {
      off_t rc = lseek(fd, chunk_offset, SEEK_SET);
      if (rc == (off_t)-1) {
         LOG(LOG_ERR, "lseek(%ld) faild for posix_dal: %s\n",
             chunk_offset, object_path);
         return -1;
      }
   }

   POSIX_DAL_FD(ctx) = fd;
   POSIX_DAL_OS(ctx)->flags |= OSF_OPEN;

   EXIT();
   return 0;
}

int posix_dal_put(DAL_Context* ctx, const char* buf, size_t size) {
   int written = write(POSIX_DAL_FD(ctx), buf, size);

   if(written >= 0)
      POSIX_DAL_OS(ctx)->written += written;

   return written;
}

ssize_t posix_dal_get(DAL_Context* ctx, char* buf, size_t size) {
   ssize_t size_read = 0;

   size_read = read(POSIX_DAL_FD(ctx), buf, size);
   if(size_read == 0) {
      POSIX_DAL_OS(ctx)->flags |= OSF_EOF;
   }
   else if(size_read < 0) {
      return size_read; // relying on errno set by read
   }

   POSIX_DAL_OS(ctx)->written += size_read;

   return size_read;
}

static int close_posix_object(DAL_Context* ctx) {
   TRY_DECLS();

   TRY0( close(POSIX_DAL_FD(ctx)) );
   POSIX_DAL_OS(ctx)->flags &= ~OSF_OPEN;
   POSIX_DAL_OS(ctx)->flags |= OSF_CLOSED;
   // prevent future mistakes by clearing out the file descriptor.
   POSIX_DAL_FD(ctx) = -1;

   return 0;
}

// See comments on stream_sync. When this returns, all I/O (ever) is
// completed. No more I/O errors are possible. There should be no more
// operations done on the stream at this point.
int posix_dal_sync(DAL_Context* ctx) {
   TRY_DECLS();

   // fsync will set errno = EBADF if the fd is not open. In order to
   // be consistent with stream_sync, we detect that case and fail
   // with EINVAL instead.
   if(! (POSIX_DAL_OS(ctx)->flags & OSF_OPEN)) {
      LOG(LOG_ERR, "%s isn't open\n", POSIX_DAL_OS(ctx)->url);
      errno = EINVAL;
      return -1;
   }

   TRY0( fsync(POSIX_DAL_FD(ctx)) );

   // Since close might cause an error we do it here.
   TRY0( close_posix_object(ctx) );

   return 0;
}

// Undo any writes that have been made since the "object" was opened.
//
// NOTE: Based on the use of this function, we are assuming that no
//       data has been written to the object when this is called.
//       In this case, this boils down to a no-op.
int posix_dal_abort(DAL_Context* ctx) {
   if(! (POSIX_DAL_OS(ctx)->flags & OSF_OPEN)) {
      LOG(LOG_ERR, "POSIX_DAL: abort: %s isn't open", POSIX_DAL_OS(ctx)->url);
      errno = EINVAL;
      return -1;
   }

   POSIX_DAL_OS(ctx)->flags |= OSF_ABORT;

   // other than setting flags, this is a no-op.

   return 0;
}

int posix_dal_close(DAL_Context* ctx) {
   TRY_DECLS();

   // the file may have already been closed by _sync
   if(! (POSIX_DAL_OS(ctx)->flags & OSF_OPEN)) {
      return 0;
   }

   TRY0( close_posix_object(ctx) );

   return 0;
}

int posix_dal_delete(DAL_Context* ctx) {
   TRY_DECLS();

   return unlink(POSIX_DAL_PATH(ctx));
}



DAL posix_dal = {
   .name         = "POSIX",
   .name_len     = 5,

   .global_state = NULL,

   .config       = &default_dal_config,
   .init         = &posix_dal_ctx_init,
   .destroy      = &posix_dal_ctx_destroy,

   .open         = &posix_dal_open,
   .put          = &posix_dal_put,
   .get          = &posix_dal_get,
   .sync         = &posix_dal_sync,
   .abort        = &posix_dal_abort,
   .close        = &posix_dal_close,
   .del          = &posix_dal_delete,

   .update_object_location = &generate_path
};


#if USE_MC
// ===========================================================================
// MC (Multi-component)
// ===========================================================================


#define MC_FH(CTX)      MC_CONTEXT(CTX)->fh
#define MC_OS(CTX)      (&MC_FH(CTX)->os)
#define MC_REPO(CTX)    MC_CONTEXT(CTX)->fh->repo
#define MC_HANDLE(CTX)  MC_CONTEXT(CTX)->mc_handle
#define MC_CONFIG(CTX)  MC_CONTEXT(CTX)->config
#define MC_CONTEXT(CTX) ((MC_Context*)((CTX)->data.ptr))

typedef struct mc_context {
   ObjectStream*     os;
   ne_handle         mc_handle;
   MarFS_FileHandle* fh;
   off_t             chunk_offset;

   // These define the path we will use for the open and are
   // updated/set by ->update_object_location()
   char              path_template[MC_MAX_PATH_LEN];
   unsigned int      start_block;
   unsigned int      pod;
   unsigned int      cap;
   MC_Config         *config;
   Udal_Rules*       udal_rules;
} MC_Context;


// fwd-decl
int mc_path_snprintf_sockets(char*       dest,
                             size_t      size,
                             const char* format,
                             uint32_t    block,
                             void*       state);



static int open_degraded_object_log(const char *log_dir_path) {
   char log_path[PATH_MAX];
   char host_name[HOST_NAME_MAX];
   if(gethostname(host_name, HOST_NAME_MAX)) {
      LOG(LOG_ERR, "gethostname() failed.\n");
      return -1;
   }
   pid_t pid = getpid();
   unsigned int pid_scatter = pid % MC_LOG_SCATTER_WIDTH;

   sprintf(log_path, "%s/%s", log_dir_path, host_name);
   int res = mkdir(log_path, 0777);
   // avoid problems with umask
   if(res == 0) {
      chmod(log_path, 0777);
   }
   sprintf(log_path, "%s/%s/%u", log_dir_path, host_name, pid_scatter);
   res = mkdir(log_path, 0777);
   if(res == 0) {
      chmod(log_path, 0777);
   }

   // I'm ignoring the results of mkdir() since the directory might
   // already exist, in which case it would return an error. If
   // something went wrong and the directory doesn't exist and
   // couldn't be created then we will catch it when we try to open
   // the file below.

   sprintf(log_path, "%s/%s/%u/degraded_objects.%u", log_dir_path, host_name,
           pid_scatter, pid);
   int fd = open(log_path, O_CREAT|O_APPEND|O_WRONLY,
                 S_IWUSR|S_IWGRP|S_IWOTH);
   // TBD: fchown the file so it is owned by root? Then we can have
   //      read permissions on for the owner. For now we can get awy
   //      with no read permission since root can read anything and
   //      the rebuild utility will be run by an admin.
   if(fd < 0) {
      LOG(LOG_ERR, "Failed to open degraded object log (%s): %s\n",
          log_path, strerror(errno));
   }
   else {
      // If we successfully opened the file, the try to chmod it,
      // since we might have created it. If the file exists this is
      // wasted effort, but there isn't an easy way to tell whether
      // open created it without also doing a stat().
      chmod(log_path, S_IWUSR|S_IWGRP|S_IWOTH);
   }
      
   return fd;
}


int mc_config(struct DAL*     dal,
              xDALConfigOpt** opts,
              size_t          opt_count) {
   ENTRY();
   int        is_sockets = ((! strcmp(dal->name, "MC_SOCKETS"))
                            && (dal->name_len == strlen("MC_SOCKETS")));

   MC_Config* config     = malloc(sizeof(MC_Config));
   memset(config, 0, sizeof(MC_Config));

   config->degraded_log_fd   = -1;
   config->degraded_log_path = NULL;

   char* mc_user = NULL;


   int i;
   for(i = 0; i < opt_count; i++) {
      if(!strcmp(opts[i]->key, "n")) { // ? Should be strncmp?
         config->n = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"n\" = %d\n", config->n);
      }
      else if(!strcmp(opts[i]->key, "e")) {
         config->e = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"e\" = %d\n", config->e);
      }
      else if(!strcmp(opts[i]->key, "num_pods")) {
         config->num_pods = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"num_pods\" = %d\n",
             config->num_pods);
      }
      else if(!strcmp(opts[i]->key, "num_cap")) {
         config->num_cap = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"num_cap\" = %d\n",
             config->num_cap);
      }
      else if(!strcmp(opts[i]->key, "scatter_width")) {
         config->scatter_width = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"scatter_width\" = %d\n",
             config->scatter_width);
      }
      else if(!strcmp(opts[i]->key, "degraded_log_dir")) {
         config->degraded_log_path = strdup(opts[i]->val.value.str);
         LOG(LOG_INFO, "parsing mc option \"degraded_log_path\" = %s\n",
             config->degraded_log_path);
      }
      else if (! is_sockets) {
         LOG(LOG_ERR, "Unrecognized MC DAL config option: %s\n",
             opts[i]->key);
         free(config);
         return -1;
      }

      // options only used by MC_SOCKETS DAL
      else if(!strcmp(opts[i]->key, "host_offset")) {
         config->host_offset = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"host_offset\" = %d\n",
             config->host_offset);
      }
      else if(!strcmp(opts[i]->key, "host_count")) {
         config->host_count = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"host_count\" = %d\n",
             config->host_count);
      }
      else if(!strcmp(opts[i]->key, "blocks_per_host")) {
         config->blocks_per_host = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"blocks_per_host\" = %d\n",
             config->blocks_per_host);
      }
      else if(!strcmp(opts[i]->key, "block_offset")) {
         config->block_offset = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"block_offset\" = %d\n",
             config->block_offset);
      }
      else if(!strcmp(opts[i]->key, "global_block_numbering")) {
         config->global_block_numbering = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"global_block_numbering\" = %d\n",
             config->global_block_numbering);
      }
      else if(!strcmp(opts[i]->key, "pod_offset")) {
         config->pod_offset = strtol(opts[i]->val.value.str, NULL, 10);
         LOG(LOG_INFO, "parsing mc option \"pod_offset\" = %d\n",
             config->pod_offset);
      }
      else if(!strcmp(opts[i]->key, "mc_user")) {
         mc_user = strdup(opts[i]->val.value.str);
         LOG(LOG_INFO, "parsing mc option \"mc_user\" = %s\n",
             mc_user);
      }
      else {
         LOG(LOG_ERR, "Unrecognized MC DAL (sockets) config option: %s\n",
             opts[i]->key);
         free(config);
         return -1;
      }
   }

   if(config->degraded_log_path == NULL) {
      LOG(LOG_ERR, "no degraded_log_dir specified for DAL '%s'.\n", dal->name);
      return -1;
   }
   else {
      // initialize the lock to prevent concurrent writes to the log.
      SEM_INIT(&config->lock, 0, 1);
   }
#ifdef S3_AUTH
   // To allow generating per-connection auth-signatures, capture the S3
   // credentials up front, while we have access to the AWS config file
   // (i.e. before de-escalation).  If mc_user is null, skt_auth_install()
   // defaults to SKT_S3_USER.

   if (is_sockets) {

      if (! mc_user)
         mc_user = strdup(DEFAULT_SKT_AUTH_USER);  // SKT_S3_USER

      // (see Q&A in comments at skt_auth_init() defn)
      int err = 0;
      if (skt_auth_init(mc_user, &config->auth)) {
         // probably didn't find the user in ~/.awsAuth
         LOG(LOG_ERR, "Authentication-init failed for user '%s' in sockets-DAL '%s'.\n",
             mc_user, dal->name);
         err = 1;
      }
      else if (config->auth == NULL) {
         LOG(LOG_ERR, "libne was built without enabling socket-authentication, "
             " but DAL '%s' requires it.\n",
             dal->name);
         err = 1;
      }

      free(mc_user);
      if (err)
         return -1;
   }
#endif

   config->is_sockets = is_sockets;
   if (is_sockets)
     config->snprintf = mc_path_snprintf_sockets;
   else
     config->snprintf = ne_default_snprintf;

   // we don't keep the options, themselves.
   // we parsed out what we wanted, and installed into the MC_Config
   free_xdal_config_options(opts);


   dal->global_state = config;
   EXIT();
   return 0;
}


#if 0
// Commented out until we add ->deconfig() to the DAL
void mc_deconfig(struct DAL *dal) {
   MC_Config *config = (MC_Config*)dal->global_state;
   WAIT(&config->lock);

#  ifdef S3_AUTH
   if (config->auth)
      skt_auth_free(config->auth);
#  endif

   close(config->degraded_log_fd);
   SEM_DESTROY(&config->lock);
   free(config);
}
#endif


// Computes a good, uniform, hash of the string.
//
// Treats each character in the length n string as a coefficient of a
// degree n polynomial.
//
// f(x) = string[n -1] + string[n - 2] * x + ... + string[0] * x^(n-1)
//
// The hash is computed by evaluating the polynomial for x=33 using
// Horner's rule.
//
// Reference: http://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-14.html
static uint64_t polyhash(const char* string) {
   // According to http://www.cse.yorku.ca/~oz/hash.html
   // 33 is a magical number that inexplicably works the best.
   const int salt = 33;
   char c;
   uint64_t h = *string++;
   while((c = *string++))
      h = salt * h + c;
   return h;
}

// compute the hash function h(x) = (a*x) >> 32
static uint64_t h_a(const uint64_t key, uint64_t a) {
   return ((a * key) >> 32);
}

// Initialize the context for a multi-component backed object.
// (e.g. new file-handle at open-time)
// Returns 0 on success or -1 on failure (if memory cannot be allocated).
int mc_init(DAL_Context* ctx, struct DAL* dal, void* fh) {
   ENTRY();
   
   ctx->data.ptr = malloc(sizeof(MC_Context));
   if(! MC_CONTEXT(ctx)) {
      LOG(LOG_ERR, "failed to allocate memory for MC_Context\n");
      return -1;
   }
   memset(MC_CONTEXT(ctx), 0, sizeof(MC_Context));

   // memset(MC_CONTEXT(ctx)->path_template, '\0', MC_MAX_PATH_LEN);   
   MC_FH(ctx) = (MarFS_FileHandle*)fh;
   // MC_HANDLE(ctx) = NULL;
   // MC_CONTEXT(ctx)->chunk_offset = 0;
   // ctx->flags = 0;
   MC_CONFIG(ctx) = (MC_Config*)dal->global_state;

   EXIT();
   return 0;
}

// Free the multi-component context stored in the dal context.
// `ctx' should not be used any more after this is called.
int mc_destroy(DAL_Context *ctx, struct DAL* dal) {
   free(MC_CONTEXT(ctx));
   return 0;
}


// stage-2 path-printing function for the sockets MC DAL.  libne now
// allows callers to provide custom functions to fill out remaining
// fields in the template path generated by mc_update_path().  In the
// case of the MC_SOCKETS DAL, this allows us to use
// configuration-state to compute the host for a given pod.  In that
// case, after mc_update_path(), the path-template might look like
// this:
//
//    192.168.0.%d:1200/zfs/repo/pod1/block%d/sockets63
//
// for the sockets MC DAL, we must remember the pod we compute in
// mc_update_path() (for this DAL_Context), so we can use it, along
// with some config info, to find the right offset for the host-field.
//
// NOTE: We assume the local file paths within each pod are the same.
//
//       For example:
//         192.168.0.<octet>:/zfs/repo/pod<p>/block<b>/...
//
//       Where:
//         <octet> advances sequentially across all hosts in all pods.
//                 starting at some offset.
//
//         Within each pod:
//             <p> is constant
//             <b> starts at some offset, and increments by 1 up to N+E+offset
//                 [I'm not referring to the way a stripe is laid out along
//                 the block-files but rather to the names of the N+E
//                 block-files.]
//
//         Across pods:
//             <p> starts at some offset, and increments by 1 up to num_pods+offset
//
//       All these "offsets" are configurable in the MC_SOCKETS DAL repo spec,
//       in the marfs config file.
//             
int mc_path_snprintf_sockets(char*       dest,
                             size_t      size,
                             const char* format,
                             uint32_t    block,
                             void*       state) {


  DAL_Context* ctx    = (DAL_Context*)state;
  MC_Config*   config = MC_CONFIG(ctx);
  
  // uint32_t hosts_per_pod = (config->n + config->e) / config->blocks_per_host;
  uint32_t hosts_per_pod = config->host_count / config->num_pods;
  uint32_t pod_offset    = MC_CONTEXT(ctx)->pod * hosts_per_pod;
  uint32_t host_offset   = config->host_offset + (block / config->blocks_per_host);

  uint32_t block_offset  = block + config->block_offset;

  return snprintf(dest, size, format,
                  pod_offset + host_offset,  // "192.168.0.%d"
                  block_offset);             // "block%d"
}

// For MC DALs, the MarFS configuration specifies a sprintf-format
// representing a generic path.  This function fills out some of those
// fields, leaving a template that still has 1 or more "%d" fileds,
// which vary across the block-elements in a stripe.  libne will then
// call a custom function to fill out the remaining fields, to
// generate each per-block filename, as needed.
int mc_update_path(DAL_Context* ctx) {
   // QUESTION: Do we need to prepend the bucket and ns->alias to the
   //           objid? For now We can just flatten the url to make
   //           things easy.

   // shorthand
   PathInfo*         info          = &(MC_FH(ctx)->info);
   const MarFS_Repo* repo          = info->pre.repo;
   char*             objid         = info->pre.objid;
   char*             path_template = MC_CONTEXT(ctx)->path_template;
   char obj_filename[MARFS_MAX_OBJID_SIZE];
   strncpy(obj_filename, objid, MARFS_MAX_OBJID_SIZE);
   flatten_objid(obj_filename);

   // We will use the hash in multiple places, save it to avoid
   // recomputing.
   //
   // Hash the actual object ID so the hash will remain the same,
   // regadless of changes to the "file-ification" format.
   unsigned long objid_hash = polyhash(objid);
   
   char *mc_path_format = repo->host;

   unsigned int num_blocks    = MC_CONFIG(ctx)->n+MC_CONFIG(ctx)->e;
   unsigned int num_pods      = MC_CONFIG(ctx)->num_pods;
   unsigned int num_cap       = MC_CONFIG(ctx)->num_cap;
   unsigned int scatter_width = MC_CONFIG(ctx)->scatter_width;
   unsigned int seed = objid_hash;
   uint64_t a[3];
   int i;
   for(i = 0; i < 3; i++)
      a[i] = rand_r(&seed) * 2 + 1; // generate 32 random bits

   MC_CONTEXT(ctx)->pod         = objid_hash % num_pods;
   MC_CONTEXT(ctx)->cap         = h_a(objid_hash, a[0]) % num_cap;
   unsigned long scatter        = h_a(objid_hash, a[1]) % scatter_width;
   MC_CONTEXT(ctx)->start_block = h_a(objid_hash, a[2]) % num_blocks;

   // fill in path template
   // the mc_path_format is sometheing like:
   //
   //   "<protected-root>/repo10+2/pod%d/block%s/cap%d/scatter%d/"
   //
   if (MC_CONFIG(ctx)->is_sockets) {
     // The config-path for MC_SOCKETS DAL uses "%%d" for the block,
     // instead of "%s" that receives "%d"
     snprintf(path_template, MC_MAX_PATH_LEN, mc_path_format,
              MC_CONTEXT(ctx)->pod + MC_CONFIG(ctx)->pod_offset,
              MC_CONTEXT(ctx)->cap,
              scatter);
   }
   else {
     snprintf(path_template, MC_MAX_PATH_LEN, mc_path_format,
              MC_CONTEXT(ctx)->pod,
              "%d", // this will be filled in by the ec library
              MC_CONTEXT(ctx)->cap,
              scatter);
   }

   // be robust to vairation in the config... We could always just add
   // a slash, but that will get ugly in the logs.
   if(path_template[strlen(path_template) - 1] != '/')
      strcat(path_template, "/");
   
   // append the fileified object id
   strncat(path_template, obj_filename, MARFS_MAX_OBJID_SIZE);
   LOG(LOG_INFO, "MC path template: (starting block: %d) %s\n",
       MC_CONTEXT(ctx)->start_block, path_template);
   
   return 0;
}

int find_stats_from_tflags(int tflags)
{
	int tot_stats = 0;

	if (tflags & TF_OPEN)
	{
		tot_stats++;
	}
	if (tflags & TF_RW)
	{
		tot_stats += 2;
	}
	if (tflags & TF_CLOSE)
	{
		tot_stats++;
	}
	/*if (tflags & TF_RENAME)
	{
		tot_stats++;
	}*/
	/*if (tflags & TF_CRC)
	{
		printf("DAL.C detected CRC flag\n");
		tot_stats++;
	}*/
	/*if (tflags & TF_ERASURE)
	{
		printf("DAL.C detected ERASURE flag\n");
		tot_stats++;
	}*/
	/*if (tflags & TF_XATTR)
	{
		tot_stats++;
	}
	if (tflags & TF_STAT)
	{
		tot_stats++;
	}*/
	return tot_stats;
}

// Open a multi-component object stream backed by a ne_handle.
//
// This used to defer opens, since that is now done in MarFS proper,
// we no longer need to do it here.
int mc_open(DAL_Context* ctx,
            int is_put,
            size_t chunk_offset,
            size_t content_length,
            uint8_t preserve_write_count,
            uint16_t timeout) {
   ENTRY();

   ObjectStream* os            = MC_OS(ctx);
   char*         path_template = MC_CONTEXT(ctx)->path_template;

   unsigned int  n             = MC_CONFIG(ctx)->n;
   unsigned int  e             = MC_CONFIG(ctx)->e;

   int           impl          = (MC_CONFIG(ctx)->is_sockets ? UDAL_SOCKETS : UDAL_POSIX);
   int           timing_flags  = (MC_FH(ctx)->info.pre.repo->timing_flags
                                  | MC_FH(ctx)->info.pre.ns->timing_flags );

   int           mode          = (is_put ? NE_WRONLY : NE_RDONLY);
   // do the generic cleanup stuff like resetting flags.
   TRY0( stream_cleanup_for_reopen(os, preserve_write_count) );
   if (MC_FH(ctx)->repo == NULL)
   {
   	//allocate repo
   	MC_FH(ctx)->repo = (char*)malloc(MARFS_MAX_REPO_SIZE);
   	memset(MC_FH(ctx)->repo, 0, MARFS_MAX_REPO_SIZE);
        memcpy(MC_FH(ctx)->repo, MC_FH(ctx)->info.pre.repo->name, MC_FH(ctx)->info.pre.repo->name_len);
   	//find the number of time stats to collect
   	MC_FH(ctx)->tot_stats = find_stats_from_tflags(timing_flags);
   	//allocate pointers include 2 characters for name of stat
   	//MC_FH(ctx)->timing_stats = (char*)malloc(sizeof(double) * 65 * (MC_FH(ctx)->tot_stats) + 2 * (MC_FH(ctx)->tot_stats));
   	//memset(MC_FH(ctx)->timing_stats, 0, sizeof(double) * 65 * (MC_FH(ctx)->tot_stats) + 2 * (MC_FH(ctx)->tot_stats));
   }
   MC_HANDLE(ctx) = ne_open1(MC_CONFIG(ctx)->snprintf, ctx,
                             impl, MC_CONTEXT(ctx)->udal_rules, MC_CONFIG(ctx)->auth, timing_flags,
                             path_template, mode,
                             MC_CONTEXT(ctx)->start_block, n, e);

   if(! MC_HANDLE(ctx)) {
             LOG(LOG_ERR, "Failed to open MC Handle %s\n", path_template);
                   return -1;
   }
   //we need total_blk from ne_handle to allocate stat buffer
   MC_FH(ctx)->total_blk = MC_HANDLE(ctx)->N + MC_HANDLE(ctx)->E;
   MC_FH(ctx)->pod_id = MC_CONTEXT(ctx)->pod;
   //MC_HANDLE(ctx)->repo = MC_FH(ctx)->repo;
   //MC_HANDLE(ctx)->pod_id = &(MC_FH(ctx)->pod_id);
   if (MC_FH(ctx)->timing_stats == NULL)
   {
	//allocate stat buffer based on total blks from ne_handle
	MC_FH(ctx)->timing_stats_buff_size = sizeof(double) * 65 * (MC_FH(ctx)->tot_stats) * (MC_FH(ctx)->total_blk) + 3 * (MC_FH(ctx)->tot_stats);
	MC_FH(ctx)->timing_stats = (char*)malloc(MC_FH(ctx)->timing_stats_buff_size);
	memset(MC_FH(ctx)->timing_stats, 0, MC_FH(ctx)->timing_stats_buff_size);
	
   }
   MC_HANDLE(ctx)->timing_stats = MC_FH(ctx)->timing_stats;

   if(is_put) {
      os->flags |= OSF_WRITING;
   }
   else {
      os->flags |= OSF_READING;
   }

   os->flags  |= OSF_OPEN;
   MC_CONTEXT(ctx)->chunk_offset = chunk_offset;
   
   EXIT();
   return 0;
}

// Put data into a MC Object. For performance reasons we will need to
// try to align the size of `buf' with the underlying buffers in the
// n+e lib.  This could be hard to achieve, since the buffer size may
// be determined by fuse, or PFTool. Is there some efficient way to
// simply give the buffer to the n+e library and relinquish ownership,
// using a fresh buffer for the next write?
int mc_put(DAL_Context* ctx,
           const char* buf,
           size_t size) {
   ENTRY();

   ObjectStream* os     = MC_OS(ctx);
   if(! (os->flags & OSF_OPEN)) {
      LOG(LOG_ERR, "Attempted put on OS that is not open.\n");
      errno = EBADF;
      return -1;
   }

   // ne_write() takes a signed int to specify size.
   // Make sure the DAL-caller's size_t doesn't exceed that size
   if (size > INT_MAX) {
     LOG(LOG_ERR, "DAL size_t arg %llu exceeds max signed-int size accepted by ne_write()\n",
         size);
     errno = EIO;
     return -1;
   }

   ne_handle handle = MC_HANDLE(ctx);
   printf("mc_put calling ne_write\n");
   int written = ne_write(handle, buf, size);

   if(written < 0) {
      LOG(LOG_ERR, "ftone_write() failed.\n");
      os->flags |= OSF_ERRORS;
      return -1;
   }

   os->written += written;
   
   EXIT();
   return written;
}

ssize_t mc_get(DAL_Context* ctx, char* buf, size_t size) {
   ENTRY();

   ssize_t       size_read;
   ObjectStream* os     = MC_OS(ctx);

   if(! (os->flags & OSF_OPEN)) {
      errno = EBADF;
      LOG(LOG_ERR, "Attempted get on OS that is not open.\n");
   }

   ne_handle handle = MC_HANDLE(ctx);

   // context->chunk_offset tracks the offset we are reading at within
   // the object.
   size_read = ne_read(handle, buf, size, MC_CONTEXT(ctx)->chunk_offset);

   if(size_read < 0) {
      LOG(LOG_ERR, "ne_read() failed.\n");
      os->flags |= OSF_ERRORS;
      return -1;
   }
   else if(size_read == 0) { // EOF
      os->flags |= OSF_EOF;
   }

   // update the offset for the next call to read.
   MC_CONTEXT(ctx)->chunk_offset += size_read;
   os->written += size_read;

   EXIT();
   return size_read;
}

// Upon return no more I/O is possible. The stream is closed.
//
// If ne_close indicates a recoverable error, then the object is
// logged to the "degraded object log".
int mc_sync(DAL_Context* ctx) {
   ENTRY();

   ObjectStream* os         = MC_OS(ctx);
   ne_handle     handle     = MC_HANDLE(ctx);
   MC_Config*    config     = MC_CONFIG(ctx);
   MC_Context*   mc_context = MC_CONTEXT(ctx);
   
   if(! (os->flags & OSF_OPEN)) {
      LOG(LOG_ERR, "%s isn't open\n", os->url);
      errno = EINVAL;
      return -1;
   }

   // the result of close for a handle opened for reading is an
   // indicator of whether the data is degraded and, if so, which
   // block is corrupt or missing.
   int error_pattern = ne_close(handle);
   if(error_pattern > 0) {

      // Keeping the log message as well as writing to the degraded
      // object file for debugging purposes.
      LOG(LOG_INFO, "WARNING: Object %s degraded. Error pattern: 0x%x."
          " (N: %d, E: %d, Start: %d).\n",
          mc_context->path_template, error_pattern,
          config->n, config->e, mc_context->start_block);

      // we shouldn't need more than 512 bytes to hold the extra data
      // needed for rebuild
      char buf[MC_MAX_LOG_LEN];
      snprintf(buf, MC_MAX_LOG_LEN,
               MC_DEGRADED_LOG_FORMAT, mc_context->path_template,
               config->n, config->e,
               mc_context->start_block, error_pattern,
               MC_FH(ctx)->info.pre.repo->name,
               mc_context->pod, mc_context->cap);
      
      printf("MC_SYNC buf %s\n", buf);

      // If the degraded log file has not already been opened, open it now.
      WAIT(&config->lock);
      if(config->degraded_log_fd == -1) {

         config->degraded_log_fd =
            open_degraded_object_log(config->degraded_log_path);

         if(config->degraded_log_fd < 0) {
            LOG(LOG_ERR, "failed to open degraded log file\n");
         }
         else {
            // If we successfully opened it, then free the resources
            // used to store the path.
            free(config->degraded_log_path);
            config->degraded_log_path = NULL;
         }
      }

      if(write(config->degraded_log_fd, buf, strlen(buf))
         != strlen(buf)) {

         // theoretically the data is still safe, so we can just log
         // and ignore the failure.
         LOG(LOG_ERR, "Failed to write to degraded object log\n");
      }
      POST(&config->lock);
   }
   else if(error_pattern < 0) {

      // close the stream, a failed sync renders the ne_handle
      // invalid calling mc_close should prevent marfs from ever
      // trying to use it again.
      mc_close(ctx);
      os->flags |= OSF_ERRORS;
      LOG(LOG_ERR, "ne_close failed on %s\n", mc_context->path_template);
      return -1;
   }

   EXIT();
   return 0;
}

// see notes posix_dal_abort().
int mc_abort(DAL_Context* ctx) {
   ENTRY();

   if(! (MC_OS(ctx)->flags & OSF_OPEN)) {
      LOG(LOG_ERR, "abort on not open object stream %s\n", MC_OS(ctx)->url);
      errno = EINVAL;
      return -1;
   }

   MC_OS(ctx)->flags |= OSF_ABORT;
   
   EXIT();
   return 0;
}

// Marks an OS as closed. All "actual" close work should have been
// done in a previous call to mc_sync(). See notes on stream_sync()
// and stream_close() for more information.
int mc_close(DAL_Context* ctx) {
   ENTRY();

   ObjectStream* os     = MC_OS(ctx);

   if(! (os->flags & OSF_OPEN)) {
      LOG(LOG_INFO, "Close on not-open stream %s\n", os->url);
      return 0;
   }

   os->flags &= ~OSF_OPEN;
   os->flags |= OSF_CLOSED;

   EXIT();
   return 0;
}

int mc_del(DAL_Context* ctx) {
   char* path_template = MC_CONTEXT(ctx)->path_template;
   int   nblocks       = MC_CONFIG(ctx)->n + MC_CONFIG(ctx)->e;

   int   impl          = (MC_CONFIG(ctx)->is_sockets ? UDAL_SOCKETS : UDAL_POSIX);
   int   timing_flags  = (MC_FH(ctx)->info.pre.repo->timing_flags
                          | MC_FH(ctx)->info.pre.ns->timing_flags );

   return ne_delete1(MC_CONFIG(ctx)->snprintf, ctx, impl, MC_CONFIG(ctx)->auth, timing_flags, path_template, nblocks);
}


DAL mc_dal = {
   .name         = "MC",
   .name_len     = 2,

   .global_state = NULL,

   .config       = &mc_config,
   .init         = &mc_init,
   .destroy      = &mc_destroy,

   .open         = &mc_open,
   .put          = &mc_put,
   .get          = &mc_get,
   .del          = &mc_del,
   .sync         = &mc_sync,
   .abort        = &mc_abort,
   .close        = &mc_close,

   .update_object_location = &mc_update_path
};


DAL mc_sockets_dal = {
   .name         = "MC_SOCKETS",
   .name_len     = 10,

   .global_state = NULL,

   .config       = &mc_config,
   .init         = &mc_init,
   .destroy      = &mc_destroy,

   .open         = &mc_open,
   .put          = &mc_put,
   .get          = &mc_get,
   .del          = &mc_del,
   .sync         = &mc_sync,
   .abort        = &mc_abort,
   .close        = &mc_close,

   .update_object_location = &mc_update_path
};

#endif // USE_MC


// ===================================================
// FUZZY
// ===================================================

#define POSIX_DAL 0
#define MC_DAL 1
#define OBJ_DAL 2
#define NOP_DAL 3
//#define OPEN_RULE 1
//#define READ_RULE 2
//#define WRITE_RULE 3
//#define CLOSE_RULE 4
//#define STALL 1
//#define FAIL 2


#define FUZZY_CONFIG(CTX) FUZZY_CONTEXT(CTX)->config
#define FUZZY_CONTEXT(CTX) ((Fuzzy_Context*)((CTX)->data.ptr))

//fuzzy_context definition
typedef struct fuzzy_context {
        DAL_Context*        wrap_context;
        Fuzzy_Config*       config;
} Fuzzy_Context;


int check_match(Server_Rule* rule, int func, int pod, int cap, int blk){
    int match = 0;
    //make sure this is the same function, if not we can just return now
    if (func != rule->rule_type){
        return 0;
    }

    //if rule->xxx == -1, that means this rule applies for all xxx 
    match += (pod == rule->pod) || (rule->pod == -1);
    match += (cap == rule->cap) || (rule->cap == -1);
    match += (blk == rule->blk) || (rule->blk == -1);

    //all criteria match, we return ture
    if (match == 3) {
        return 1;
    }
    else {
        return 0;
    }
}

//find udal rule that matches with function type, pod, cap, blk.
//if match, we return the index of the rule 
int fuzzy_check_udal_rule(int func, void* state, int blk) {
    //now we can assume only mc/mc_sockets use this 

    DAL_Context * ctx = (DAL_Context*) state;
    MC_Config* config = MC_CONFIG(ctx);
    /*
    uint32_t hosts_per_pod = config->host_count / config->num_pods;
    uint32_t pod_offset    = MC_CONTEXT(ctx)->pod * hosts_per_pod;
    uint32_t host_offset   = config->host_offset + (blk / config->blocks_per_host);
    uint32_t block_offset  = blk + config->block_offset;
    
    //convert to absolute pod and blk, we assume that 
    //pod, cap, blk in the MARFSCONFIGRC are also absolute 
    int pod = (int)(pod_offset + host_offset);
    int cap = MC_CONTEXT(ctx)->cap;
    int blk_real = (int)(block_offset + config->block_offset);*/

    /*
    * NOTE: FOR NOW WE USE LOGICAL POD AND BLK
    */

    Udal_Rules* udal_rules = MC_CONTEXT(ctx)->udal_rules;
    int num_rules = udal_rules->num_rules;
    int i;

    for(i = 0; i < num_rules; i++) {
        if (check_match(&(udal_rules->rules[i]), func, MC_CONTEXT(ctx)->pod, MC_CONTEXT(ctx)->cap, blk)) {
            //we have to fire up this rule
            return i;
        }
    }

    return -1;

    //now go over rules to check for matching pod/
}

#define DAL_RULE 0
#define UDAL_RULE 1

//helper funciton to parse a rule for an opt
int parse_rule(const char* rule, Fuzzy_Config* config, int dal_rule_cursor, int udal_rule_cursor) {
    char* string, *token; //string must be freed
    int which_func = -1;
    int mode = -1;
    int pos;
    int err = 0;
    int ret = 0;
    double fail_freq = 0;
    unsigned int stall_time_sec = 0;
    unsigned int seed = 0;
    double stall_freq = 0;

    int pod = -1;
    int cap = -1;
    int blk = -1;

    int which_type = -1;
    
    string = strdup(rule); 
    printf("parsing_rule: parsing rule %s\n", string);
    while ((token = strsep(&string, ",")) != NULL) {
        if (!strncmp(token, "which", 5)) {
            //still got to check if this is udal or dal....
            if (token[6] == 'd') {
                which_type = DAL_RULE;
                ret = 0;
            }
            else if (token[6] == 'u') {
                which_type = UDAL_RULE;
                ret = 1;
            }
            else {
                printf("Ungrecodnized dal/udal type in rule token %s\n", token); //debug purpose
                LOG(LOG_ERR, "Ungrecodnized dal/udal type in rule token %s\n", token);
                free(string);
                return -1;
            }
        }
        else if (!strncmp(token, "pod", 3)) {
            pod = atoi(&token[4]);
        }
        else if (!strncmp(token, "cap", 3)) {
            cap = atoi(&token[4]);
        }
        else if (!strncmp(token, "blk", 3)) {
            blk = atoi(&token[4]);
        }
        else if (!strncmp(token, "func", 4)) {
            //this is a function
            if (token[5] == 'o') {
                which_func = OPEN_RULE;
            }
            else if (token[5] == 'r') {
                which_func = READ_RULE;
            }
            else if (token[5] == 'w') {
                which_func = WRITE_RULE;
            }
            else if (token[5] == 'c') {
                which_func = CLOSE_RULE;
            }
            else {
                printf("Unrecognized function in rule token %s\n", token);
                LOG(LOG_ERR, "Unrecognized function in rule token %s\n", token);
                free(string);
                return -1;
            }
        }
        else if (!strncmp(token, "mode", 4)) {
            //this is a mode, either fail or stall
            if (token[5] == 'f') {
                //this is fail
                mode = FAIL;
            }
            else if (token[5] == 's') {
                //this is stall
                mode = STALL;
            }
            else {
                printf("Unrecognized mode in rule token %s\n", token);
                LOG(LOG_ERR, "Unrecognized mode in rule token %s\n", token);
                free(string);
                return -1;
            }
        }
        else if (!strncmp(token, "fail_freq", 9)) {
            //parse time/frequency
            fail_freq  = atof(&token[10]);
        }
        else if (!strncmp(token, "stall_time", 10)) {
            stall_time_sec = (unsigned int)strtoul(&token[11], NULL, 10);
        }
        else if (!strncmp(token, "stall_freq", 10)) {
            stall_freq = atof(&token[11]);
        }
        else if (!strncmp(token, "ret", 3)) {
            //parse return code
            err = atoi(&token[4]);
        }
        else if (!strncmp(token, "seed", 4)) {
            //parse seed
            seed = (unsigned int)strtoul(&token[5], NULL, 10);
        }
    }

    //now check some values
    if (mode == FAIL) {
        if (fail_freq < 0) {
            LOG(LOG_ERR, "Fail frequency must be greater than 0\n");
            ret = -1;
        }
    }
    else if (mode == STALL) {
        if (!stall_time_sec) {
            LOG(LOG_ERR, "Stall time must be greater than 0\n");
            ret = -1;
        }
        if (stall_freq < 0) {
            LOG(LOG_ERR, "Stall frequency must be greater than or equal to 0\n");
            ret = -1;
        }
    }

    //assign values based on which dal
    if (which_type == DAL_RULE) {
        config->dal_rules.rules[dal_rule_cursor].type = which_func;
        config->dal_rules.rules[dal_rule_cursor].mode = mode;
        config->dal_rules.rules[dal_rule_cursor].stall_time_sec = stall_time_sec;
        config->dal_rules.rules[dal_rule_cursor].stall_freq = stall_freq;
        config->dal_rules.rules[dal_rule_cursor].fail_freq = fail_freq;
        config->dal_rules.rules[dal_rule_cursor].err = err;
        config->dal_rules.rules[dal_rule_cursor].seed = seed;
        config->dal_rules.rules[dal_rule_cursor].pod = pod;
        config->dal_rules.rules[dal_rule_cursor].cap = cap;
        printf("Parsed dal rule: func %d mode %d fail_freq %f stall_freq %f stall_time %d err %d seed %d pod %d cap %d\n",
                                which_func, mode, fail_freq, stall_freq, stall_time_sec, err,config->dal_rules.rules[dal_rule_cursor].seed, pod, cap);
    }
    else if (which_type == UDAL_RULE) {
        config->udal_rules.rules[udal_rule_cursor].rule_type = which_func;
        config->udal_rules.rules[udal_rule_cursor].mode = mode;
        config->udal_rules.rules[udal_rule_cursor].stall_time_sec = stall_time_sec;
        config->udal_rules.rules[udal_rule_cursor].stall_freq = stall_freq;
        config->udal_rules.rules[udal_rule_cursor].fail_freq = fail_freq;
        config->udal_rules.rules[udal_rule_cursor].err = err;
        config->udal_rules.rules[udal_rule_cursor].seed = seed;
        config->udal_rules.rules[udal_rule_cursor].pod = pod;
        config->udal_rules.rules[udal_rule_cursor].cap = cap;
        config->udal_rules.rules[udal_rule_cursor].blk = blk;
    }
    free(string);
    return ret;
}

//wrap the default dal
int wrap_default(struct DAL* dal, Fuzzy_Config* config, xDALConfigOpt** opts, size_t opt_count) {
    //we start by parsing rules
    int i;
    int ret = 0;

    int cursors[2];
    memset(cursors, 0, sizeof(int) * 2);

    for (i = 0; i < opt_count; i++) {
        if (!strcmp(opts[i]->key, "rule")) {
            ret = parse_rule(opts[i]->val.value.str, config, cursors[0], cursors[1]);
            if (ret == -1) {
                LOG(LOG_ERR, "Failed to parse rule %s\n", opts[i].val.value.str);
                return -1;
            }
            cursors[ret]++;
        }
    }

    return ret;
}

int wrap_mc(struct DAL* dal, Fuzzy_Config* config, int is_sockets, xDALConfigOpt** opts, size_t opt_count) {
    int i;
    MC_Config* mc_config = malloc(sizeof(MC_Config));
    memset(mc_config, 0, sizeof(MC_Config));

    mc_config->degraded_log_fd = -1;
    mc_config->degraded_log_path = NULL;
    char* mc_user = NULL;
    int cursors[2]; //cursors[0] for dal; cursors[1] for udal
    memset(cursors, 0, sizeof(int) * 2);

    for(i = 0; i < opt_count; i++) {
        if (!strcmp(opts[i]->key, "rule")) {
            int ret = parse_rule(opts[i]->val.value.str, config, cursors[0], cursors[1]);
            if (ret == -1) {
                LOG(LOG_ERR, "Failed to parse rule %s\n", opts[i]->val.value.str);
                return -1;
            }
            cursors[ret] ++;
        }
        else if (!strcmp(opts[i]->key, "wrap")) {
            ;
        }
        //the rest is simliar to mc_config
        else if (!strcmp(opts[i]->key, "n")) {
            mc_config->n = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"n\" = %d\n", mc_config->n);
        }
        else if (!strcmp(opts[i]->key, "e")) {
            mc_config->e = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"e\" = %d\n", mc_config->e);
        }
        else if (!strcmp(opts[i]->key, "num_pods")) {
            mc_config->num_pods = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"num_pods\" = %d\n", mc_config->num_pods);
        }
        else if (!strcmp(opts[i]->key, "num_cap")) {
            mc_config->num_cap = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"num_cap\" = %d\n", mc_config->num_cap);
        }
        else if (!strcmp(opts[i]->key, "scatter_width")) {
            mc_config->scatter_width = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"scatter_width\" = %d\n", mc_config->scatter_width);
        }
        else if (!strcmp(opts[i]->key, "degraded_log_dir")) {
            mc_config->degraded_log_path = strdup(opts[i]->val.value.str);
            LOG(LOG_INFO, "parsing mc option \"degraded_log_path\" = %s\n", mc_config->degraded_log_path);
        }
        else if (!strcmp(opts[i]->key, "host_offset")) {
            mc_config->host_offset = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"host_offset\" = %d\n", mc_config->host_offset);
        }
        else if (!strcmp(opts[i]->key, "host_count")) {
            mc_config->host_count = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"host_count\" = %d\n", mc_config->host_count);
        }
        else if (!strcmp(opts[i]->key, "blocks_per_host")) {
            mc_config->blocks_per_host = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"blocks_per_host\" = %d\n", mc_config->blocks_per_host);
        }
        else if (!strcmp(opts[i]->key, "block_offset")) {
            mc_config->block_offset = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"block_offset\" = %d\n", mc_config->block_offset);
        }
        else if (!strcmp(opts[i]->key, "global_block_numbering")) {
            mc_config->global_block_numbering = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"global_block_numbering\" = %d\n", mc_config->global_block_numbering);
        }
        else if (!strcmp(opts[i]->key, "pod_offset")) {
            mc_config->pod_offset = strtol(opts[i]->val.value.str, NULL, 10);
            LOG(LOG_INFO, "parsing mc option \"pod_offset\" = %d\n", mc_config->pod_offset);
        }
        else if (!strcmp(opts[i]->key, "mc_user")) {
            mc_user = strdup(opts[i]->val.value.str);
            LOG(LOG_INFO, "parsing mc option \"mc_user\" = %s\n", mc_user);
        }
        else {
            LOG(LOG_ERR, "Unrecognized MC DAL (sockets) config option: %s\n", opts[i]->key);
            free(mc_config);
            return -1;
        }
    }

    if (mc_config->degraded_log_path == NULL) {
        LOG(LOG_ERR, "no degraded_log_dir specified for wrap DAL '%s' in fuzzy dal wrap.\n", config->wrap_dal->name);
        return -1;
    }
    else {
        SEM_INIT(&mc_config->lock, 0, 1);
    }
#ifdef S3_AUTH

    if (is_sockets) {

        if (!mc_user)
            mc_user = strdup(DEFAULT_SKT_AUTH_USER);

        int err = 0;
        if (skt_auth_init(mc_user, &mc_config->auth)) {
            LOG(LOG_ERR, "Authentication-init failed for user '%s' in sockets-DAL '%s'.\n", mc_user, config->wrap_dal->name);
            err = 1;
        }
        else if (mc_config->auth == NULL) {
            LOG(LOG_ERR, "libne was built without enabling socket-authentication, "" but DAL '%s' requires it.\n", config->wrap_dal->name);
            err = 1;
        }

        free(mc_user);
        if (err)
            return -1;
    }
#endif

    mc_config->is_sockets = is_sockets;
    if (is_sockets)
        mc_config->snprintf = mc_path_snprintf_sockets;
    else
        mc_config->snprintf = ne_default_snprintf;

    free_xdal_config_options(opts);

    config->wrap_dal->global_state = mc_config;
    EXIT();
    return 0;
}

//helper functions to find number of dal rules and number of udal rules
void get_num_dal_udal_rules(xDALConfigOpt** opts, 
                           size_t opt_count, 
                           int* num_dal_rules, 
                           int* num_udal_rules) {
    int num_dals = 0;
    int num_udals = 0;
    int i;
    int which_strlen = strlen("which");
    for (i = 0; i < opt_count; i++) {
        if (!strcmp(opts[i]->key, "rule")) {
            //now check if it is a dal or udal, THIS MUST BE REINFORCED IN THE CONFIG
            if (!strncmp(opts[i]->val.value.str, "which", which_strlen)
                && (opts[i]->val.value.str[which_strlen + 1] == 'd')) {
                num_dals ++;
            }
            else if (!strncmp(opts[i]->val.value.str, "which", which_strlen)
                     && (opts[i]->val.value.str[which_strlen + 1] == 'u')) {
                num_udals ++;
            }
        }
    }

    *num_dal_rules = num_dals;
    *num_udal_rules = num_udals;
}


int fuzzy_config(struct DAL*     dal,
              xDALConfigOpt** opts,
              size_t          opt_count) {

    ENTRY();

    Fuzzy_Config* config = (Fuzzy_Config*) malloc(sizeof(Fuzzy_Config));
    memset(config, 0, sizeof(Fuzzy_Config));

    enum dal_type wrap_dal;
    int ret = 0;
    int num_dal_rules = 0;
    int num_udal_rules = 0;
    int i;
    //first pass, check how many rules for dal, how many rules for udal
    get_num_dal_udal_rules(opts, opt_count, &num_dal_rules, &num_udal_rules);

    printf("fuzzy_config: number of dal rules %d; number of udal rules %d\n", num_dal_rules, num_udal_rules);
    config->dal_rules.num_rules = num_dal_rules;
    config->udal_rules.num_rules = num_udal_rules;
    config->dal_rules.rules = (Rule*)malloc(num_dal_rules * sizeof(Rule));
    config->udal_rules.rules = (Server_Rule*)malloc(num_udal_rules * sizeof(Server_Rule));

    //second pass, parse rules
    for (i = 0; i < opt_count; i++) {
        if (!strcmp(opts[i]->key, "wrap")) {
            //get the wrapped dal
            config->wrap_dal = get_DAL(opts[i]->val.value.str);
            if (!strcmp(opts[i]->val.value.str, "POSIX")) {
                ret = wrap_default(dal, config, opts, opt_count);
            }
            else if (!strcmp(opts[i]->val.value.str, "NO_OP")) {
                ret = wrap_default(dal, config, opts, opt_count);
            }
            else if (!strcmp(opts[i]->val.value.str, "OBJ")) {
                ret = wrap_default(dal, config, opts, opt_count);
            }
            else if (!strcmp(opts[i]->val.value.str, "MC")) {
                ret = wrap_mc(dal, config, 0, opts, opt_count);
            }
            else if (!strcmp(opts[i]->val.value.str, "MC_SOCKETS")) {
                ret = wrap_mc(dal, config, 1, opts, opt_count);
            }
            else {

                LOG(LOG_ERR, "Unspecified dal to wrap\n");
                ret =  -1;
                break;
            }
            break;
        }
    }
    config->udal_rules.check_rule = fuzzy_check_udal_rule;
    dal->global_state = config;
    EXIT();
    return ret;
}

int fuzzy_init(DAL_Context* ctx, DAL* dal, void* fh) {
    ENTRY();
    int i;

    ctx->data.ptr = malloc(sizeof(Fuzzy_Context));
    if (! FUZZY_CONTEXT(ctx)) {
        LOG(LOG_ERR, "failed to allocate memory for Fuzzy_Context\n");
        return -1;
    }
    memset(FUZZY_CONTEXT(ctx), 0, sizeof(Fuzzy_Context));
    
    FUZZY_CONTEXT(ctx)->wrap_context = malloc(sizeof(DAL_Context));

    if (! FUZZY_CONTEXT(ctx)->wrap_context) {
        LOG(LOG_ERR, "failed to allocate memory for wrap dal context\n");
        return -1;
    }
    memset(FUZZY_CONTEXT(ctx)->wrap_context, 0, sizeof(DAL_Context));

    //call init in wrapped dal
    FUZZY_CONFIG(ctx) = (Fuzzy_Config*)dal->global_state;   

    if (FUZZY_CONFIG(ctx)->wrap_dal->init(FUZZY_CONTEXT(ctx)->wrap_context, FUZZY_CONFIG(ctx)->wrap_dal, fh)) {
        LOG(LOG_ERR, "Fuzzy init failed to init wrapped dal\n");
        return -1;
    }

    //pass udal rules to wrapped dal context only if it is MC or MC_SOCKETS
    if ((!strcmp(FUZZY_CONFIG(ctx)->wrap_dal->name, "MC")) || (!strcmp(FUZZY_CONFIG(ctx)->wrap_dal->name, "MC_SOCKETS"))){
        printf("passing udal_rules from fuzzy config to wrapped mc context\n");
        MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->udal_rules = &(FUZZY_CONFIG(ctx)->udal_rules);
    }


    //copy config seeds into context so each context has its own seeds
    /*for (i = 0; i < MAX_RULES; i++) {
        FUZZY_CONTEXT(ctx)->ops_seeds[i] = FUZZY_CONFIG(ctx)->rules[i].seed;
    }
    */
    EXIT();
    return 0;
}

int fuzzy_destroy(DAL_Context* ctx, struct DAL* dal) {
    free(FUZZY_CONTEXT(ctx)->wrap_context);
    free(FUZZY_CONTEXT(ctx));
    return 0;
}

int find_matching_rule(Rule* dal_rule, int func, int pod, int cap) {
    //first check if function type matches
    if (dal_rule->type != func) {
        return 0;
    }

    int ret = 0;
    //now check if pod and cap match
    int pod_match = (pod == dal_rule->pod) || (dal_rule->pod == -1);
    int cap_match = (cap == dal_rule->cap) || (dal_rule->cap == -1);

    if (pod_match + cap_match == 2) {
        ret = 1;
    }

    return ret;
}

int check_fuzzy_dal_rule(int func, DAL_Context* ctx) {
    Dal_Rules* dal_rules = &(FUZZY_CONFIG(ctx)->dal_rules);
    int ret = -1;
    if ((!strcmp(FUZZY_CONFIG(ctx)->wrap_dal->name, "MC")) 
        || (!strcmp(FUZZY_CONFIG(ctx)->wrap_dal->name, "MC_SOCKETS"))) {
        //this is MC or MC_SOCKETS, we apply rules here
        //first we get the pod and cap from wrapped dal context
        MC_Config* wrap_config = MC_CONFIG(FUZZY_CONTEXT(ctx)->wrap_context);
        /*
        uint32_t pod = MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->pod;
        uint32_t hosts_per_pod = wrap_config->host_count / wrap_config->num_pods;
        uint32_t pod_offset = pod * hosts_per_pod;
        */
        printf("check_fuzzy_dal_rule: checking func type %d pod %d cap %d\n", func, MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->pod, MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->cap);
        int i;
        
        for(i = 0; i < dal_rules->num_rules; i++) {
            if (find_matching_rule(&dal_rules->rules[i], 
                                   func, 
                                   MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->pod, 
                                   MC_CONTEXT(FUZZY_CONTEXT(ctx)->wrap_context)->cap)) {
                printf("we have a match for func %d with rule index %d\n", func, i);
                ret = i;
                break;
            }
        }

    }

    return ret;
}

int execute_dal_rule(DAL_Context* ctx, int rule_idx) {
    Rule* rule = &(FUZZY_CONFIG(ctx)->dal_rules.rules[rule_idx]);
    printf("execute dal rule rule pointer val %p rule seed %u\n", rule, rule->seed);
    double roll = rand_r(&rule->seed) % 101;
    int err = 0;
    if (rule->mode == FAIL) {
        if (roll <= (rule->fail_freq * 100)) {
            err = rule->err;
        }
    }
    else if (rule->mode == STALL) {
        if (roll <= (rule->stall_freq * 100)) {
            sleep(rule->stall_time_sec);
        }
    }

    return err;
}

int fuzzy_open( DAL_Context* ctx,
                int is_put,
                size_t chunk_offset,
                size_t content_length,
                uint8_t preserve_write_count,
                uint16_t timeout) {
    ENTRY();
    int ret = 0;

    //first check for rule
    printf("fuzzy_open checking rules\n");
    int rule_idx = check_fuzzy_dal_rule(OPEN_RULE, ctx);
    if (rule_idx != -1) {
        //there is a rule for open with pod and cap
        ret = execute_dal_rule(ctx, rule_idx);
    }

    if (!ret) {
        //run normally
        ret = FUZZY_CONFIG(ctx)->wrap_dal->open(FUZZY_CONTEXT(ctx)->wrap_context,
                                                is_put,
                                                chunk_offset,
                                                content_length,
                                                preserve_write_count,
                                                timeout);
    }

    EXIT();
    return ret;
}

//this is the write
int fuzzy_put(DAL_Context* ctx, const char* buf, size_t size){
    ENTRY();
    int ret = 0;
    printf("fuzzy_put checking rules\n");
    int rule_idx = check_fuzzy_dal_rule(WRITE_RULE, ctx);

    if (rule_idx != -1) {
        ret = execute_dal_rule(ctx, rule_idx);
    }

    if (!ret) {
        ret = FUZZY_CONFIG(ctx)->wrap_dal->put(FUZZY_CONTEXT(ctx)->wrap_context,
                                                buf,
                                                size);
    }

    EXIT();
    return ret;
}

//read
ssize_t fuzzy_get(DAL_Context* ctx, char* buf, size_t size) {
    ENTRY();
    int ret = 0;
    printf("fuzzy_get checking rules\n");
    int rule_idx = check_fuzzy_dal_rule(READ_RULE, ctx);

    if (rule_idx != -1) {
        ret = execute_dal_rule(ctx, rule_idx);
    }

    if (!ret) {
        ret = FUZZY_CONFIG(ctx)->wrap_dal->get(FUZZY_CONTEXT(ctx)->wrap_context,
                                                buf,
                                                size);
    }

    EXIT();
    return ret;
}

int fuzzy_del(DAL_Context* ctx) {
    ENTRY();
    int ret = FUZZY_CONFIG(ctx)->wrap_dal->del(FUZZY_CONTEXT(ctx)->wrap_context);
    EXIT();
    return ret;
}

int fuzzy_close(DAL_Context* ctx) {
    ENTRY();
    int ret = 0;
    printf("fuzz_close checking rule\n");
    int rule_idx = check_fuzzy_dal_rule(CLOSE_RULE, ctx);

    if (rule_idx != -1) {
        ret = execute_dal_rule(ctx, rule_idx);
    }

    if (!ret) {
        ret = FUZZY_CONFIG(ctx)->wrap_dal->close(FUZZY_CONTEXT(ctx)->wrap_context);
    }

    EXIT();
    return ret;
}

int fuzzy_sync(DAL_Context* ctx) {
    ENTRY();
    int ret = FUZZY_CONFIG(ctx)->wrap_dal->sync(FUZZY_CONTEXT(ctx)->wrap_context);
    EXIT();
    return ret;
}

int fuzzy_abort(DAL_Context* ctx) {
    ENTRY();
    int ret = FUZZY_CONFIG(ctx)->wrap_dal->abort(FUZZY_CONTEXT(ctx)->wrap_context);
}

int fuzzy_update_path(DAL_Context* ctx) {
    ENTRY();
    int ret = FUZZY_CONFIG(ctx)->wrap_dal->update_object_location(FUZZY_CONTEXT(ctx)->wrap_context);
    EXIT();
    return ret;
}

DAL fuzzy_dal = {
    .name           = "FUZZY",
    .name_len       = 5,

    .global_state   = NULL,

    .config         = &fuzzy_config,
    .init           = &fuzzy_init,
    .destroy        = &fuzzy_destroy,

    .open           = &fuzzy_open,
    .put            = &fuzzy_put,
    .get            = &fuzzy_get,
    .del            = &fuzzy_del,
    .sync           = &fuzzy_sync,
    .abort          = &fuzzy_abort,
    .close          = &fuzzy_close,

    .update_object_location = &fuzzy_update_path
};


// ===========================================================================
// GENERAL
// ===========================================================================

// should be plenty
// static const size_t MAX_DAL = 32; // stupid compiler
#define MAX_DAL 32

static DAL*   dal_list[MAX_DAL];
static size_t dal_count = 0;


# define DL_CHECK(SYM)                                                  \
   if (! dal->SYM) {                                                    \
      LOG(LOG_ERR, "DAL '%s' has no symbol '%s'\n", dal->name, #SYM);   \
      return -1;                                                         \
   }


// add a new DAL to dal_list
int install_DAL(DAL* dal) {
   if (! dal) {
      LOG(LOG_ERR, "NULL arg\n");
      return -1;
   }
   // insure that no DAL with the given name already exists
   int i;
   for (i=0; i<dal_count; ++i) {
      if ((dal->name_len == dal_list[i]->name_len)
          && (! strcmp(dal->name, dal_list[i]->name))) {

         LOG(LOG_ERR, "DAL named '%s' already exists\n", dal->name);
         return -1;
      }
   }

   // validate that DAL has all required members
   DL_CHECK(name);
   DL_CHECK(name_len);
   DL_CHECK(init);
   DL_CHECK(destroy);
   DL_CHECK(open);
   DL_CHECK(put);
   DL_CHECK(get);
   DL_CHECK(sync);
   DL_CHECK(abort);
   DL_CHECK(close);
   DL_CHECK(del);
   DL_CHECK(update_object_location);
   if (dal_count >= MAX_DAL) {
         LOG(LOG_ERR,
             "No room for DAL '%s'.  Increase MAX_DAL_COUNT and rebuild.\n",
             dal->name);
         return -1;
   }

   // install
   LOG(LOG_INFO, "Installing DAL '%s'\n", dal->name);
   dal_list[dal_count] = dal;
   ++ dal_count;

   return 0;
}

# undef DL_CHECK



// Untested support for dynamically-loaded DAL. This is not a link-time
// thing, but a run-time thing.  The name in the configuration is something
// like: "DYNAMIC /path/to/my/lib", and we go look for all the DAL symbols
// (e.g. dal_open()) in that module, and install a corresponding DAL.
// *All* DAL symbols must be defined in the library.

static
DAL* dynamic_DAL(const char* name) {

   DAL* dal = (DAL*)calloc(1, sizeof(DAL));
   if (! dal) {
      LOG(LOG_ERR, "no memory for new DAL '%s'\n", name);
      return NULL;
   }

   // zero everything, so if we forget to update something it will be obvious
   memset(dal, 0, sizeof(DAL));

   dal->name     = name;
   dal->name_len = strlen(name);

   if (! strcmp(name, "DYNAMIC")) {
      return NULL;

      // second token is library-name
      const char* delims = " \t";
      char* delim = strpbrk(name, delims);
      if (! delim)
         return NULL;
      char* lib_name = delim + strspn(delim, delims);
      if (! *lib_name)
         return NULL;

      // dig out symbols
      void* lib = dlopen(lib_name, RTLD_LAZY);
      if (! lib) {
         LOG(LOG_ERR, "Couldn't open dynamic lib '%s'\n", lib_name);
         return NULL;
      }

      dal->global_state = NULL;

      dal->config  = (dal_config)      dlsym(lib, "dal_config");
      dal->init    = (dal_ctx_init)    dlsym(lib, "dal_init");
      dal->destroy = (dal_ctx_destroy) dlsym(lib, "dal_destroy");

      dal->open    = (dal_open)        dlsym(lib, "dal_open");
      dal->put     = (dal_put)         dlsym(lib, "dal_put");
      dal->get     = (dal_get)         dlsym(lib, "dal_get");
      dal->sync    = (dal_sync)        dlsym(lib, "dal_sync");
      dal->abort   = (dal_abort)       dlsym(lib, "dal_abort");
      dal->close   = (dal_close)       dlsym(lib, "dal_close");

      dal->update_object_location =
         (dal_update_object_location)  dlsym(lib, "dal_update_object_location");

      dlclose(lib);

   }
   else {
      // unknown name
      return NULL;
   }


   return dal;
}



// Applications can push private custom DALs before calling
// read_configuration() [or, at least, before calling
// validate_configuration()].  They should just build a struct like the
// ones used below, and call install_DAL() with their struct.  Then those
// DALs will be found, when referenced in the configuration.

DAL* get_DAL(const char* name) {
   static int needs_init = 1;
   if (needs_init) {

      // one-time initialization of dal_list
      assert(! install_DAL(&obj_dal) );
      assert(! install_DAL(&nop_dal) );
      assert(! install_DAL(&posix_dal) );
#if USE_MC
      assert(! install_DAL(&mc_dal) );
      assert(! install_DAL(&mc_sockets_dal) );
#endif
      assert(! install_DAL(&fuzzy_dal));
      needs_init = 0;
   }

   // look up <name> in known DALs
   size_t name_len = strlen(name);
   int i;
   for (i=0; i<dal_count; ++i) {
      if ((name_len == dal_list[i]->name_len)
          && (! strcmp(name, dal_list[i]->name))) {

         return dal_list[i];
      }
   }

   // not found.  Maybe it was dynamic?
   if (! strcmp(name, "DYNAMIC")) {
      DAL* dynamic = dynamic_DAL(name);
      assert(! install_DAL(dynamic) );
      return dynamic;
   }

   return NULL;
}
