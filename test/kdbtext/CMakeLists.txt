# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

# if( WIN32 )
#     set( ADDITIONAL_LIBS Crypt32 )
# 	set( TMPDIR ./data)
# else()
#     set( ADDITIONAL_LIBS "" )
# 	set( TMPDIR /tmp )
# endif()

add_compile_definitions( __mod__="test/kdbtext" )

set( LIBS ${COMMON_LIBS_READ};kdbtext )
AddExecutableTest( Test_KDBText_Path        "test-path"     "${LIBS}" )
AddExecutableTest( Test_KDBText_Metadata    "test-metadata" "${LIBS}" )
AddExecutableTest( Test_KDBText_Index       "test-index"    "${LIBS}" )
AddExecutableTest( Test_KDBText_Column      "test-column"   "${LIBS}" )
AddExecutableTest( Test_KDBText_Table       "test-table"    "${LIBS}" )
AddExecutableTest( Test_KDBText_Database    "test-database" "${LIBS}" )
AddExecutableTest( Test_KDBText_Manager     "test-manager"  "${LIBS}" )
AddExecutableTest( Test_KDBText_Blob        "test-blob"     "${LIBS}" )
AddExecutableTest( Test_KDBText_Metanode    "test-metanode" "${LIBS}" )
