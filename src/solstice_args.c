/* Copyright (C) CNRS 2016
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

#define _POSIX_C_SOURCE 2

#include "solstice_args.h"

#include <rsys/cstr.h>

#ifdef COMPILER_CL
  #include <getopt.h>
#else
  #include <unistd.h>
#endif

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
print_help(const char* program)
{
  printf(
"Usage: %s [OPTIONS] [FILE]\n"
"Integrate the solar flux in complex solar facilities.\n",
    program);
  /* TODO print short help for the options */
}

static res_T
parse_rendering(const char* str, struct solstice_args* args)
{
  ASSERT(args);
  (void)str, (void)args;
  /* TODO */
  return RES_OK;
}

/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
solstice_args_init(struct solstice_args* args, const int argc,  char** argv)
{
  int opt;
  res_T res = RES_OK;
  ASSERT(args && argc && argv);

  *args = SOLSTICE_ARGS_NULL;

  while((opt = getopt(argc, argv, "hn:o:qr:")) != -1) {
    switch(opt) {
      case 'h':
        print_help(argv[0]);
        solstice_args_release(args);
        goto exit;
      case 'n':
        res = cstr_to_ulong(optarg, &args->nrealisations);
        if(res != RES_OK && !args->nrealisations) res = RES_BAD_ARG;
        break;
      case 'o': args->output_filename = optarg; break;
      case 'q': args->quiet = 1; break;
      case 'r': res = parse_rendering(optarg, args); break;
      default: res = RES_BAD_ARG; break;
    }
    if(res != RES_OK) {
      if(optarg) {
        fprintf(stderr, "%s: invalid option argument '%s' -- '%c'\n",
          argv[0], optarg, opt);
      }
      goto error;
    }
  }
exit:
  return res;
error:
  solstice_args_release(args);
  goto exit;
}

void
solstice_args_release(struct solstice_args* args)
{
  ASSERT(args);
  *args = SOLSTICE_ARGS_NULL;
}

