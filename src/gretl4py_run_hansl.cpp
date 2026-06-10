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

// NOTE: Ported from gretlcli.c and adapted for gretl4py.
// The control flow is intentionally kept close to the original
// implementation to preserve compatibility with gretl CLI behavior.

#include "gretl4py.h" /* MUST be at the first position!!! */
#include "gretl_calls.h"
#include "gretl4py_utils.h"
#include "gretl4py_types.h"
#include "gretl4py_internals.h"
#include "gretl4py_run_hansl.h"
#include <gretl/libset.h>
extern "C" {
    #include <gretl/monte_carlo.h>
    #include <gretl/gretl_xml.h>
    #include <gretl/system.h>
}
#include <filesystem>
#include <chrono>
#ifdef _WIN32
 #include <windows.h>
#endif

#define ENDRUN (NC + 1)
#define RUNLOOP (NC + 2)
#define N_STACKED_FILES 8
#define XOR_CMDFLAG(x, flag) x = static_cast<CmdFlags>((x) ^ (flag))
#define OR_EXECFLAG(x, flag) x = static_cast<ExecFlags>((x) | (flag))
#define XOR_EXECFLAG(x, flag) x = static_cast<ExecFlags>((x) ^ (flag))

static int nfiles;
static std::FILE *fstack[N_STACKED_FILES];
char datafile[MAXLEN];
char cmdfile[MAXLEN];
std::FILE *fb;
int batch;
int runit;
int indent0;
int batch_stdin;
int data_status;
int gui_exec;
char linebak[MAXLINE];      // for storing comments
char *line_read;
static int xout;

static GretlPrint global_printer = GretlPrint();
static gretl_dataset_ptr script_dataset{nullptr};  // dataset for subsequent script calls
static bool nulldata_executed = false;             // flag that script_dataset was created/modified by 'nulldata' command

#ifdef _WIN32
static void line_ensure_utf8(char *line)
{
    if (line == nullptr || *line == '\0') return;

    int len = std::strlen(line);

    // convert from current ANSI codepage to UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, line, len, nullptr, 0);

    if (wlen <= 0) return;

    std::wstring wbuf(wlen, L'\0');

    MultiByteToWideChar(CP_ACP, 0, line, len, wbuf.data(), wlen);

    // convert UTF-16 → UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, nullptr, 0, nullptr, nullptr);

    if (u8len <= 0) return;

    std::string utf8(u8len, '\0');

    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, utf8.data(), u8len, nullptr, nullptr);

    // copy back into original buffer (truncate safely)
    std::strncpy(line, utf8.c_str(), MAXLINE - 1);
    line[MAXLINE - 1] = '\0';

}
#endif

static void printline (const char *s)
{
    if (*s != '\0') {
        if (gretl_compiling_loop()) {
            std::printf("> %s\n", s);
        } else {
            std::printf("%s\n", s);
        }
    }
}

static void handle_datafile (char *filearg, char *runfile, DATASET *dset, PRN *prn, PRN *cmdprn)
{
    char given_file[MAXLEN];
    int load_datafile = 1;
    GretlFileType ftype;
    int err = 0;

    std::strcpy(given_file, filearg);
    std::strcpy(datafile, filearg);

    ftype = detect_filetype(datafile, OPT_P);

    switch (ftype) {
        case GRETL_UNRECOGNIZED:
        case GRETL_NATIVE_DB:
        case GRETL_RATS_DB:
            std::exit(EXIT_FAILURE);
            break;
        case GRETL_XML_DATA:
        case GRETL_BINARY_DATA:
            err = gretl_read_gdt(datafile, dset, OPT_NONE, prn);
            break;
        case GRETL_CSV:
            err = import_csv(datafile, dset, OPT_NONE, prn);
            break;
        case GRETL_XLS:
        case GRETL_GNUMERIC:
        case GRETL_ODS:
            err = import_spreadsheet(datafile, ftype, nullptr, nullptr, dset, OPT_NONE, prn);
            break;
        case GRETL_DTA:
        case GRETL_SAV:
        case GRETL_SAS:
        case GRETL_JMULTI:
        case GRETL_OCTAVE:
        case GRETL_WF1:
            err = import_other(datafile, ftype, dset, OPT_NONE, prn);
            break;
        case GRETL_SCRIPT:
            runit = 1;
            std::strcpy(runfile, datafile);
            std::memset(datafile, 0, sizeof datafile);
            load_datafile = 0;
            break;
        default:
            break;
    }

    if (load_datafile) {
        if (err) {
            errmsg(err, prn);
            if (err == E_FOPEN) show_paths();
            std::exit(EXIT_FAILURE);
        }
        data_status = 1;
        if (!batch) pprintf(cmdprn, "open %s\n", given_file);
    }
}

