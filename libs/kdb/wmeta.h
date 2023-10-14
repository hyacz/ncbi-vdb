/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#pragma once

#include <klib/symbol.h>

#include <kfs/directory.h>
#include <kfs/md5.h>

#include <kdb/meta.h>
#include <kdb/manager.h>
#include <kdb/database.h>
#include <kdb/table.h>

typedef struct KMetadata KMetadata;
#define KMETA_IMPL KMetadata
#include "meta-base.h"

struct KDatabase;
struct KTable;
struct KWColumn;
struct KDBManager;
struct KMDataNode;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KMetadata KMetadata;
struct KMetadata
{
    KMetadataBase dad;

    // BSTNode nn; ?? is it used? not as far as i could see in this module

    KDirectory *dir;
    KDBManager *mgr;

    /* owner */
    KDatabase *db;
    KTable *tbl;
    struct KWColumn *col;

    KMD5SumFmt * md5;

    /* root node */
    KMDataNode *root;

    KSymbol sym;

    uint32_t opencount;
    uint32_t vers;
    uint32_t rev;
    uint8_t read_only;
    uint8_t dirty;
    bool byteswap;

    char path [ 1 ];
};

rc_t KMetadataMake ( KMetadata **metap, KDirectory *dir, const char *path, uint32_t rev, bool populate, bool read_only );

#ifdef __cplusplus
}
#endif

