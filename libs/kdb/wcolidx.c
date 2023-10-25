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

#include <kdb/extern.h>

#include "wcolidx.h"
#include "widxblk.h"

#include "werror.h"

#include <kfs/file.h>
#include <kfs/md5.h>

#include <sysalloc.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


/*--------------------------------------------------------------------------
 * KWColumnIdx
 *  the index fork
 */


/* EstablishIdRange
 */
static
void KWColumnIdxEstablishIdRange ( KWColumnIdx *self )
{
    int64_t first, upper;

    if ( ! KWColumnIdx0IdRange ( & self -> idx0, & self -> id_first, & self -> id_upper ) )
    {
        if ( ! KWColumnIdx1IdRange ( & self -> idx1, & self -> id_first, & self -> id_upper ) )
            self -> id_first = self -> id_upper = 1;
    }
    else if ( KWColumnIdx1IdRange ( & self -> idx1, & first, & upper ) )
    {
        if ( self -> id_first > first )
            self -> id_first = first;
        if ( self -> id_upper < upper )
            self -> id_upper = upper;
    }
}


/* Create
 */
rc_t KWColumnIdxCreate ( KWColumnIdx *self, KDirectory *dir,
    KMD5SumFmt *md5, KCreateMode mode, uint64_t *data_eof,
    size_t pgsize, int32_t checksum )
{
    rc_t rc = 0;
    uint64_t idx2_eof;
    uint32_t idx0_count;

    assert ( self != NULL );

    rc = KWColumnIdx1Create ( & self -> idx1, dir, md5, mode,
        data_eof, & idx0_count, & idx2_eof, pgsize, checksum );
    if ( rc == 0 )
    {
        rc = ( self -> idx1 . vers < 3 ) ?
            KWColumnIdx0Create_v1 ( & self -> idx0, dir, md5, mode, self -> idx1 . bswap ):
            KWColumnIdx0Create ( & self -> idx0, dir, idx0_count, md5, mode, self -> idx1 . bswap );
        if ( rc == 0 )
        {
            rc = KWColumnIdx2Create ( & self -> idx2, dir, md5, mode, idx2_eof );
            if ( rc == 0 )
            {
                KWColumnIdxEstablishIdRange ( self );

                /* successful return */
                return 0;
            }

            KWColumnIdx0Whack ( & self -> idx0 );
        }

        KWColumnIdx1Whack ( & self -> idx1 );
    }

    /* failure return */
    return rc;
}

/* Open
 */
rc_t KWColumnIdxOpenRead ( KWColumnIdx *self, const KDirectory *dir,
    uint64_t *data_eof, size_t *pgsize, int32_t *checksum )
{
    rc_t rc;
    uint64_t idx2_eof;
    uint32_t idx0_count;

    assert ( self != NULL );

    rc = KWColumnIdx1OpenRead ( & self -> idx1,
        dir, data_eof, & idx0_count, & idx2_eof, pgsize, checksum );
    if ( rc == 0 )
    {
        rc = ( self -> idx1 . vers < 3 ) ?
            KWColumnIdx0OpenRead_v1 ( & self -> idx0, dir, self -> idx1 . bswap ):
            KWColumnIdx0OpenRead ( & self -> idx0, dir, idx0_count, self -> idx1 . bswap );
        if ( rc == 0 )
        {
            rc = KWColumnIdx2OpenRead ( & self -> idx2, dir, idx2_eof );
            if ( rc == 0 || GetRCState ( rc ) == rcNotFound )
            {
                KWColumnIdxEstablishIdRange ( self );
                return 0;
            }

            KWColumnIdx0Whack ( & self -> idx0 );
        }

        KWColumnIdx1Whack ( & self -> idx1 );
    }

    return rc;
}

