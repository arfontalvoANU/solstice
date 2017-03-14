/* Copyright (C) CNRS 2016-2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */

#define _POSIX_C_SOURCE 200809L /* mkstemp support */

#include <rsys/rsys.h>
#include <rsys/math.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef COMPILER_CL
  /* Wrap POSIX functions and constants */
  #include <io.h>
  #include <fcntl.h>
  #include <sys/stat.h>
  #define fdopen _fdopen
  #define open _open
  #define mktemp _mktemp

  /* mkstemp extracted from libc/sysdeps/posix/tempname.c.  Copyright
  (C) 1991-1999, 2000, 2001, 2006 Free Software Foundation, Inc.

  The GNU C Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version. */

  static const char letters [] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  /* Generate a temporary file name based on TMPL.  TMPL must match the
  rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
  does not exist at the time of the call to mkstemp.  TMPL is
  overwritten with the result.  */
  int
  mkstemp(char *tmpl)
  {
    size_t len;
    char *XXXXXX;
    static unsigned long long value;
    unsigned long long random_time_bits;
    unsigned int count;
    int fd = -1;
    int save_errno = errno;

    /* A lower bound on the number of temporary files to attempt to
    generate.  The maximum total number of temporary file names that
    can exist for a given template is 62**6.  It should never be
    necessary to try all these combinations.  Instead if a reasonable
    number of names is tried (we define reasonable as 62**3) fail to
    give the system administrator the chance to remove the problems. */
    #define ATTEMPTS_MIN (62 * 62 * 62)

    /* The number of times to attempt to generate a temporary file.  To
    conform to POSIX, this must be no smaller than TMP_MAX. */
    #if ATTEMPTS_MIN < TMP_MAX
      unsigned int attempts = TMP_MAX;
    #else
      unsigned int attempts = ATTEMPTS_MIN;
    #endif

    len = strlen(tmpl);
    if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX")) {
      errno = EINVAL;
      return -1;
    }

    /* This is where the Xs start. */
    XXXXXX = &tmpl[len - 6];

    /* Get some more or less random data. */
    {
      SYSTEMTIME stNow;
      FILETIME ftNow;

      /* get system time */
      GetSystemTime(&stNow);
      stNow.wMilliseconds = 500;
      if (!SystemTimeToFileTime(&stNow, &ftNow)) {
        errno = -1;
        return -1;
      }

      random_time_bits = (((unsigned long long)ftNow.dwHighDateTime << 32)
        | (unsigned long long)ftNow.dwLowDateTime);
    }
    value += random_time_bits ^ (unsigned long long)GetCurrentThreadId();

    for (count = 0; count < attempts; value += 7777, ++count) {
      unsigned long long v = value;

      /* Fill in the random bits.  */
      XXXXXX[0] = letters[v % 62];
      v /= 62;
      XXXXXX[1] = letters[v % 62];
      v /= 62;
      XXXXXX[2] = letters[v % 62];
      v /= 62;
      XXXXXX[3] = letters[v % 62];
      v /= 62;
      XXXXXX[4] = letters[v % 62];
      v /= 62;
      XXXXXX[5] = letters[v % 62];
  
      fd = open(tmpl, O_RDWR|O_CREAT|O_EXCL, S_IREAD|S_IWRITE);
      if (fd >= 0) {
        errno = save_errno;
        return fd;
      }
      else if (errno != EEXIST)
        return -1;
    }

    /* We got out of the loop because we ran out of combinations to try. */
    errno = EEXIST;
    return -1;
  }
#endif

enum side {
  FRONT,
  BACK
};

enum result_type {
  FIRST_RESULT, 
  FRONT_INTEGRATED_ABSORBED_IRRADIANCE = FIRST_RESULT,
  FRONT_INTEGRATED_IRRADIANCE,
  FRONT_REFLECTIVITY_LOSS,
  FRONT_ABSORPTIVITY_LOSS,
  FRONT_EFFICIENCY,
  BACK_INTEGRATED_ABSORBED_IRRADIANCE,
  BACK_INTEGRATED_IRRADIANCE,
  BACK_REFLECTIVITY_LOSS,
  BACK_ABSORPTIVITY_LOSS,
  BACK_EFFICIENCY,
  MAX_RESULTS_COUNT__
};

#define MAX_LINE_LEN 2048

static const char
sundir_header [] = "#--- Sun direction:";

#define IS_NEW_BLOCK(Line, Header) (!strncmp((Line), (Header), strlen(Header)))

static int
read_line(char* line, size_t max_line_len, FILE* stream)
{
  ASSERT(stream && line && max_line_len);
  line = fgets(line, (int)max_line_len, stream);
  if(!line) return 0;
  CHECK(strlen(line) + 1 < max_line_len, 1);
  return 1;
}

