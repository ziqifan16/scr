/*
 * Copyright (c) 2009, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov>.
 * LLNL-CODE-411039.
 * All rights reserved.
 * This file is part of The Scalable Checkpoint / Restart (SCR) library.
 * For details, see https://sourceforge.net/projects/scalablecr/
 * Please also read this file: LICENSE.TXT.
*/

/* This is a utility program that checks various values in the flush
 * file. */

#include "scr.h"
#include "scr_io.h"
#include "scr_err.h"
#include "scr_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#define PROG ("scr_flush_file")

int print_usage()
{
  printf("\n");
  printf("  Usage:  %s --dir <cntl_dir> [--needflush <id> | --latest]\n", PROG);
  printf("\n");
  exit(1);
}

struct arglist {
  char* dir;      /* control direcotry */
  int need_flush; /* check whether a certain checkpoint id needs to be flushed */
  int latest;     /* return the id of the latest (most recent) checkpoint in cache */
};

int process_args(int argc, char **argv, struct arglist* args)
{
  int ckpt;

  /* define our options */
  static struct option long_options[] = {
    {"dir",       required_argument, NULL, 'd'},
    {"needflush", required_argument, NULL, 'n'},
    {"latest",    no_argument,       NULL, 'l'},
    {"help",      no_argument,       NULL, 'h'},
    {0, 0, 0, 0}
  };

  /* set our options to default values */
  args->dir        = NULL;
  args->need_flush = -1;
  args->latest     = 0;

  /* loop through and process all options */
  int c;
  do {
    /* read in our next option */
    int option_index = 0;
    c = getopt_long(argc, argv, "d:n:lh", long_options, &option_index);
    switch (c) {
      case 'd':
        /* control directory */
        args->dir = optarg;
        break;
      case 'n':
        /* check whether specified checkpoint id needs to be flushed */
        ckpt = atoi(optarg);
        if (ckpt <= 0) {
          return 1;
        }
        args->need_flush = ckpt;
        break;
      case 'l':
        /* return the id of the latest (most recent) checkpoint in cache */
        args->latest = 1;
        break;
      case 'h':
        /* print help message and exit */
        print_usage();
        break;
      case '?':
        /* getopt_long printed an error message */
        break;
      default:
        if (c != -1) {
          /* missed an option */
          scr_err("%s: Option '%s' specified but not processed", PROG, argv[option_index]);
        }
    }
  } while (c != -1);

  /* check that we got a directory name */
  if (args->dir == NULL) {
    scr_err("%s: Must specify control directory via '--dir <cntl_dir>'", PROG);
    return 0;
  }

  return 1;
}

int main (int argc, char *argv[])
{
  /* process command line arguments */
  struct arglist args;
  if (!process_args(argc, argv, &args)) {
    return 1;
  }

  /* determine the number of bytes we need to hold the full name of the flush file */
  int filelen = snprintf(NULL, 0, "%s/flush.scrinfo", args.dir);
  filelen++; /* add one for the terminating NUL char */

  /* allocate space to store the filename */
  char* file = NULL;
  if (filelen > 0) {
    file = (char*) malloc(filelen);
  }
  if (file == NULL) {
    scr_err("%s: Failed to allocate storage to store flush file name @ %s:%d",
            PROG, __FILE__, __LINE__
    );
    return 1;
  }

  /* build the full file name */
  int n = snprintf(file, filelen, "%s/flush.scrinfo", args.dir);
  if (n >= filelen) {
    scr_err("%s: Flush file name is too long (need %d bytes, %d byte buffer) @ %s:%d",
            PROG, n, filelen, __FILE__, __LINE__
    );
    free(file);
    return 1;
  }

  /* assume we'll fail */
  int rc = 1;

  /* create a new hash to hold the file data */
  scr_hash* hash = scr_hash_new();

  /* read in our flush file */
  if (scr_hash_read(file, hash) != SCR_SUCCESS) {
    /* failed to read the flush file */
    goto cleanup;
  }

  /* check whether a specified checkpoint id needs to be flushed */
  if (args.need_flush != -1) {
    /* first, see if we have this checkpoint */
    scr_hash* ckpt_hash = scr_hash_get_kv_int(hash, SCR_FLUSH_KEY_CKPT, args.need_flush);
    if (ckpt_hash != NULL) {
      /* now check for the PFS location marker */
      scr_hash* location_hash = scr_hash_get(ckpt_hash, SCR_FLUSH_KEY_LOCATION);
      scr_hash_elem* pfs_elem = scr_hash_elem_get(location_hash, SCR_FLUSH_KEY_LOCATION_PFS);
      if (pfs_elem == NULL) {
        /* we have the checkpoint, but we didn't find the PFS marker,
         * so return success to indicate that we do need to flush it */
        rc = 0;
      }
    }
    goto cleanup;
  }

  /* print the latest checkpoint id to stdout */
  if (args.latest) {
    /* scan through the checkpoint ids to find the most recent */
    int latest_ckpt = -1;
    scr_hash_elem* elem;
    scr_hash* latest_hash = scr_hash_get(hash, SCR_FLUSH_KEY_CKPT);
    for (elem = scr_hash_elem_first(latest_hash);
         elem != NULL;
         elem = scr_hash_elem_next(elem))
    {
      /* update our latest checkpoint id if this checkpoint is more recent */
      int ckpt = scr_hash_elem_key_int(elem);
      if (ckpt > latest_ckpt) {
        latest_ckpt = ckpt;
      }
    }

    /* if we found a checkpoint, print its id and return success */
    if (latest_ckpt != -1) {
      printf("%d\n", latest_ckpt);
      rc = 0;
    }
    goto cleanup;
  }

cleanup:
  /* delete the hash holding the flush file data */
  scr_hash_delete(hash);

  /* free off our file name storage */
  if (file != NULL) {
    free(file);
    file = NULL;
  }

  /* return appropriate exit code */
  return rc;
}