static void gretl_abort (char *line)
{
    const char *tokline = get_parser_errline();

    std::fprintf(stderr, "\ngretlcli: error executing script: halting\n");
    if (tokline != nullptr && *tokline != '\0' && std::strcmp(tokline, line)) std::fprintf(stderr, "> %s\n", tokline);
    if (*line != '\0') std::fprintf(stderr, "> %s\n", line);

    std::exit(EXIT_FAILURE);
}

static int cli_parse_object_request (const char *line, char *objname, char **param,
                                     void **pptr, GretlObjType *type, PRN *prn)
{
    char word[MAXSAVENAME] = {0};
    int action;

    /* get object name (if any) and dot param */
    parse_object_command(line, word, param);

    /* if no dot param, nothing doing */
    if (*param == nullptr) return OBJ_ACTION_NONE;

    if (gretl_is_bundle(word)) return OBJ_ACTION_NONE;

    // see if there's an object associated with the name
    *pptr = gretl_get_object_and_type(word, type);

    if (*pptr == nullptr) {
        // no matching object
        if (*param != nullptr && !strcmp(*param, "show")) {
            /* maybe OK in GUI, skip in CLI? */
            pprintf(prn, ("%s: no such object\n"), word);
            return OBJ_ACTION_NULL;
        }
        return OBJ_ACTION_NONE;
    }

    action = match_object_command(*param);

    if (action == OBJ_ACTION_INVALID) {
        pprintf(prn, ("command '%s' not recognized"), *param);
        pputc(prn, '\n');
    } else {
        std::strcpy(objname, word);
    }

    return action;
}

static int cli_saved_object_action (const char *line, DATASET *dset, PRN *prn)
{
    char objname[MAXSAVENAME] = {0};
    char *param = nullptr;
    void *ptr = nullptr;
    GretlObjType type;
    int action;

    // shell command or comment (not an object command)
    if (*line == '!' || *line == '#') return OBJ_ACTION_NONE;

    // special: display icon view window, ignored in CLI program
    if (!std::strncmp(line, "iconview", 8)) return OBJ_ACTION_NULL;

    action = cli_parse_object_request(line, objname, &param, &ptr, &type, prn);

    if (action == OBJ_ACTION_SHOW) {
        // ignore
        action = OBJ_ACTION_NULL;
    } else if (action == OBJ_ACTION_FREE) {
        gretl_object_remove_from_stack(ptr, type);
        pprintf(prn, ("Freed %s\n"), objname);
    }

    std::free(param);

    return action;
}

static int maybe_abort_open (ExecState *s)
{
    if (data_status && !batch) {
        char response[3];

        std::fprintf(stderr, ("Opening a new data file closes the " "present one.  Proceed? (y/n) "));
        if (std::fgets(response, sizeof response, stdin) != nullptr && *response != 'y' && *response != 'Y') {
            pprintf(s->prn, ("OK, staying with current data set\n"));
            return 1;
        }
    }

    return 0;
}

