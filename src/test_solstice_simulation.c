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
#include <rsys/double2.h>

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

enum global_result_type {
  GLOBAL_SHADOW,
  GLOBAL_MISSING,
  GLOBAL_COS,
  GLOBAL_RESULTS_COUNT__
};

enum receiver_result_type {
  FIRST_RECEIVER_RESULT,
  FRONT_INTEGRATED_ABSORBED_IRRADIANCE = FIRST_RECEIVER_RESULT,
  FRONT_INTEGRATED_IRRADIANCE,
  FRONT_REFLECTIVITY_LOSS,
  FRONT_ABSORPTIVITY_LOSS,
  FRONT_EFFICIENCY,
  BACK_INTEGRATED_ABSORBED_IRRADIANCE,
  BACK_INTEGRATED_IRRADIANCE,
  BACK_REFLECTIVITY_LOSS,
  BACK_ABSORPTIVITY_LOSS,
  BACK_EFFICIENCY,
  RECEIVER_RESULTS_COUNT__
};

enum primary_result_type {
  FIRST_PRIMARY_RESULT,
  PRIMARY_SHADOW = FIRST_PRIMARY_RESULT,
  PRIMARY_RESULTS_COUNT__
};

struct counts {
  unsigned long global, receiver, primary, realisation, failed;
};

static int
counts_ok(const struct counts* ref, const struct counts* c)
{
  CHECK(ref->global, GLOBAL_RESULTS_COUNT__);
  CHECK(c->global, GLOBAL_RESULTS_COUNT__);
  return ref->receiver == c->receiver
    && ref->primary == c->primary
    && ref->failed >= c->failed;
}

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

static int
get_angles_and_counts
  (FILE* file,
   double angles[2],
   struct counts* counts)
{
  char line[MAX_LINE_LEN];
  int r;

  NCHECK(file, NULL);
  NCHECK(angles, NULL);
  NCHECK(counts, NULL);

  /* Get sun dir */
  r = read_line(line, sizeof(line), file);
  if (!r) {
    CHECK(feof(file), 1);
    return 0;
  }
  CHECK(IS_NEW_BLOCK(line, sundir_header), 1);
  CHECK(
    sscanf(line+strlen(sundir_header), "%lg%lg", &angles[0], &angles[1]),
    2);

  /* Get counts */
  CHECK(read_line(line, sizeof(line), file), 1);
  CHECK(
    sscanf(line,
      "%lu%lu%lu%lu%lu",
      &counts->global, &counts->receiver, &counts->primary,
      &counts->realisation, &counts->failed),
    5);
  return 1;
}

static FINLINE void
read_global(FILE* file, char name [], double* E, double* SE)
{
  char line[MAX_LINE_LEN];
  CHECK(read_line(line, sizeof(line), file), 1);
  CHECK(
    sscanf(line, "%lg %lg # %s", E, SE, name),
    3);
}

static void
read_recv(FILE* file, char name[], double E[], double SE[])
{
  char line[MAX_LINE_LEN];

  NCHECK(file, NULL);
  NCHECK(name, NULL);
  NCHECK(E, NULL);
  NCHECK(SE, NULL);

  CHECK(read_line(line, sizeof(line), file), 1);
  CHECK(
    sscanf(line,
      "%s %*lu  "
      "FRONT: %lg %lg   %lg %lg   %lg %lg   %lg %lg   %lg %lg  "
      " BACK: %lg %lg   %lg %lg   %lg %lg   %lg %lg   %lg %lg",
      name, /* ID */
      &E[FRONT_INTEGRATED_ABSORBED_IRRADIANCE],
      &SE[FRONT_INTEGRATED_ABSORBED_IRRADIANCE],
      &E[FRONT_INTEGRATED_IRRADIANCE], &SE[FRONT_INTEGRATED_IRRADIANCE],
      &E[FRONT_REFLECTIVITY_LOSS], &SE[FRONT_REFLECTIVITY_LOSS],
      &E[FRONT_ABSORPTIVITY_LOSS], &SE[FRONT_ABSORPTIVITY_LOSS],
      &E[FRONT_EFFICIENCY], &SE[FRONT_EFFICIENCY],
      &E[BACK_INTEGRATED_ABSORBED_IRRADIANCE],
      &SE[BACK_INTEGRATED_ABSORBED_IRRADIANCE],
      &E[BACK_INTEGRATED_IRRADIANCE], &SE[BACK_INTEGRATED_IRRADIANCE],
      &E[BACK_REFLECTIVITY_LOSS], &SE[BACK_REFLECTIVITY_LOSS],
      &E[BACK_ABSORPTIVITY_LOSS], &SE[BACK_ABSORPTIVITY_LOSS],
      &E[BACK_EFFICIENCY], &SE[BACK_EFFICIENCY]),
    2 * RECEIVER_RESULTS_COUNT__ + 1);
}

