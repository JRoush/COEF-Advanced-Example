/*
    Version info for this plugin
*/
#pragma once

//  General version control for OBME
const int MAJOR_VERSION    = 0x01;     // Major version
const int MINOR_VERSION    = 0x00;     // Minor version (release)
const int BETA_VERSION     = 0x00;     // Minor version (alpha/beta), FF is post-beta
const int COSAVE_VERSION   = 0x00;     // Cosave record version

// macros for combining version info into one 32bit number.  Pass zero for overall build version
#define RECORD_VERSION(recordT)    ((MAJOR_VERSION << 0x18) | (MINOR_VERSION << 0x10) | \
                                        (BETA_VERSION << 0x08) | recordT)

#define VERSION_VANILLA_OBLIVION        0x00000000      // indicates an object initialized by vanilla code