static int cli_clear_data (ExecState *s, DATASET *dset)
{
    CMD *cmd = s->cmd;
    gretlopt clearopt = OPT_NONE;
    int err = 0;

    if (cmd->ci == CLEAR) {
        clearopt = cmd->opt;
    } else if (cmd->opt & OPT_P) {
        // --preserve: clear dataset only
        clearopt = OPT_D;
    } else if (csv_open_needs_matrix(cmd->opt)) {
        clearopt = OPT_D;
    }

    if (dset->Z != nullptr) {
        err = restore_full_sample(dset, nullptr);
        free_Z(dset);
    }

    clear_datainfo(dset, CLEAR_FULL);
    data_status = 0;
    *datafile = '\0';

    clear_model(s->model);

    if (clearopt & OPT_A) gretl_functions_cleanup();
    if (clearopt & OPT_D) {
        libgretl_session_cleanup(SESSION_CLEAR_DATASET);
    } else {
        libgretl_session_cleanup(SESSION_CLEAR_ALL);
    }

    set_model_count(0);
    gretl_cmd_destroy_context(cmd);

    return err;
}

static int cli_exec_callback (ExecState *s, void *ptr, GretlObjType type)
{
    if (s->cmd->ci == MODELTAB || s->cmd->ci == GRAPHPG) {
        pprintf(s->prn, ("%s: command not available\n"), gretl_command_word(s->cmd->ci));
    } else if (s->cmd->ci == OPEN) {
        if (type == GRETL_OBJ_DSET) {
            // check that "open" is really OK
            if (maybe_abort_open(s)) {
                return 1;
            } else {
                if (gretl_looping()) {
                    //s->cmd->opt |= OPT_P;
                    SET_OPT(s->cmd->opt, OPT_P);
                }
                cli_clear_data(s, (DATASET *) ptr);
            }
        } else if (type == GRETL_OBJ_ANY) {
            // handle successful "open"
            OpenOp *op = static_cast<OpenOp*>(ptr);

            if (op->fname[0] != '\0') std::snprintf(datafile, MAXLEN, "%s", op->fname); //std::strncpy(datafile, op->fname, MAXLEN - 1);
            data_status = 1;
        }
    }

    return 0;
}

// apparatus for keeping track of input stream: start

static int push_input_file (std::FILE *fp)
{
    int err = 0;

    if (nfiles >= N_STACKED_FILES) {
        err = 1;
    } else {
        fstack[nfiles++] = fp;
    }

    return err;
}

static std::FILE *pop_input_file (void)
{
    std::FILE *ret = nullptr;

    if (nfiles > 0) ret = fstack[--nfiles];

    return ret;
}

// apparatus for keeping track of input stream: end

static void do_quit_message (ExecState *s, int err)
{
    if (gretl_messages_on() && s != nullptr && s->prn != nullptr) {
        if (err) {
            pputs(s->prn, ("Terminated on error\n"));
        } else {
            pputs(s->prn, ("Done\n"));
        }
    }
}

static void maybe_save_session_output (const char *cmdfile)
{
    char outfile[FILENAME_MAX];

    std::printf(("type a filename to store output (enter to quit): "));

    *outfile = '\0';
    if (std::fgets(outfile, sizeof outfile, stdin) != nullptr) top_n_tail(outfile, 0, nullptr);
    if (*outfile != '\0' && *outfile != '\n' && *outfile != '\r' && strcmp(outfile, "q")) {
        const char *udir = gretl_workdir();
        int err;

        std::printf(("writing session output to %s%s\n"), udir, outfile);
#ifdef WIN32
        auto syscmd = gretl_char_ptr(gretl_strdup_printf("\"%sgretlcli\" -b \"%s\" > \"%s%s\"", gretl_home(), cmdfile, udir, outfile));
#else
        auto syscmd = gretl_char_ptr(gretl_strdup_printf("gretlcli -b \"%s\" > \"%s%s\"", cmdfile, udir, outfile));
#endif
        err = std::system(syscmd.get());
        if (!err) std::printf("%s\n", syscmd.get());
    }
}

