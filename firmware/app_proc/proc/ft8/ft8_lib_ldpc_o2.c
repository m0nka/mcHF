/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_lib_ldpc_o2.c                                              **
**  Description:	ft8_lib ft8/ldpc.c built at -O2                                **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The project builds -O0. The FT8 decode path (sync search, LLR extraction,
// LDPC) is the one place where that decides whether a slot decodes in time,
// so this file compiles ft8_lib's ldpc.c at -O2 in place of the original,
// which is dropped from proj/.project. Keeps the optimisation out of the
// IDE per-file settings and leaves the submodule untouched.
//
#pragma GCC optimize ("O2")

#include "ft8/ldpc.c"
