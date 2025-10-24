/*===------- llvm/Config/llvm-config.h - llvm configuration -------*- C -*-===*/
/*                                                                            */
/* Part of the LLVM Project, under the Apache License v2.0 with LLVM          */
/* Exceptions.                                                                */
/* See https://llvm.org/LICENSE.txt for license information.                  */
/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception                    */
/*                                                                            */
/*===----------------------------------------------------------------------===*/

/* This file enumerates variables from the LLVM configuration so that they
can be in exported headers and won't override package specific directives.
This is a C header that can be included in the llvm-c headers. */

#ifndef LLVM_CONFIG_H
#define LLVM_CONFIG_H

#define PACKAGE_NAME "LLVM"
#define PACKAGE_VERSION 666
#define BUG_REPORT_URL "NONE"

#define LLVM_ENABLE_CRASH_DUMPS 1
#define LLVM_WINDOWS_PREFER_FORWARD_SLASH 0

/* Define if threads enabled */
#cmakedefine01 LLVM_ENABLE_THREADS

/* Define if this is Unixish platform */
#cmakedefine LLVM_ON_UNIX

/* Define to 1 if you have the <sysexits.h> header file. */
#cmakedefine01 HAVE_SYSEXITS_H

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H ${HAVE_UNISTD_H}

/* Define to 1 if you have the <sys/mman.h> header file. */
#cmakedefine HAVE_SYS_MMAN_H ${HAVE_SYS_MMAN_H}

/* Define to 1 if you have the `getpagesize' function. */
#cmakedefine HAVE_GETPAGESIZE ${HAVE_GETPAGESIZE}

/* Define to 1 if you have the <pthread.h> header file. */
#cmakedefine HAVE_PTHREAD_H ${HAVE_PTHREAD_H}

/* Have pthread_rwlock_init */
#cmakedefine HAVE_PTHREAD_RWLOCK_INIT ${HAVE_PTHREAD_RWLOCK_INIT}

/* Define to 1 if you have the `futimens' function. */
#cmakedefine HAVE_FUTIMENS ${HAVE_FUTIMENS}

#endif