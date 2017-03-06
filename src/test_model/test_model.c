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

#include <rsys/rsys.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef COMPILER_CL
  /* Wrap POSIX functions and constants */
  #include <io.h>
  #define fdopen _fdopen
  #define fileno _fileno
  #define execvp _execvp
#endif

#ifndef COMPILER_CL
#include <linux/limits.h> /* MAX_PATH */
#endif

enum side {
  FRONT,
  BACK
};

enum RESULTS {
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

static int
file_exists(const char* name)
{
  FILE* f = fopen(name, "r");
  if (!f) return 0;
  fclose(f);
  return 1;
}

#define MAX_LINE_LEN 2048
static res_T
get_dir_and_counts
  (FILE* ref_file,
   double sun_dir[3],
   size_t* recv_count,
   size_t* realisation_count)
{
  const char sundir_header [] = "#--- Sun direction:";
  char line[MAX_LINE_LEN];

  ASSERT(ref_file);
  if (!fgets(line, sizeof(line), ref_file)) return RES_BAD_ARG;
  if (0 != strncmp(line, sundir_header, strlen(sundir_header)))
    return RES_BAD_ARG;
  /* get sun dir */
  if (3 != sscanf(line + strlen(sundir_header),
    "%lg%lg%lg", &sun_dir[0], &sun_dir[1], &sun_dir[2])) {
    return RES_BAD_ARG;
  }
  /* get sun dir */
  if (!fgets(line, sizeof(line), ref_file)) return RES_BAD_ARG;
  if (2 != sscanf(line, "%zu%zu", recv_count, realisation_count))
    return RES_BAD_ARG;
  return RES_OK;
}

#define READ_RECV(Name, Values, Std)                                          \
{                                                                             \
 if (2 * MAX_RESULTS_COUNT__ + 1                                              \
  != sscanf(line,                                                             \
  "%s%*zu%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg%lg",       \
  Name,                                                                       \
  &Values[FRONT_INTEGRATED_IRRADIANCE], &Std[FRONT_INTEGRATED_IRRADIANCE],    \
  &Values[BACK_INTEGRATED_IRRADIANCE], &Std[BACK_INTEGRATED_IRRADIANCE],      \
  &Values[FRONT_REFLECTIVITY_LOSS], &Std[FRONT_REFLECTIVITY_LOSS],            \
  &Values[BACK_REFLECTIVITY_LOSS], &Std[BACK_REFLECTIVITY_LOSS],              \
  &Values[FRONT_ABSORPTIVITY_LOSS], &Std[FRONT_ABSORPTIVITY_LOSS],            \
  &Values[BACK_ABSORPTIVITY_LOSS], &Std[BACK_ABSORPTIVITY_LOSS],              \
  &Values[FRONT_COS_LOSS], &Std[FRONT_COS_LOSS],                              \
  &Values[BACK_COS_LOSS], &Std[BACK_COS_LOSS],                                \
  &Values[FRONT_EFFICIENCY], &Std[FRONT_EFFICIENCY],                          \
  &Values[BACK_EFFICIENCY], &Std[BACK_EFFICIENCY])                            \
  )                                                                           \
 {                                                                            \
    res = RES_BAD_ARG;                                                        \
    goto error;                                                               \
  }                                                                           \
}

#define POSITIVE_OR_M_ONE(x) ((x) == -1 || (x) >= 0)

static FINLINE int
is_compatible_with
  (const double ref_E,
   const double ref_SE,
   const double test_E,
   const double test_SE)
{
  ASSERT(POSITIVE_OR_M_ONE(ref_E) && POSITIVE_OR_M_ONE(ref_SE)
    && POSITIVE_OR_M_ONE(test_E) && POSITIVE_OR_M_ONE(test_SE));
  if (ref_E == -1) {
    ASSERT(ref_SE == -1);
    return (test_E == -1 && test_SE == -1);
  }
  ASSERT(ref_SE != -1);
  return (fabs(ref_E - test_E) <= 2 * ref_SE && test_SE <= 2 * ref_SE);
}

static res_T 
check_1_reference
  (const char* tested_file_name,
   const char* rcv_name,
   const double* reference_E,
   const double* reference_SE)
{
  res_T res = RES_OK;
  ASSERT(tested_file_name && rcv_name && reference_E && reference_SE);
  FILE* tested_file = fopen(tested_file_name, "r");
  double d[3];
  size_t c1, c2;

  if (!tested_file) {
    res = RES_IO_ERR;
    goto end;
  }
  res = get_dir_and_counts(tested_file, d, &c1, &c2); /* skip headers */
  if (res != RES_OK) goto error;
  while(!feof(tested_file)) {
    char line[MAX_LINE_LEN];
    char tested_rcv_name[MAX_LINE_LEN];
    double tested_E[MAX_RESULTS_COUNT__], tested_SE[MAX_RESULTS_COUNT__];
    enum RESULTS r;
    if (!fgets(line, sizeof(line), tested_file)) {
      res = RES_BAD_ARG;
      goto error;
    }
    READ_RECV(tested_rcv_name, tested_E, tested_SE);
    if (strcmp(rcv_name, tested_rcv_name)) continue;
    for (r = FRONT_INTEGRATED_IRRADIANCE; r < MAX_RESULTS_COUNT__; r++) {
      if (!is_compatible_with
        (reference_E[r], reference_SE[r], tested_E[r], tested_SE[r]))
      {
        res = RES_BAD_ARG;
        goto error;
      }
    }
    goto end; /* success */
  }
  res = RES_BAD_ARG; /* could not find data */
end:
  if (tested_file) fclose(tested_file);
  return res;
error:
  goto end;
}

static res_T
check_1_global
  (const char* tested_file_name,
   const double reference_E,
   const double reference_SE,
   const unsigned rank)
{
  res_T res = RES_OK;
  char line[MAX_LINE_LEN];
  FILE* tested_file;
  double d[3];
  size_t recv_count, r2;
  unsigned i;
  int nb;
  double tested_E, tested_SE;

  ASSERT(tested_file_name);
  tested_file = fopen(tested_file_name, "r");
  if (!tested_file) {
    res = RES_IO_ERR;
    goto end;
  }
  res = get_dir_and_counts(tested_file, d, &recv_count, &r2);
  if (res != RES_OK) goto error;
  /* skip receivers */
  for ( ; recv_count--; ) {
    if (!fgets(line, sizeof(line), tested_file)) {
      res = RES_BAD_ARG;
      goto error;
    }
  }
  /* read the rank th global data */
  for (i = 0; i <= rank; i++) {
    if (!fgets(line, sizeof(line), tested_file)) {
      res = RES_BAD_ARG;
      goto error;
    }
  }
  nb = sscanf(line, "%lg%lg", &tested_E, &tested_SE);
  if (nb != 2) {
    res = RES_BAD_ARG;
    goto error;
  }
  if (!is_compatible_with
    (reference_E, reference_SE, tested_E, tested_SE)) {
    res = RES_BAD_ARG;
    goto error;
  }

end:
  if (tested_file) fclose(tested_file);
  return res;
error:
  goto end;
}


static res_T
check_references
  (FILE* ref_file, const char* tested_file_name)
{
  res_T res = RES_OK;
  char line[MAX_LINE_LEN];
  int nb_global = 0;

  ASSERT(ref_file && tested_file_name);
  for ( ; ; ) {
    int nb = 0;
    double val, std;
    if (!fgets(line, sizeof(line), ref_file)) {
      if (feof(ref_file)) goto end;
      res = RES_BAD_ARG;
      goto error;
    }
    nb = sscanf(line, "%lg%lg", &val, &std);
    ASSERT(nb == 0 || nb == 2);
    if (nb != 0) {
      res = check_1_global(tested_file_name, val, std, nb_global);
      if (res != RES_OK) goto error;
      nb_global++;
    }
    else {
      char ref_name[MAX_LINE_LEN];
      double reference_E[MAX_RESULTS_COUNT__], reference_SE[MAX_RESULTS_COUNT__];
      READ_RECV(ref_name, reference_E, reference_SE);
      res = 
        check_1_reference(tested_file_name, ref_name, reference_E, reference_SE);
      if (res != RES_OK) goto error;
    }
  };

end:
  CHECK(remove(tested_file_name), 0);
  return res;
error:
  goto end;
}

static FINLINE res_T
create_tmp_file_name(char* out_name)
{
  ASSERT(out_name);
#ifdef COMPILER_CL
  if (tmpnam_s(out_name, L_tmpnam_s)) return RES_IO_ERR;
#else
  int fd;
  strncpy(out_name, "solstice_tmp_file_XXXXXX", L_tmpnam_s);
  fd = mkstemp(out_name);
  if (-1 == fd) return RES_IO_ERR;
  /* just want a name */
  close(fd);
#endif
  return RES_OK;
}

static res_T
do_check(const char* base_name)
{
  res_T res = RES_OK;
  FILE* ref_file;
  char ref_file_name[MAX_PATH];
  size_t c1, realisation_count;

  ASSERT(base_name);
  snprintf(ref_file_name, MAX_PATH, "../../yaml/%s.ref", base_name);
  ref_file = fopen(ref_file_name, "r");
  if (!ref_file) {
    res = RES_IO_ERR;
    printf("Cannot open file '%s'\n", base_name);
    goto end;
  }
  while (!feof(ref_file)) {
    char cmd[128 + 3 * MAX_PATH];
    double sun_dir[3];
    char tested_file_name[L_tmpnam_s];
#ifdef COMPILER_CL
    char* exe_name = "..\\Debug\\solstice.exe";
#else
    char* exe_name = "../Debug/solstice.exe";
#endif

    res = get_dir_and_counts(ref_file, sun_dir, &c1, &realisation_count);
    if (res != RES_OK) goto end;

    res = create_tmp_file_name(tested_file_name);
    if (res != RES_OK) goto end;

    snprintf(cmd, sizeof(cmd),
      "%s -o %s -f -3 %lg,%lg,%lg -n %zu -R ../../yaml/%s_receiver.yaml ../../yaml/%s.yaml",
      exe_name, tested_file_name, SPLIT3(sun_dir), realisation_count, base_name, base_name);
    if (system(cmd)) {
      res = RES_BAD_ARG;
      goto end;
    }
    res = check_references(ref_file, tested_file_name);
    if (res != RES_OK) {
      goto end;
    }
  }
end:
  if (ref_file) fclose(ref_file);
  return res;
}

/*
 * FIXME: does not manage multiple sun directions
 */
int
main(int argc, char** argv)
{
  int err = 0;

  if (argc != 2) goto usage;

  if (RES_OK != do_check(argv[1]))
    goto error;

exit:
  return err;
usage:
  printf("Usage: %s <file_base_name>\n", argv[0]);
error:
  err = 1;
  goto exit;
}