rc_t KWColumnIdxOpenUpdate ( KWColumnIdx *self, KDirectory *dir,
    KMD5SumFmt *md5, uint64_t *data_eof, size_t *pgsize, int32_t *checksum )
{
    rc_t rc;
    uint64_t idx2_eof;
    uint32_t idx0_count;

    assert ( self != NULL );

    rc = KWColumnIdx1OpenUpdate ( & self -> idx1, dir,
        md5, data_eof, & idx0_count, & idx2_eof, pgsize, checksum );
    if ( rc == 0 )
    {
        rc = ( self -> idx1 . vers < 3 ) ?
            KWColumnIdx0Create_v1 ( & self -> idx0, dir, md5, kcmOpen, self -> idx1 . bswap ):
            KWColumnIdx0Create ( & self -> idx0, dir, idx0_count, md5, kcmOpen, self -> idx1 . bswap );
        if ( rc == 0 )
        {
            rc = KWColumnIdx2Create ( & self -> idx2, dir, md5, kcmOpen, idx2_eof );
            if ( rc == 0 )
            {
                KWColumnIdxEstablishIdRange ( self );
                return 0;
            }

            KWColumnIdx0Whack ( & self -> idx0 );
        }
        KWColumnIdx1Whack ( & self -> idx1 );
    }

    return rc;
}

/* Whack
 */
rc_t KWColumnIdxWhack ( KWColumnIdx *self,
    uint64_t data_eof, size_t pgsize, int32_t checksum )
{
    rc_t rc;

    assert ( self != NULL );

    if ( self -> commit_count != 0 )
    {
        rc = KWColumnIdx1WriteHeader ( & self -> idx1,
            data_eof, self -> idx0 . count, self -> idx2 . eof,
            pgsize, checksum );
        if ( rc != 0 )
            return rc;

        self -> commit_count = 0;
    }

    rc = KWColumnIdx1Whack ( & self -> idx1 );
    if ( rc == 0 )
    {
        KWColumnIdx0Whack ( & self -> idx0 );
        KWColumnIdx2Whack ( & self -> idx2 );
    }

    return rc;
}

/* Version
 */
#ifndef KWColumnIdxVersion
rc_t KWColumnIdxVersion ( const KWColumnIdx *self, uint32_t *version )
{
    return KWColumnIdx1Version ( & self -> idx1, version );
}
#endif

/* IdRange
 *  returns range of ids contained within
 */
rc_t KWColumnIdxIdRange ( const KWColumnIdx *self,
    int64_t *first, int64_t *last )
{
    assert ( self != NULL );
    assert ( first != NULL );
    assert ( last != NULL );

    * first = self -> id_first;
    * last = self -> id_upper - 1;

    if ( self -> id_first == self -> id_upper )
        return RC ( rcDB, rcColumn, rcAccessing, rcRange, rcInvalid );
    return 0;
}

/* FindFirstRowId
 */
rc_t KWColumnIdxFindFirstRowId ( const KWColumnIdx * self,
    int64_t * found, int64_t start )
{
    rc_t rc0, rc1;
    KColBlockLoc bloc;
    int64_t best0, best1;

    assert ( self != NULL );
    assert ( found != NULL );

    /* global reject */
    if ( start < self -> id_first || start >= self -> id_upper )
        return RC ( rcDB, rcColumn, rcSelecting, rcRow, rcNotFound );

    /* look in idx0 */
    rc0 = KWColumnIdx0FindFirstRowId ( & self -> idx0, found, start );
    if ( rc0 == 0 )
    {
        if ( * found == start )
            return 0;

        best0 = * found;
        assert ( best0 > start );
    }

    /* look in main index */
    /* KWColumnIdx1LocateFirstRowIdBlob returns the blob containing 'start', if such blob exists, otherwise the next blob (or RC if there's no next) */
    rc1 = KWColumnIdx1LocateFirstRowIdBlob ( & self -> idx1, & bloc, start );
    if ( rc1 != 0 )
    {
        return rc0;
    }
    if ( start >= bloc . start_id )
    {   /* found inside a blob */
        best1 = start;
    }
    else
    {   /* not found; pick the start of the next blob */
        best1 = bloc . start_id;
    }

    if ( rc0 != 0 )
    {
        * found = best1;
        return 0;
    }

    /* found in both - return lesser */

    /* "found" already contains 'best0" */
    assert ( * found == best0 );
    if ( best1 < best0 )
        * found = best1;

    return 0;
}


/* LocateBlob
 *  locate an existing blob
 */