static void cli_quit (ExecState *s, PRN *cmdprn, int err)
{
    if (runit || batch_stdin) {
        *s->runfile = '\0';
        runit--;
        std::fclose(fb);
        fb = pop_input_file();
        if (fb == nullptr) {
            do_quit_message(s, err);
        } else {
            gretl_if_state_reset(indent0);
            s->cmd->ci = ENDRUN;
        }
    } else if (batch && fb == nullptr) {
        do_quit_message(s, err);
    } else {
        gretl_print_destroy(cmdprn);
        if (s->cmd->opt & OPT_X) {
            gretl_remove(cmdfile);
        } else {
            std::printf(("commands saved as %s\n"), cmdfile);
            maybe_save_session_output(cmdfile);
        }
    }
}

static int cli_do_pkg_sample (const char *pkgname, char *runfile, PRN *prn)
{
    char *buf = nullptr;
    int err = 0;

    err = grab_package_sample(pkgname, &buf);

    if (!err) {
        std::string base = std::string(pkgname) + "_sample.inp";        //was: gchar *base = g_strdup_printf("%s_sample.inp", pkgname);
        auto fname = gretl_gchar_ptr(gretl_make_dotpath(base.c_str())); //was: gchar *fname = gretl_make_dotpath(base);
        std::FILE *fp = gretl_fopen(fname.get(), "w");

        if (fp == nullptr) {
            err = 1;
        } else {
            std::fputs(buf, fp);
            std::fclose(fp);
            std::strcpy(runfile, fname.get());
        }
        std::free(buf);
    }

    return err;
}

static int run_include_error (ExecState *s, const char *param, int err, PRN *prn)
{
    const char *msg = gretl_errmsg_get();

    pprintf(prn, ("Error reading '%s'\n"), param);
    if (*msg != '\0') pprintf(prn, "%s\n", msg);

    return process_command_error(s, err);
}

static int cli_renumber_series (const int *list, const char *parm, DATASET *dset, PRN *prn)
{
    int err, fixmax = highest_numbered_var_in_saved_object(dset);

    err = renumber_series_with_checks(list, parm, fixmax, dset, prn);
    if (err) errmsg(err, prn);

    return err;
}

static int file_get_line (ExecState *s, const char *fname)
{
    char *line = s->line;
    int len = 0;

    std::memset(line, 0, MAXLINE);

    if (std::fgets(line, MAXLINE, fb) == nullptr) {
        // no more input from current source */
        gretl_exec_state_uncomment(s);
    } else {
        len = std::strlen(line);
    }

    if (*line == '\0') {
        if (fb != stdin && gretl_compiling_loop()) {
            gretl_errmsg_sprintf(("Broken syntax in %s: unmatched %s"), fname, "loop/endloop");
            return E_PARSE;
        }
        std::strcpy(line, "quit");
        s->cmd->ci = QUIT;
    } else if (len == MAXLINE - 1 && line[len-1] != '\n') {
        return E_TOOLONG;
    } else {
        *linebak = '\0';
        std::strncat(linebak, line, MAXLINE-1);
        tailstrip(linebak);
    }

    if (gretl_echo_on() && s->cmd->ci == RUN && batch && *line == '(') {
        std::printf("%s", line);
        *linebak = '\0';
    }

    return 0;
}

static const char *get_prompt (ExecState *s)
{
    if (s->flags & DEBUG_EXEC) {
        return "$ ";
    } else if (gretl_compiling_function() || gretl_compiling_loop()) {
        return "> ";
    } else {
        return "? ";
    }
}

static int get_interactive_line (void *p)
{
    ExecState *s = (ExecState *) p;
    const char *prompt = get_prompt(s);
    int err = 0;

#ifdef HAVE_READLINE
    rl_gets(&line_read, prompt);

    if (line_read == nullptr) {
        strcpy(s->line, "quit");
    } else if (strlen(line_read) > MAXLINE - 2) {
        err = E_TOOLONG;
    } else {
        *s->line = '\0';
        strncat(s->line, line_read, MAXLINE - 2);
        strcat(s->line, "\n");
    }
#else
    std::printf("%s", prompt);
    std::fflush(stdout);
    file_get_line(s, "***stdin*** (FIXME)"); //FIXME!!!!!
#endif

    return err;
}