static void
get_angles_and_counts
  (FILE* ref_file,
   double angles[2],
   unsigned long* recv_count,
   unsigned long* primary_count,
   unsigned long* realisation_count,
   unsigned long* failed_count)
{
  char line[MAX_LINE_LEN];
  int n;

  NCHECK(ref_file, NULL);
  NCHECK(angles, NULL);
  NCHECK(recv_count, NULL);
  NCHECK(primary_count, NULL);
  NCHECK(realisation_count, NULL);
  NCHECK(failed_count, NULL);

  /* Get sun dir */
  CHECK(read_line(line, sizeof(line), ref_file), 1);
  CHECK(IS_NEW_BLOCK(line, sundir_header), 1);
  n = sscanf(line+strlen(sundir_header), "%lg%lg", &angles[0], &angles[1]);
  CHECK(n, 2);

  /* Get counts */
  CHECK(read_line(line, sizeof(line), ref_file), 1);
  n = sscanf(line, "%lu%lu%lu%lu", recv_count, primary_count, realisation_count, failed_count);
  CHECK(n, 4);
}

static void
read_recv(const char* line, char name[], double E[], double SE[])
{
  int n;

  NCHECK(line, NULL);
  NCHECK(name, NULL);
  NCHECK(E, NULL);
  NCHECK(SE, NULL);

  n = sscanf(line,
     "%s %*lu  "
     "FRONT: %lg %lg   %lg %lg   %lg %lg   %lg %lg   %lg %lg  "
     " BACK: %lg %lg   %lg %lg   %lg %lg   %lg %lg   %lg %lg",
     name,
     &E[FRONT_INTEGRATED_ABSORBED_IRRADIANCE], &SE[FRONT_INTEGRATED_ABSORBED_IRRADIANCE],
     &E[FRONT_INTEGRATED_IRRADIANCE], &SE[FRONT_INTEGRATED_IRRADIANCE],
     &E[FRONT_REFLECTIVITY_LOSS], &SE[FRONT_REFLECTIVITY_LOSS],
     &E[FRONT_ABSORPTIVITY_LOSS], &SE[FRONT_ABSORPTIVITY_LOSS],
     &E[FRONT_EFFICIENCY], &SE[FRONT_EFFICIENCY],
     &E[BACK_INTEGRATED_ABSORBED_IRRADIANCE], &SE[BACK_INTEGRATED_ABSORBED_IRRADIANCE],
     &E[BACK_INTEGRATED_IRRADIANCE], &SE[BACK_INTEGRATED_IRRADIANCE],
     &E[BACK_REFLECTIVITY_LOSS], &SE[BACK_REFLECTIVITY_LOSS],
     &E[BACK_ABSORPTIVITY_LOSS], &SE[BACK_ABSORPTIVITY_LOSS],
     &E[BACK_EFFICIENCY], &SE[BACK_EFFICIENCY]);

  CHECK(n,  2*MAX_RESULTS_COUNT__+1);
}

#define POSITIVE_OR_M_ONE(x) ((x) == -1 || (x) >= 0)

static FINLINE int
is_compatible_with
  (const double ref_E,
   const double ref_SE,
   const double test_E,
   const double test_SE)
{
  double SE;

  CHECK(POSITIVE_OR_M_ONE(ref_E), 1);
  CHECK(POSITIVE_OR_M_ONE(ref_SE), 1);
  CHECK(POSITIVE_OR_M_ONE(test_E), 1);
  CHECK(POSITIVE_OR_M_ONE(test_SE), 1);

  if(ref_E == -1) {
    CHECK(ref_SE, -1);
    return (test_E == -1 && test_SE == -1);
  }

  NCHECK(ref_SE, -1);
  SE = ref_SE > 0 ? 2 * ref_SE : (ref_E > 0 ? ref_E * 1e-6 : 1e-6);
  return (fabs(ref_E - test_E) <= SE && test_SE <= SE);
}

static void
check_1_receiver
  (FILE* test_file,
   const char* rcv_name,
   const double* reference_E,
   const double* reference_SE)
{
  double a[2];
  unsigned long c1, c2, c3, c4;
  int found = 0;

  NCHECK(test_file, NULL);
  NCHECK(rcv_name, NULL);
  NCHECK(reference_E, NULL);
  NCHECK(reference_SE, NULL);

  get_angles_and_counts(test_file, a, &c1, &c2, &c3, &c4); /* Skip headers */

  while(!feof(test_file) && !found) {
    char line[MAX_LINE_LEN];
    char test_rcv_name[MAX_LINE_LEN];
    double test_E[MAX_RESULTS_COUNT__], test_SE[MAX_RESULTS_COUNT__];
    double v, s;
    size_t nb;
    enum result_type r;

    CHECK(read_line(line, sizeof(line), test_file), 1);

    nb = sscanf(line, "%lg %lg # %s", &v, &s, test_rcv_name);
    CHECK(nb == 0 || nb == 3, 1);

    if(nb == 3) continue; /* skip global */
    read_recv(line, test_rcv_name, test_E, test_SE);
    if(strcmp(rcv_name, test_rcv_name)) continue;

    FOR_EACH(r, FIRST_RESULT, MAX_RESULTS_COUNT__) {
      CHECK(is_compatible_with
        (reference_E[r], reference_SE[r], test_E[r], test_SE[r]), 1);
    }
    found = 1;
  }
  CHECK(found, 1);
}