static FINLINE void
read_primary
  (FILE* file, char name[], double* area, double* cos, double E[], double SE[])
{
  char line[MAX_LINE_LEN];
  CHECK(read_line(line, sizeof(line), file), 1);
  CHECK(
    sscanf(line,
      "%s %*lu   "
      "%lg %lg %*lu   "
      "%lg %lg\n",
      name, /* ID */
      area, cos, /* count */
      &E[PRIMARY_SHADOW], &SE[PRIMARY_SHADOW]),
    2 * PRIMARY_RESULTS_COUNT__ + 3);
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
check_1_reference
  (FILE* ref_file,
   FILE* test_file,
   const struct counts* counts)
{
  unsigned n;

  NCHECK(ref_file, NULL);
  NCHECK(test_file, NULL);
  NCHECK(counts, NULL);

  /* both files' pointer are just past the new bloc header */

  for (n = 0; n < counts->global; n++) {
    char ref_global_name[MAX_LINE_LEN], test_global_name[MAX_LINE_LEN];
    double reference_E, reference_SE, test_E, test_SE;

    read_global(ref_file, ref_global_name, &reference_E, &reference_SE);
    read_global(test_file, test_global_name, &test_E, &test_SE);
    CHECK(strcmp(ref_global_name, test_global_name), 0);
    CHECK(is_compatible_with(reference_E, reference_SE, test_E, test_SE), 1);
  }
  for (n = 0; n < counts->receiver; n++) {
    char ref_rcv_name[MAX_LINE_LEN], test_rcv_name[MAX_LINE_LEN];
    double reference_E[RECEIVER_RESULTS_COUNT__];
    double reference_SE[RECEIVER_RESULTS_COUNT__];
    double test_E[RECEIVER_RESULTS_COUNT__];
    double test_SE[RECEIVER_RESULTS_COUNT__];
    enum receiver_result_type r;

    read_recv(ref_file, ref_rcv_name, reference_E, reference_SE);
    read_recv(test_file, test_rcv_name, test_E, test_SE);
    CHECK(strcmp(ref_rcv_name, test_rcv_name), 0);
    FOR_EACH(r, FIRST_RECEIVER_RESULT, RECEIVER_RESULTS_COUNT__) {
      CHECK(is_compatible_with
        (reference_E[r], reference_SE[r], test_E[r], test_SE[r]), 1);
    }
  }
  for (n = 0; n < counts->primary; n++) {
    char ref_prim_name[MAX_LINE_LEN], test_prim_name[MAX_LINE_LEN];
    double reference_E[PRIMARY_RESULTS_COUNT__];
    double reference_SE[PRIMARY_RESULTS_COUNT__];
    double test_E[PRIMARY_RESULTS_COUNT__];
    double test_SE[PRIMARY_RESULTS_COUNT__];
    double ref_area, ref_cos;
    double test_area, test_cos;
    enum primary_result_type r;

    read_primary(ref_file, ref_prim_name, &ref_area, &ref_cos, reference_E, reference_SE);
    read_primary(test_file, test_prim_name, &test_area, &test_cos, test_E, test_SE);
    CHECK(is_compatible_with(ref_area, 0, test_area, 0), 1);
    CHECK(is_compatible_with(ref_cos, 0, test_cos, 0), 1);
    CHECK(strcmp(ref_prim_name, test_prim_name), 0);
    FOR_EACH(r, FIRST_RECEIVER_RESULT, PRIMARY_RESULTS_COUNT__) {
      CHECK(is_compatible_with
      (reference_E[r], reference_SE[r], test_E[r], test_SE[r]), 1);
    }
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
  struct counts ref_counts, test_counts;
  int n;
  ASSERT(base_name);

  n = snprintf(ref_file_name, sizeof(ref_file_name), "%s%s.ref", dir, base_name);
  CHECK((size_t)n < sizeof(ref_file_name), 1);

  ref_file = fopen(ref_file_name, "r");
  NCHECK(ref_file, NULL);

  while(!feof(ref_file)) {
    char cmd[512];
    char test_file_name[128];
    double ref_sun_angles[2], test_sun_angles[2];
    FILE* test_file = NULL;
    int fd = -1;

    if (!get_angles_and_counts(ref_file, ref_sun_angles, &ref_counts))
      break; /* EOF */

    fd = create_tmp_file(test_file_name, sizeof(test_file_name));
    test_file = fdopen(fd, "r");
    NCHECK(test_file, NULL);

    n = snprintf(cmd, sizeof(cmd),
      "%s -o %s -f -D %g,%g -n %lu -R %s%s_receiver.yaml %s%s.yaml",
      binary, test_file_name, SPLIT2(ref_sun_angles), ref_counts.realisation,
      dir, base_name, dir, base_name);
    CHECK((unsigned)n < sizeof(cmd), 1);

    CHECK(system(cmd), RES_OK);

    get_angles_and_counts(test_file, test_sun_angles, &test_counts);
    CHECK(d2_eq(ref_sun_angles, test_sun_angles), 1);
    CHECK(counts_ok(&ref_counts, &test_counts), 1);
    check_1_reference(ref_file, test_file, &ref_counts);

    fclose(test_file);
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