static int cli_get_input_line (ExecState *s, const char *fname)
{
    int err = 0;

    if (runit || batch) {
        // reading from script file
        err = file_get_line(s, fname);
    } else {
        // interactive use
        err = get_interactive_line(s);
    }

    return err;
}

// cli_exec_line: this is called to execute both interactive and
// script commands.  Note that most commands get passed on to the
// libgretl function gretl_cmd_exec(), but some commands that require
// special action are dealt with here.
//
// see also gui_exec_line() in gui2/library.c

static int cli_exec_line (ExecState *s, DATASET *dset, PRN *cmdprn)
{
    char *line = s->line;
    CMD *cmd = s->cmd;
    PRN *prn = s->prn;
    int old_runit = runit;
    char runfile[MAXLEN];
    int renumber = 0;
    int err = 0;

    if (cmd->ci == RUNLOOP) goto cmd_proceed;

    if (gretl_compiling_function()) {
        err = gretl_function_append_line(s);
        if (err) {
            errmsg(err, prn);
            goto cmd_finish;
        } else {
            pprintf(cmdprn, "%s\n", line);
            return 0;
        }
    }

    if (string_is_blank(line)) return 0;

    if (!gretl_compiling_loop() && !s->in_comment && !cmd->context && gretl_if_state_true()) {
        // catch requests relating to saved objects, which are not really "commands" as such
        int action = cli_saved_object_action(line, dset, prn);

        if (action == OBJ_ACTION_INVALID) {
            // action was fault
            err = 1;
            goto cmd_finish;
        } else if (action != OBJ_ACTION_NONE) {
            return 0; // action was OK (and handled), or ignored
        }
    }

    // tell libgretl if we're in batch mode
    gretl_set_batch_mode(batch);

    if (gretl_compiling_loop()) {
        // if we're stacking commands for a loop, parse "lightly"
        err = get_command_index(s, LOOP, 0);
    } else {
        err = parse_command_line(s, dset, nullptr);
    }

    if (err) {
        int catch_local = 0;

        gretl_exec_state_uncomment(s);
        if (err != E_ALLOC && (cmd->flags & CMD_CATCH)) {
            set_gretl_errno(err);
            catch_local = 1;
        }
        gretl_echo_command(cmd, line, prn);
        errmsg(err, prn);
        err = catch_local ? 0 : err;
        goto cmd_finish;
    }

    gretl_exec_state_transcribe_flags(s, cmd);

    // if in batch mode, echo comments from input
    if (batch && runit < 2 && cmd->ci == CMD_COMMENT && gretl_if_state_true())
        if (gretl_echo_on() || gretl_comments_on()) printline(linebak);

    // nothing there, comment, or masked by "if"
    if (cmd->ci < 0) return 0;

    if (s->sys != nullptr && cmd->ci != END && cmd->ci != EQUATION && cmd->ci != SYSTEM) {
        std::printf(("Command '%s' ignored; not valid within equation system\n"), line);
        equation_system_destroy(s->sys);
        s->sys = nullptr;
        err = 1;
        goto cmd_finish;
    }

    if (cmd->ci == LOOP && !batch && !runit) pputs(prn, ("Enter commands for loop.  " "Type 'endloop' to get out\n"));

    // accumulating loop commands
    if (cmd->ci == LOOP || gretl_compiling_loop()) {
        // straight visual echo
        if (gretl_echo_on() && (!gretl_compiling_loop() || batch || runit)) gretl_echo_command(cmd, line, prn);
        err = gretl_loop_append_line(s, dset);
        if (err) {
            errmsg(err, prn);
        } else if (!batch && !runit) {
            gretl_record_command(cmd, line, cmdprn);
        }
        goto cmd_finish;
    }

    if (gretl_echo_on()&& !(s->flags & INIT_EXEC)) {
        // visual feedback, not recording
        if (cmd->ci == FUNC && runit > 1) {
            ; // don't echo
        } else if (batch || runit) {
            gretl_echo_command(cmd, line, prn);
        }
    }

    check_for_loop_only_options(cmd->ci, cmd->opt, prn);

cmd_proceed:

    gretl_exec_state_set_callback(s, cli_exec_callback, OPT_NONE);

    switch (cmd->ci) {

        case DELEET:
            err = gretl_delete_variables(cmd->list, cmd->param, cmd->opt, dset, &renumber, prn);
            if (err) {
                errmsg(err, prn);
            } else if (renumber && !batch) {
                pputs(prn, ("Take note: variables have been renumbered"));
                pputc(prn, '\n');
                maybe_list_series(dset, prn);
            }
            if (err && cmd->flags & CMD_CATCH) {
                //cmd->flags ^= CMD_CATCH;
                XOR_CMDFLAG(cmd->flags, CMD_CATCH);
                err = 0;
            }
            break;

        case HELP:
            cli_help(cmd->param, cmd->parm2, cmd->opt, nullptr, prn);
            break;

        case NULLDATA:
            if (cmd->order < 1) {
                err = 1;
                pputs(prn, ("Data series length count missing or invalid\n"));
            } else {
                cli_clear_data(s, dset);
                err = open_nulldata(dset, data_status, cmd->order, cmd->opt, prn);
                if (err) {
                    errmsg(err, prn);
                } else {
                    data_status = 1;
                }
            }
            break;

        case RUNLOOP:
            err = gretl_loop_exec(s, dset);
            break;

        case QUIT:
            gretl_if_state_clear();
            cli_quit(s, cmdprn, err);
            break;

        case RUN:
        case INCLUDE:
        case PKG:
            if (cmd->ci == PKG) {
                if (!strcmp(cmd->param, "run-sample")) {
                    err = cli_do_pkg_sample(cmd->parm2, runfile, prn);
                } else {
                    err = gretl_cmd_exec(s, dset);
                    break;
                }
            } else if (cmd->ci == INCLUDE) {
                err = get_full_filename(cmd->param, runfile, OPT_I);
            } else {
                err = get_full_filename(cmd->param, runfile, OPT_S);
            }
            if (err) {
                err = run_include_error(s, cmd->param, err, prn);
                break;
            }
            if (gretl_messages_on() && !(s->flags & INIT_EXEC)) pprintf(prn, " %s\n", runfile);
            if (cmd->ci == INCLUDE && gretl_is_xml_file(runfile)) {
                err = load_XML_functions_file(runfile, cmd->opt, prn);
                if (err) {
                    err = run_include_error(s, runfile, err, prn);
                } else {
                    pprintf(cmdprn, "include \"%s\"\n", runfile);
                }
                break;
            } else if (cmd->ci == INCLUDE && gfn_is_loaded(runfile)) {
                break;
            }
            if (!strcmp(runfile, s->runfile)) {
                pprintf(prn, ("Infinite loop detected in script\n"));
                err = 1;
                break;
            }
            if (fb != nullptr) push_input_file(fb);
            if ((fb = gretl_fopen(runfile, "r")) == nullptr) {
                pprintf(prn, ("Error reading %s\n"), runfile);
                err = process_command_error(s, E_FOPEN);
                fb = pop_input_file();
            } else {
                strcpy(s->runfile, runfile);
                gretl_set_script_dir(runfile);
                if (cmd->ci == INCLUDE) {
                    pprintf(cmdprn, "include \"%s\"\n", runfile);
                } else if (cmd->ci == PKG) {
                    pprintf(cmdprn, "pkg run-sample \"%s\"\n", cmd->parm2);
                } else {
                    pprintf(cmdprn, "run \"%s\"\n", runfile);
                }
                runit++;
                indent0 = gretl_if_state_record();
            }
            break;

            case CLEAR:
                err = incompatible_options(cmd->opt, static_cast<gretlopt>(OPT_A | OPT_D | OPT_F));
                if (!err) {
                    if (cmd->opt & OPT_F) {
                        gretl_functions_cleanup();
                    } else {
                        err = cli_clear_data(s, dset);
                    }
                }
                break;

            case DATAMOD:
                if (cmd->auxint == DS_CLEAR) {
                    err = cli_clear_data(s, dset);
                    pputs(prn, ("Dataset cleared\n"));
                    break;
                } else if (cmd->auxint == DS_RENUMBER) {
                    err = cli_renumber_series(cmd->list, cmd->parm2, dset, prn);
                    break;
                } else {
                    err = gretl_cmd_exec(s, dset);
                }
                break;

            default:
                err = gretl_cmd_exec(s, dset);
                break;
    }

    // record a successful interactive command
    if (!err && cmd->ci != QUIT && gretl_echo_on() && !batch && !old_runit) gretl_record_command(cmd, line, cmdprn);

cmd_finish:

    if (err) {
        gretl_exec_state_uncomment(s);
        if ((runit || batch) && cmd->ci != QUIT) cli_quit(s, cmdprn, err);
    }

    return err;
}