rc_t KWColumnIdxLocateBlob ( const KWColumnIdx *self,
    KColBlobLoc *loc, int64_t first, int64_t upper )
{
    rc_t rc;

    assert ( self != NULL );

    /* convert "last" to "upper" */
    if ( first >= ++ upper )
        return RC ( rcDB, rcColumn, rcSelecting, rcRange, rcInvalid );

    /* global reject */
    if ( first < self -> id_first || upper > self -> id_upper )
        return RC ( rcDB, rcColumn, rcSelecting, rcBlob, rcNotFound );

    /* look in idx0 */
    rc = KWColumnIdx0LocateBlob ( & self -> idx0, loc, first, upper );
    if ( GetRCState ( rc ) == rcNotFound )
    {
        KColBlockLoc bloc;

        /* find block containing range */
        rc = KWColumnIdx1LocateBlock ( & self -> idx1, & bloc, first, upper );
        if ( rc == 0 )
        {
            /* find location in idx2 */
            rc = KWColumnIdx2LocateBlob ( & self -> idx2,
                loc, & bloc, first, upper, self -> idx1 . bswap );
        }
    }

    return rc;
}

/* KWColumnIdxCommit
 *  writes a new blob location to idx0
 *  updates idx1 with header information
 *  this should be the final step in committing a write operation
 */
rc_t KWColumnIdxCommit ( KWColumnIdx *self, KMD5SumFmt *md5,
    const KColBlobLoc *loc, uint32_t commit_freq,
    uint64_t data_eof, size_t pgsize, int32_t checksum )
{
    rc_t idx0rc, rc;
    KColBlobLoc prior;

    assert ( self != NULL );
    assert ( loc != NULL );

    /* check index range for wraparound */
    if ( ( loc -> start_id + loc -> id_range ) <= loc -> start_id )
        return RC ( rcDB, rcColumn, rcCommitting, rcRange, rcInvalid );

    /* journal to idx0 */
    rc = idx0rc =
        KWColumnIdx0Commit ( & self -> idx0, loc, & prior, self -> idx1 . bswap );
    if ( rc == 0 || rc == kdbReindex )
    {
        rc = 0;

        if ( ++ self -> commit_count >= commit_freq )
        {
            rc = KWColumnIdx1WriteHeader ( & self -> idx1,
                data_eof, self -> idx0 . count, self -> idx2 . eof,
                pgsize, checksum );
            if ( rc == 0 )
                self -> commit_count = 0;
        }

        if ( rc == 0 )
        {
            int64_t upper = loc -> start_id + loc -> id_range;

            /* incorporate index into range */
            if ( self -> id_first == self -> id_upper )
            {
                self -> id_first = loc -> start_id;
                self -> id_upper = upper;
            }
            else
            {
                if ( self -> id_first > loc -> start_id )
                    self -> id_first = loc -> start_id;
                if ( self -> id_upper < upper )
                    self -> id_upper = upper;
            }

            assert ( self -> id_first < self -> id_upper );

            return idx0rc;
        }

        /* revert idx0 commit */
        KWColumnIdx0Revert ( & self -> idx0, loc, & prior );
    }

    return rc;
}

rc_t KWColumnIdxCommitDone ( KWColumnIdx * self )
{
    rc_t rc = 0;

    assert ( self != NULL );
    rc = KWColumnIdx1CommitDone ( & self -> idx1 );
    if ( rc == 0 )
	rc = KWColumnIdx0CommitDone ( & self -> idx0 );
    if ( rc == 0 )
	rc = KWColumnIdx2CommitDone ( & self -> idx2 );
    return rc;
}

/* KWColumnIdxRecordIdx0Edges
 *  creates tentative idx1 blocks from idx0 entries
 *  at the block type edges as identified by KWColumnIdx0
 */
typedef struct Idx0EdgeNode Idx0EdgeNode;
struct Idx0EdgeNode
{
    DLNode n;
    KColBlockLocInfo loc;
};

static
void CC Idx0EdgeNodeWhack ( DLNode *n, void *ignore )
{
    free ( n );
}

