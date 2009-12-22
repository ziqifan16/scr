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

#ifndef SCR_META_H
#define SCR_META_H

#include <sys/types.h>

/* needed for SCR_MAX_FILENAME */
#include "scr.h"

/* compute crc32, needed for uLong */
#include <zlib.h>

/* file types */
#define SCR_FILE_FULL (0)
#define SCR_FILE_XOR  (2)

/*
=========================================
Metadata functions
=========================================
*/

/* data structure for meta file */
struct scr_meta
{
  int rank;
  int ranks;
  int checkpoint_id;
  int filetype;

  char filename[SCR_MAX_FILENAME];
  unsigned long filesize;
  int complete;
  int crc32_computed;
  uLong crc32;

  char src_filename[SCR_MAX_FILENAME];
  unsigned long src_filesize;
  int src_complete;
  int src_crc32_computed;
  uLong src_crc32;
};

/* build meta data filename for input file */
int scr_meta_name(char* metaname, const char* file);

/* initialize meta structure to represent file, filetype, and complete */
void scr_set_meta(struct scr_meta* meta, const char* file, int rank, int ranks, int checkpoint_id, int filetype, int complete);

/* initialize meta structure to represent file, filetype, and complete */
void scr_copy_meta(struct scr_meta* m1, const struct scr_meta* m2);

/* read meta for file_orig and fill in meta structure */
int scr_read_meta(const char* file_orig, struct scr_meta* meta);

/* creates corresponding .scr meta file for file to record completion info */
int scr_write_meta(const char* file, const struct scr_meta* meta);

#endif