static int maybe_get_input_line_continuation (char *line)
{
    char tmp[MAXLINE];
    int contd, err = 0;

    if (!std::strncmp(line, "quit", 4)) return 0;
    contd = top_n_tail(line, MAXLINE, &err);

    while (contd && !err) {
        *tmp = '\0';

        if (batch || runit) {
            if (std::fgets(tmp, MAXLINE, fb) == nullptr) break;
        } else {
            if (std::fgets(tmp, MAXLINE, stdin) == nullptr) break;
        }

        if (*tmp != '\0') {
            if (std::strlen(line) + std::strlen(tmp) > MAXLINE - 1) {
                err = E_TOOLONG;
                break;
            } else {
                std::strcat(line, tmp);
                compress_spaces(line);
            }
        }
        contd = top_n_tail(line, MAXLINE, &err);
    }

    return err;
}

// T == std::filesystem::path: path to file to execute
// T == std::string: hansl code to be executed
template <typename T>
int gretl_execute_script (const T &source)
{
    //
    // apparatus for executing regular hansl code
    // note: this function acts as "main" in gretlcli.c
    //
    char linecopy[MAXLINE];
    DATASET *dset = nullptr;
    MODEL *model = nullptr;
    ExecState state;
    char line[MAXLINE] = {'\0'}; //was: char *line = nullptr;
    //int quiet = 0;    not used
    //int tool = 0;     not used
    int pkgmode = 0;
    int load_datafile = 1;
    char filearg[MAXLEN];
    char runfile[MAXLEN];
    //double scriptval = NADBL; not used
    CMD cmd;
    PRN *prn = global_printer.prn_ptr();
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

    // we assume "batch" mode every time
    SET_OPT(opt, OPT_BATCH);
    runfile[0] = filearg[0] = '\0';

    // either we read already existing file or write tempfile
    if constexpr (std::is_same_v<T, std::filesystem::path>) {
        std::snprintf(filearg, MAXLEN, "%s", source.string().c_str());
    } else if constexpr (std::is_same_v<T, std::string>) {
        auto res = gretl_generate_gretl_object("$gretldir", err, GRETL_OBJ_ANY, nullptr, nullptr);

        if (err) handle_gretl_error(err);
        if (auto *gretldir = std::get_if<std::string>(res.get())) {
            std::filesystem::path fullpath =
                std::filesystem::path(*gretldir) / "tmp" / (std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".inp");
            std::FILE *tmp_file = std::fopen(fullpath.string().c_str(), "w+");

            if (!tmp_file) handle_gretl_error(E_FOPEN, "failed to open temp file");
            std::fwrite(source.data(), 1, source.size(), tmp_file);
            std::fflush(tmp_file);
            std::fclose(tmp_file);
            std::snprintf(filearg, MAXLEN, "%s", fullpath.string().c_str());
        } else {
            throw std::runtime_error("didn't get $gretldir");
        }
    } else {
        static_assert(always_false<T>, "gretl_execute_script(): unsupported type T");
    }

    if (opt & (OPT_BATCH | OPT_RUNIT | OPT_MAKEPKG | OPT_INSTPKG)) {
        if (*filearg == '\0') {
            // we're missing a filename argument
            handle_gretl_error(E_INVARG, "No filename given");
            //usage(1);
        } else if ((opt & OPT_BATCH) && !strcmp(filearg, "-")) {
            // batch mode, but read from stdin
            /*quiet = batch_stdin = batch = 1;
            *runfile = '\0';
            load_datafile = 0;*/
            } else {
                // record argument (not a datafile)
                std::strcpy(runfile, filearg);
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

    // end of argc:argv

    /*if (!batch && !tool) {
        std::strcpy(cmdfile, gretl_workdir());
        std::strcat(cmdfile, "session.inp");
        cmdprn = gretl_print_new_with_filename(cmdfile, &err);
        if (err) {
            errmsg(err, prn);
            return EXIT_FAILURE;
        }
    }*/

    if (load_datafile) handle_datafile(filearg, runfile, dset, prn, cmdprn);

    gretl_cmd_init(&cmd);
    gretl_exec_state_init(&state, static_cast<ExecFlags>(0), line, &cmd, model, prn);

    /*if (!na(scriptval)) {
        // define "scriptopt"
        gretl_scalar_add("scriptopt", scriptval);
    }*/

    if (!batch) {
        // misc. interactive-mode setup
        //check_help_file();
        fb = stdin;
        push_input_file(fb);
        if (!runit && !data_status) std::fputs(("Type \"open filename\" to open a data set\n"), stdout);
    } else if (batch_stdin) {
        fb = stdin;
        push_input_file(fb);
    }

    if (batch || runit) {
        // re-initialize: will be incremented by "run" cmd
        runit = 0;
        if (*runfile != '\0' && pkgmode != OPT_INSTPKG) {
            if (std::strchr(runfile, ' ')) {
                std::sprintf(line, "run \"%s\"", runfile);
            } else {
                std::sprintf(line, "run %s", runfile);
            }
            if (gui_exec) OR_EXECFLAG(state.flags, INIT_EXEC);
            err = cli_exec_line(&state, dset, cmdprn);
            if (gui_exec) XOR_EXECFLAG(state.flags, INIT_EXEC);
            if (err && fb == nullptr) std::exit(EXIT_FAILURE);
        }
    }

    *linecopy = '\0';

    // enter main command loop
    while (cmd.ci != QUIT && fb != nullptr) {
        if (err && gretl_error_is_fatal()) gretl_abort(linecopy);
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
                  // no more input available
                  cli_exec_line(&state, dset, cmdprn);
                  if (runit == 0) {
                      err = gretl_if_state_check(0);
                      if (err) errmsg(err, prn);
                  }
                  continue;
              }
          }

          if (xout) {
              // readline Ctrl-X: get out without saving input or output
              gretl_print_destroy(cmdprn);
              gretl_remove(cmdfile);
              break;
          }

          if (!state.in_comment) {
              if (cmd.context == FOREIGN || cmd.context == MPI || gretl_compiling_python(line)) {
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
        std::strcpy(linecopy, line);
        tailstrip(linecopy);
        err = cli_exec_line(&state, dset, cmdprn);
      }
      // finished main command loop

    if (!err) {
        err = gretl_if_state_check(0);
        if (err) errmsg(err, prn);
    }

    if (!err && pkgmode) {
        if (pkgmode == OPT_MAKEPKG) {
            switch_ext(filearg, runfile, "gfn");
            std::sprintf(line, "makepkg %s\n", filearg);
        } else {
            if (!std::isalpha(filearg[0]) || filearg[1] == ':') {
                // some sort of path: install local file
                std::sprintf(line, "pkg install %s --local\n", filearg);
            } else {
                // plain filename or http: install from server
                std::sprintf(line, "pkg install %s\n", filearg);
            }
        }
        cli_exec_line(&state, dset, cmdprn);
    }

    gretl_object_unref(model, GRETL_OBJ_EQN);
    if (fb != stdin && fb != nullptr) std::fclose(fb);
    gretl_cmd_free(&cmd);
    if (err) handle_gretl_error(err);
    global_printer.print();

    return err;
}
template int gretl_execute_script<std::filesystem::path>(const std::filesystem::path&);
template int gretl_execute_script<std::string>(const std::string&);