static
rc_t KWColumnIdxRecordIdx0Edges ( const KColBlockLocInfo *info, void *edges )
{
    Idx0EdgeNode *node;

    assert ( edges != NULL );
    assert ( info != NULL );

    assert ( info -> start_id < info -> end_id );
    assert ( info -> start_pg < info -> end_pg );
    assert ( info -> size != 0 );
    assert ( info -> count != 0 );
    assert ( info -> id_type < 4 );
    assert ( info -> pg_type < 4 );

    assert ( ( ( info -> end_id - info -> start_id ) >> 32 ) == 0 );

    node = malloc ( sizeof * node );
    if ( node == NULL )
        return RC ( rcDB, rcColumn, rcReindexing, rcMemory, rcExhausted );

    node -> loc = * info;

    DLListPushTail ( edges, & node -> n );

    return 0;
}

/* KWColumnIdxCollapseBlocks
 *  scans over blocks, looking for opportunities to
 *  combine them and collapse their number
 */
static
rc_t KWColumnIdxCollapseBlocks ( KWColumnIdx *self, DLList *edges )
{
    uint32_t count;

    do
    {
        Idx0EdgeNode *prior;

        /* merge count */
        count = 0;

        /* apply collapse */
        prior = ( Idx0EdgeNode* ) DLListHead ( edges );
        if ( prior != NULL )
        {
            Idx0EdgeNode *next, *cur;
            for ( cur = ( Idx0EdgeNode* ) DLNodeNext( & prior -> n );
                  cur != NULL; cur = next )
            {
                int64_t cost_left, cost_right;
                KColBlockLocInfo buffer [ 2 ], *info;

                /* grab next guy right away
                   now we have prior, current, and next */
                next = ( Idx0EdgeNode* ) DLNodeNext ( & cur -> n );

                /* calculate a cost of merging prior and current */
                buffer [ 0 ] = prior -> loc;
                cost_left = KColBlockLocInfoMerge ( info = & buffer [ 0 ], & cur -> loc );

                /* if there would be some advantage to the merge */
                if ( cost_left <= 0 )
                {
                    /* calculate cost of merging current with next if there */
                    if ( next != NULL )
                    {
                        buffer [ 1 ] = cur -> loc;
                        cost_right = KColBlockLocInfoMerge
                            ( info = & buffer [ 1 ], & next -> loc );

                        /* if prev merge beats next,
                           set info to previous block value */
                        if ( cost_left <= cost_right )
                            info = & buffer [ 0 ];
                        else
                        {
                            /* otherwise shift our window
                               and keep next block value */
                            prior = cur;
                            cur = next;
                            next = ( Idx0EdgeNode* ) DLNodeNext ( & next -> n );
                        }
                    }

                    /* merge prior with cur */
                    prior -> loc = * info;
                    DLListUnlink ( edges, & cur -> n );
                    Idx0EdgeNodeWhack ( & cur -> n, NULL );
                    ++ count;
                    continue;
                }

                /* leave block alone */
                prior = cur;
            }
        }
    }
    while ( count != 0 );

    return 0;
}


/* KWColumnIdxWriteNewBlock
 *  writes each block into idx1 and idx2
 */
typedef struct WriteNewBlockData WriteNewBlockData;
struct WriteNewBlockData
{
    uint64_t data_eof;
    KWColumnIdx *idx;
    size_t pgsize;
    uint32_t commit_freq;
    int32_t checksum;
    rc_t rc;
    bool bswap;
};