static void
check_1_global
  (FILE* test_file,
   const double reference_E,
   const double reference_SE,
   const char* ref_name)
{
  char line[MAX_LINE_LEN], test_name[MAX_LINE_LEN];
  double a[2];
  unsigned long c1, c2, c3, c4;
  int nb;
  double test_E, test_SE;

  get_angles_and_counts(test_file, a, &c1, &c2, &c3, &c4);

  do {
    CHECK(read_line(line, sizeof(line), test_file), 1);
    nb = sscanf(line, "%lg %lg # %s", &test_E, &test_SE, test_name);
  } while (nb != 3 || strcmp(ref_name, test_name));

  CHECK(strcmp(ref_name, test_name), 0);
  CHECK(is_compatible_with(reference_E, reference_SE, test_E, test_SE), 1);
}

static void
check_references(FILE* ref_file, FILE* test_file)
{
  char line[MAX_LINE_LEN], g_name[MAX_LINE_LEN];
  unsigned nb_global = 0;
  fpos_t pos;

  NCHECK(ref_file, NULL);
  NCHECK(test_file, NULL);

  CHECK(fgetpos(ref_file, &pos), 0);
  while(read_line(line, sizeof(line), ref_file)) {
    double val, std;
    int nb = 0;

    if(IS_NEW_BLOCK(line, sundir_header)) {
      /* Keep the header as a part of the following block */
      CHECK(fsetpos(ref_file, &pos), 0);
      break;
    }

    nb = sscanf(line, "%lg %lg # %s", &val, &std, g_name);
    CHECK(nb == 0 || nb == 3, 1);

    rewind(test_file);
    if(nb != 0) {
      check_1_global(test_file, val, std, g_name);
      nb_global++;
    } else {
      char ref_name[MAX_LINE_LEN];
      double reference_E[MAX_RESULTS_COUNT__];
      double reference_SE[MAX_RESULTS_COUNT__];
      read_recv(line, ref_name, reference_E, reference_SE);
      check_1_receiver(test_file, ref_name, reference_E, reference_SE);
    }

    CHECK(fgetpos(ref_file, &pos), 0);
  }
}

static FINLINE int
create_tmp_file(char* name, const size_t max_sizeof_name)
{
  const char* template = "solstice_tmp_file_XXXXXX";
  int fd;
  NCHECK(name, NULL);
  CHECK(strlen(template)+1 <= max_sizeof_name-1, 1);
  strcpy(name, template);
  fd = mkstemp(name);
  NCHECK(fd, -1);
  return fd;
}

static void
do_check(const char* binary, const char* dir, const char* base_name)
{
  char ref_file_name[128];
  FILE* ref_file;
  unsigned long c1, c2, realisation_count, c4;
  int n;
  ASSERT(base_name);

  n = snprintf(ref_file_name, sizeof(ref_file_name), "%s%s.ref", dir, base_name);
  CHECK((size_t)n < sizeof(ref_file_name), 1);

  ref_file = fopen(ref_file_name, "r");
  NCHECK(ref_file, NULL);

  while(!feof(ref_file)) {
    char cmd[512];
    char test_file_name[128];
    double sun_angles[2];
    FILE* fp = NULL;
    int fd = -1;

    get_angles_and_counts(ref_file, sun_angles, &c1, &c2, &realisation_count, &c4);

    fd = create_tmp_file(test_file_name, sizeof(test_file_name));
    fp = fdopen(fd, "r");
    NCHECK(fp, NULL);

    n = snprintf(cmd, sizeof(cmd),
      "%s -o %s -f -D %g,%g -n %lu -R %s%s_receiver.yaml %s%s.yaml",
      binary, test_file_name, SPLIT2(sun_angles), realisation_count,
      dir, base_name, dir, base_name);
    CHECK((unsigned)n < sizeof(cmd), 1);

    CHECK(system(cmd), RES_OK);

    check_references(ref_file, fp);

    fclose(fp);
    remove(test_file_name);
  }
}

int
main(int argc, char** argv)
{
  int err = 0;

  if(argc != 4) {
    printf("Usage: %s <solstice-binary> <file-path> <file-base-name>\n", argv[0]);
    goto error;
  }

  do_check(argv[1], argv[2], argv[3]);

exit:
  return err;
error:
  err = 1;
  goto exit;
}

