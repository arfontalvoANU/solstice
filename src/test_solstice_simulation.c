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
  #define fdopen _fdopen
#endif

enum side {
  FRONT,
  BACK
};

enum result_type {
  FRONT_INTEGRATED_IRRADIANCE,
  BACK_INTEGRATED_IRRADIANCE,
  FRONT_REFLECTIVITY_LOSS,
  BACK_REFLECTIVITY_LOSS,
  FRONT_ABSORPTIVITY_LOSS,
  BACK_ABSORPTIVITY_LOSS,
  FRONT_COS_LOSS,
  BACK_COS_LOSS,
  FRONT_EFFICIENCY,
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
get_dir_and_counts
  (FILE* ref_file,
   double angles[2],
   unsigned long* recv_count,
   unsigned long* realisation_count)
{
  char line[MAX_LINE_LEN];
  int n;

  NCHECK(ref_file, NULL);
  NCHECK(angles, NULL);
  NCHECK(recv_count, NULL);
  NCHECK(realisation_count, NULL);

  /* Get sun dir */
  CHECK(read_line(line, sizeof(line), ref_file), 1);
  CHECK(IS_NEW_BLOCK(line, sundir_header), 1);
  n = sscanf(line+strlen(sundir_header), "%lg%lg", &angles[0], &angles[1]);
  CHECK(n, 2);

  /* Get #receivers and #realisations */
  CHECK(read_line(line, sizeof(line), ref_file), 1);
  n = sscanf(line, "%lu%lu", recv_count, realisation_count);
  CHECK(n, 2);
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
    "%s%*u%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg",
     name,
     &E[FRONT_INTEGRATED_IRRADIANCE], &SE[FRONT_INTEGRATED_IRRADIANCE],
     &E[BACK_INTEGRATED_IRRADIANCE], &SE[BACK_INTEGRATED_IRRADIANCE],
     &E[FRONT_REFLECTIVITY_LOSS], &SE[FRONT_REFLECTIVITY_LOSS],
     &E[BACK_REFLECTIVITY_LOSS], &SE[BACK_REFLECTIVITY_LOSS],
     &E[FRONT_ABSORPTIVITY_LOSS], &SE[FRONT_ABSORPTIVITY_LOSS],
     &E[BACK_ABSORPTIVITY_LOSS], &SE[BACK_ABSORPTIVITY_LOSS],
     &E[FRONT_COS_LOSS], &SE[FRONT_COS_LOSS],
     &E[BACK_COS_LOSS], &SE[BACK_COS_LOSS],
     &E[FRONT_EFFICIENCY], &SE[FRONT_EFFICIENCY],
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
check_1_reference
  (FILE* tested_file,
   const char* rcv_name,
   const double* reference_E,
   const double* reference_SE)
{
  double a[2];
  size_t c1, c2;
  int found = 0;

  NCHECK(tested_file, NULL);
  NCHECK(rcv_name, NULL);
  NCHECK(reference_E, NULL);
  NCHECK(reference_SE, NULL);

  get_dir_and_counts(tested_file, a, &c1, &c2); /* Skip headers */

  while(!feof(tested_file) && !found) {
    char line[MAX_LINE_LEN];
    char tested_rcv_name[MAX_LINE_LEN];
    double tested_E[MAX_RESULTS_COUNT__], tested_SE[MAX_RESULTS_COUNT__];
    enum result_type r;

    CHECK(read_line(line, sizeof(line), tested_file), 1);

    read_recv(line, tested_rcv_name, tested_E, tested_SE);
    if(strcmp(rcv_name, tested_rcv_name)) continue;

    FOR_EACH(r, FRONT_INTEGRATED_IRRADIANCE, MAX_RESULTS_COUNT__) {
      CHECK(is_compatible_with
        (reference_E[r], reference_SE[r], tested_E[r], tested_SE[r]), 1);
    }
    found = 1;
  }
  CHECK(found, 1);
}

static void
check_1_global
  (FILE* tested_file,
   const double reference_E,
   const double reference_SE,
   const unsigned rank)
{
  char line[MAX_LINE_LEN];
  double a[2];
  size_t recv_count, r2;
  unsigned i;
  int nb;
  double tested_E, tested_SE;

  get_dir_and_counts(tested_file, a, &recv_count, &r2);

  /* Skip receivers */
  while(recv_count--) CHECK(read_line(line, sizeof(line), tested_file), 1);

  /* Read the rank th global data */
  FOR_EACH(i, 0, rank+1) CHECK(read_line(line, sizeof(line), tested_file), 1);

  nb = sscanf(line, "%lg%lg", &tested_E, &tested_SE);
  CHECK(nb, 2);
  CHECK(is_compatible_with(reference_E, reference_SE, tested_E, tested_SE), 1);
}

static void
check_references(FILE* ref_file, FILE* tested_file)
{
  char line[MAX_LINE_LEN];
  unsigned nb_global = 0;
  fpos_t pos;

  NCHECK(ref_file, NULL);
  NCHECK(tested_file, NULL);

  CHECK(fgetpos(ref_file, &pos), 0);
  while(read_line(line, sizeof(line), ref_file)) {
    double val, std;
    int nb = 0;

    if(IS_NEW_BLOCK(line, sundir_header)) {
      /* Keep the header as a part of the following block */
      CHECK(fsetpos(ref_file, &pos), 0);
      break;
    }

    nb = sscanf(line, "%lg%lg", &val, &std);
    CHECK(nb == 0 || nb == 2, 1);

    rewind(tested_file);
    if(nb != 0) {
      check_1_global(tested_file, val, std, nb_global);
      nb_global++;
    } else {
      char ref_name[MAX_LINE_LEN];
      double reference_E[MAX_RESULTS_COUNT__];
      double reference_SE[MAX_RESULTS_COUNT__];
      read_recv(line, ref_name, reference_E, reference_SE);
      check_1_reference(tested_file, ref_name, reference_E, reference_SE);
    }

    CHECK(fgetpos(ref_file, &pos), 0);
  }
}

static FINLINE int
create_tmp_file_name(char* name, const size_t max_sizeof_name)
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
  unsigned long c1, realisation_count;
  int n;
  ASSERT(base_name);

  n = snprintf(ref_file_name, sizeof(ref_file_name), "%s%s.ref", dir, base_name);
  CHECK((size_t)n < sizeof(ref_file_name), 1);

  ref_file = fopen(ref_file_name, "r");
  NCHECK(ref_file, NULL);

  while(!feof(ref_file)) {
    char cmd[512];
    char tested_file_name[128];
    double sun_angles[2];
    FILE* fp = NULL;
    int fd = -1;

    get_dir_and_counts(ref_file, sun_angles, &c1, &realisation_count);

    fd = create_tmp_file_name(tested_file_name, sizeof(tested_file_name));
    fp = fdopen(fd, "r");
    NCHECK(fp, NULL);

    n = snprintf(cmd, sizeof(cmd),
      "%s -o %s -f -D %g,%g -n %lu -R %s%s_receiver.yaml %s%s.yaml",
      binary, tested_file_name, SPLIT2(sun_angles), realisation_count,
      dir, base_name, dir, base_name);
    CHECK((unsigned)n < sizeof(cmd), 1);

    CHECK(system(cmd), RES_OK);

    check_references(ref_file, fp);

    fclose(fp);
    remove(tested_file_name);
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