static
bool CC KWColumnIdxWriteNewBlock ( DLNode *n, void *data )
{
    WriteNewBlockData *pb = data;
    const Idx0EdgeNode *node = ( const Idx0EdgeNode* ) n;
    KWColumnIdx *self = pb -> idx;

    KColBlockLoc bloc;
    KColWIdxBlock iblk;

    /* create and map a block of memory */
    pb -> rc = KColWIdxBlockInit ( & iblk, & node -> loc );
    if ( pb -> rc == 0 )
    {
        size_t to_write;

        /* tell idx0 to fill in block */
        KWColumnIdx0TranscribeBlocks ( & self -> idx0,
            node -> loc . start_id, node -> loc . end_id, & iblk );
        if ( iblk . idx != node -> loc . count )
            pb -> rc = RC ( rcDB, rcColumn, rcCommitting, rcNoObj, rcUnknown );
        else
        {
            /* compress block */
            pb -> rc = KColWIdxBlockCompress ( & iblk,
                pb -> bswap, & node -> loc, & bloc, & to_write );
            if ( pb -> rc == 0 )
            {
                /* write block appropriately */
                pb -> rc = KWColumnIdx2Write ( & self -> idx2,
                    & bloc . pg, KColWIdxBlockPersistPtr ( & iblk, & bloc ), to_write );
            }
        }

        KColWIdxBlockWhack ( & iblk );

        if ( pb -> rc == 0 )
        {
            /* write idx1 block location information */
            pb -> rc = KWColumnIdx1Commit ( & self -> idx1, & bloc );
            if ( pb -> rc == 0 )
            {
                /* update the header */
                if ( ++ self -> commit_count >= pb -> commit_freq )
                {
                    pb -> rc = KWColumnIdx1WriteHeader ( & self -> idx1,
                        pb -> data_eof, self -> idx0 . count,
                        self -> idx2 . eof + bloc . u . blk . size,
                        pb -> pgsize, pb -> checksum );

                    if ( pb -> rc == 0 )
                        self -> commit_count = 0;
                }
                if ( pb -> rc == 0 )
                {
                    /* done writing block */
                    KWColumnIdx2Commit ( & self -> idx2, to_write );
                    return false;
                }

                /* revert idx1 */
                KWColumnIdx1Revert ( & self -> idx1, bloc . start_id, bloc . id_range );
            }
        }
    }

    return true;
}

/* KWColumnIdxFixIdx1
 */
static
bool CC KWColumnIdxFixIdx1 ( DLNode *n, void *data )
{
    const Idx0EdgeNode *node = ( const Idx0EdgeNode* ) n;
    return KWColumnIdx1Revert ( data, node -> loc . start_id,
        ( uint32_t ) ( node -> loc . end_id  - node -> loc . start_id ) );
}

/* KWColumnIdxReindex
 */
rc_t KWColumnIdxReindex ( KWColumnIdx *self, KMD5SumFmt *md5,
    uint32_t commit_freq, uint64_t data_eof, size_t pgsize, int32_t checksum )
{
    rc_t rc;
    DLList edges;

    assert ( self != NULL );
    assert ( pgsize > 0 );

    DLListInit ( & edges );

    /* map the new entries */
    rc = KWColumnIdx0DefineBlocks ( & self -> idx0,
        KWColumnIdxRecordIdx0Edges, & edges, pgsize );
    if ( rc == 0 )
    {
        rc = KWColumnIdxCollapseBlocks ( self, & edges );
        if ( rc == 0 )
        {
            /* preserve for restoration */
            uint64_t idx2_eof = self -> idx2 . eof;
            size_t count = self -> idx1 . count;

            WriteNewBlockData pb;
            pb . data_eof = data_eof;
            pb . idx = self;
            pb . pgsize = pgsize;
            pb . commit_freq = commit_freq;
            pb . checksum = checksum;
            pb . rc = 0;
            pb . bswap = self -> idx1 . bswap;

            /* write new blocks */
            DLListDoUntil ( & edges, false, KWColumnIdxWriteNewBlock, & pb );
            if ( ( rc = pb . rc ) == 0 )
            {
                /* idx2 is correct,
                   idx1 has proper count and bst,
                   idx0 needs to whack bst and file,
                   idx1 needs to update header */
                rc = KWColumnIdx1WriteHeader ( & self -> idx1,
                    data_eof, 0, self -> idx2 . eof,
                    pgsize, checksum );
                if ( rc == 0 )
                {
                    self -> commit_count = 0;
                    KWColumnIdx0Truncate ( & self -> idx0 );
                    DLListWhack ( & edges, Idx0EdgeNodeWhack, NULL );
                    return 0;
                }
            }

            /* restore idx2 eof */
            self -> idx2 . eof = idx2_eof;

            /* restore idx1 count and fix bst */
            DLListDoUntil ( & edges, false, KWColumnIdxFixIdx1, & self -> idx1 );
            self -> idx1 . count = count;
        }
    }

    DLListWhack ( & edges, Idx0EdgeNodeWhack, NULL );

    return rc;
}
