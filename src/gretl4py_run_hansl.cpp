/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gretl4py_internals.h"
#include "gretl4py_utils.h"
#include "gretl_calls.h"
#include <gretl/libgretl.h>
#include <gretl/libset.h>

#define ENDRUN (NC + 1)
#define RUNLOOP (NC + 2)

char datafile[MAXLEN];
char cmdfile[MAXLEN];
FILE *fb;
int batch;
int runit;
int indent0;
int batch_stdin;
int data_status;
int gui_exec;
char linebak[MAXLINE];      /* for storing comments */
char *line_read;

static gretl_dataset_ptr script_dataset{nullptr};           // dataset for subsequent script calls
static bool nulldata_executed = false;                      // flag that script_dataset was created/modified by 'nulldata' command

int gretl_execute_script (std::string &source)
{
    char linecopy[MAXLINE];
    DATASET *dset = nullptr;
    MODEL *model = nullptr;
    ExecState state;
    char *line = nullptr;
    int quiet = 0;
    int tool = 0;
    int pkgmode = 0;
    int load_datafile = 1;
    char filearg[MAXLEN];
    char runfile[MAXLEN];
    double scriptval = NADBL;
    CMD cmd;
    PRN *prn = nullptr;
    PRN *cmdprn = nullptr;
    gretlopt opt = OPT_NONE;
    int err = 0;

#ifdef HAVE_RL_DONE
    rl_bind_key(0x18, ctrl_x);
#endif

    // dataset and model to use
    model = allocate_working_model();
    if (model == nullptr) handle_gretl_error(E_ALLOC);
    if (!script_dataset && !nulldata_executed) script_dataset.reset(datainfo_new());
    dset = script_dataset.get();
    if (dset == nullptr) handle_gretl_error(E_ALLOC);

    runfile[0] = filearg[0] = '\0';

    // we assume "batch" mode
    // note: _MUST_ be templated on 'source' type ! ! ! !
    SET_OPT(opt, OPT_BATCH);
    std::strcpy(filearg,  source.c_str());

    if (argc < 2) {
        force_language(LANG_AUTO);
        load_datafile = 0;
      } else {
          if (opt & (OPT_BATCH | OPT_RUNIT | OPT_MAKEPKG | OPT_INSTPKG)) {
              if (*filearg == '\0') {
                  // we're missing a filename argument FIXME::must be checked earlier,  maybe before call
                  handle_gretl_error(E_INVARG, "No filename given");
                  //usage(1);
                } else if ((opt & OPT_BATCH) && !strcmp(filearg, "-")) {
                    // batch mode, but read from stdin
                    /*quiet = batch_stdin = batch = 1;
                    *runfile = '\0';
                    load_datafile = 0;*/
                    std::runtime_error("batch_stdin");
                  } else {
                      // record argument (not a datafile)
                      strcpy(runfile, filearg);
                      load_datafile = 0;
                      if (opt & OPT_BATCH) {
                          batch = 1;
                        /*} else if (opt & OPT_MAKEPKG) {
                            tool = quiet = batch = 1;
                            pkgmode = OPT_MAKEPKG;
                        } else if (opt & OPT_INSTPKG) {
                            tool = quiet = batch = 1;
                            pkgmode = OPT_INSTPKG;*/
                        } else {
                            runit = 1;
                        }
                    }
            } else if (*filearg == '\0') {
                load_datafile = 0;
            }

          /*if (opt & OPT_TOOL) {
              tool = quiet = 1;
              gretl_set_tool_mode();
          } else if (opt & OPT_QUIET) {
              quiet = 1;
          } */

          /*if (opt & OPT_ENGLISH) {
              force_language(LANG_C);
          } else {
              force_language(LANG_AUTO);
          } */
      }

    // end of argc:argv

    libgretl_init();

    if (gui_exec) {
        char tstr[64];

        printf("gretl %s %s\n\n", GRETL_VERSION, print_time(tstr));
      } else if (!tool) {
          logo(quiet);
          if (!quiet) {
              session_time(NULL);
            }
        }

#if defined(OPENMP_BUILD) && !defined(WIN32) && !defined(__APPLE__)
    check_blas_threading(tool, quiet);
#endif

    prn = gretl_print_new(GRETL_PRINT_STDOUT, &err);
    if (err) {
        noalloc();
      }

    line = malloc(MAXLINE);
    if (line == NULL) {
        noalloc();
      }

#ifdef WIN32
    win32_cli_read_rc();
#else
    cli_read_rc();
#endif /* WIN32 */

    if (!batch && !tool) {
        strcpy(cmdfile, gretl_workdir());
        strcat(cmdfile, "session.inp");
        cmdprn = gretl_print_new_with_filename(cmdfile, &err);
        if (err) {
            errmsg(err, prn);
            return EXIT_FAILURE;
          }
      }

    if (load_datafile) {
        handle_datafile(filearg, runfile, dset, prn, cmdprn);
      }

    /* allocate memory for model */
    model = allocate_working_model();
    if (model == NULL) {
        noalloc();
      }

    gretl_cmd_init(&cmd);
    gretl_exec_state_init(&state, 0, line, &cmd, model, prn);

    /* print list of variables */
    if (data_status) {
        list_series(dset, OPT_NONE, prn);
      }

    if (!na(scriptval)) {
        /* define "scriptopt" */
        gretl_scalar_add("scriptopt", scriptval);
      }

    if (!batch) {
        /* misc. interactive-mode setup */
        check_help_file();
        fb = stdin;
        push_input_file(fb);
        if (!runit && !data_status) {
            fputs(_("Type \"open filename\" to open a data set\n"), stdout);
          }
      } else if (batch_stdin) {
          fb = stdin;
          push_input_file(fb);
        }

    if (gui_exec) {
        write_my_pid();
      }

#ifdef HAVE_READLINE
    initialize_readline();
#endif

    if (batch || runit) {
        /* re-initialize: will be incremented by "run" cmd */
        runit = 0;
        if (*runfile != '\0' && pkgmode != OPT_INSTPKG) {
            if (strchr(runfile, ' ')) {
                sprintf(line, "run \"%s\"", runfile);
              } else {
                  sprintf(line, "run %s", runfile);
                }
            if (gui_exec) {
                state.flags |= INIT_EXEC;
              }
            err = cli_exec_line(&state, dset, cmdprn);
            if (gui_exec) {
                state.flags ^= INIT_EXEC;
              }
            if (err && fb == NULL) {
                exit(EXIT_FAILURE);
              }
          }
      }

    *linecopy = '\0';

/* enter main command loop */

    while (cmd.ci != QUIT && fb != NULL) {
        if (err && gretl_error_is_fatal()) {
            gretl_abort(linecopy);
          }
        if (gretl_execute_loop()) {
            state.cmd->ci = RUNLOOP;
            err = cli_exec_line(&state, dset, cmdprn);
            state.cmd->ci = 0;
          } else {
              err = cli_get_input_line(&state, runfile);
              if (err) {
                  errmsg(err, prn);
                  break;
                } else if (cmd.ci == QUIT) {
                    /* no more input available */
                    cli_exec_line(&state, dset, cmdprn);
                    if (runit == 0) {
                        err = gretl_if_state_check(0);
                        if (err) {
                            errmsg(err, prn);
                          }
                      }
                            continue;
                  }
            }

        if (xout) {
            /* readline Ctrl-X: get out without saving
            input or output */
            gretl_print_destroy(cmdprn);
            gretl_remove(cmdfile);
            break;
          }

        if (!state.in_comment) {
            if (cmd.context == FOREIGN || cmd.context == MPI ||
                gretl_compiling_python(line)) {
                    tailstrip(line);
                  } else {
                      err = maybe_get_input_line_continuation(line);
                      if (err) {
                          errmsg(err, prn);
                          break;
                        }
                    }
          }

#ifdef G_OS_WIN32
        line_ensure_utf8(line);
#endif
        strcpy(linecopy, line);
        tailstrip(linecopy);
        err = cli_exec_line(&state, dset, cmdprn);
      }

/* finished main command loop */

    if (!err) {
        err = gretl_if_state_check(0);
        if (err) {
            errmsg(err, prn);
          }
      }

    if (!err && pkgmode) {
        if (pkgmode == OPT_MAKEPKG) {
            switch_ext(filearg, runfile, "gfn");
            sprintf(line, "makepkg %s\n", filearg);
          } else {
              if (!isalpha(filearg[0]) || filearg[1] == ':') {
                  /* some sort of path: install local file */
                  sprintf(line, "pkg install %s --local\n", filearg);
                } else {
                    /* plain filename or http: install from server */
                    sprintf(line, "pkg install %s\n", filearg);
                  }
            }
        cli_exec_line(&state, dset, cmdprn);
      }

/* leak check -- try explicitly freeing all memory allocated */

    destroy_working_model(model);
    destroy_dataset(dset);

    if (fb != stdin && fb != NULL) {
        fclose(fb);
      }

    free(line);
    gretl_print_destroy(prn);
    gretl_cmd_free(&cmd);
    libgretl_cleanup();

    exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
