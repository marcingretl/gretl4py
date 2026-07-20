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

/*
 * gretl4py internal definitions for safe C++ integration with libgretl
 *
 * WARNING: These structures (CMD_, ExecState_) mirror private libgretl types.
 * If the upstream libgretl headers (e.g., cmd_private.h) change — even adding
 * a single field — this will **break the ABI**. Using mismatched versions may
 * cause segmentation faults, memory corruption, or crashes.
 *
 * Any update to cmd_private.h must be manually reflected here before recompiling
 * gretl4py to maintain ABI compatibility.
 */

#ifndef GRETL4PY_INTERNALS_H
#define GRETL4PY_INTERNALS_H

extern "C" {
#include <gretl/gretl_restrict.h>

typedef struct LOOPSET_ LOOPSET;

typedef struct cmd_token_ cmd_token;

typedef int (*EXEC_CALLBACK) (ExecState *, void *, GretlObjType type);

typedef enum {
    CMD_CCMT    = 1 << 0, /* line is in a C-style multi-line comment */
    CMD_SUBST   = 1 << 1, /* string substitution has been done on command */
    CMD_PROG    = 1 << 2, /* command is in context of progressive loop */
    CMD_CATCH   = 1 << 3, /* error from command should be "caught" */
    CMD_NOSUB   = 1 << 4, /* no @-substitution called for (pre-checked) */
    CMD_ENDFUN  = 1 << 5  /* line terminates a function definition */
} CmdFlags;

struct CMD_ {
    int ci;          /* current command index */
    int err;         /* error code */
    int context;     /* for block commands, index of current context */
    int ciflags;     /* see CIFlags in tokenizer.c */
    gretlopt opt;    /* option(s) for command */
    CmdFlags flags;  /* status flags for command invocation */
    GretlType gtype; /* specified type for "genr" */
    int order;       /* lag order, where appropriate */
    int auxint;      /* auxiliary int (e.g. VECM rank) */
    int cstart;      /* token index of start of 'real' command */
    int ntoks;       /* number of tokens actually used */
    int nt_alloced;  /* number of tokens allocated */
    cmd_token *toks; /* tokens */
    char *vstart;    /* portion of line where varargs or expr start */
    char *param;     /* basic parameter string */
    char *parm2;     /* second parameter string */
    int *list;       /* list of series and/or control integers */
    int *auxlist;    /* needed for "gappy" lag lists */
    char savename[MAXSAVENAME]; /* for object-saving mechanism */
};

struct ExecState_ {
    ExecFlags flags;
    CMD *cmd;
    PRN *prn;
    char *line;
    char runfile[MAXLEN];
    MODEL *model;          /* "workspace" model */
    MODEL *pmod;           /* set if new model is estimated */
    LOOPSET *loop;
    equation_system *sys;
    gretl_restriction *rset;
    GRETL_VAR *var;
    void *prev_model;
    GretlObjType prev_type;
    int prev_model_count;
    char *submask;        /* record of incoming sub-sample for functions */
    guint8 padded;        /* record of incoming panel padding, if any */
    guint8 in_comment;
    guint8 free_line;
    EXEC_CALLBACK callback;
};

struct OpenOp_ {
    char fname[MAXLEN];
    int quiet;
    int http;
    int dbdata;
    int ftype;
};

typedef struct OpenOp_ OpenOp;

struct tdisagg_info {
    int ynum, xnum;            /* series IDs, if applicable */
    const int *ylist, *xlist;  /* series lists, if applicable */
    const double *yval, *xval; /* series values */
    gretl_matrix *Y, *X;       /* matrix values */
    int xmidas;                /* 0/1: X given as a MIDAS list? */
    int efac;                  /* expansion factor */
    int extmax;                /* max # of extrapolated obs */
    GretlType targ;            /* target return type */
};

int gretl_if_state_true (void);

void gretl_if_state_clear (void);

void gretl_if_state_reset (int indent);

int gretl_if_state_check (int indent0);

int gretl_if_state_record (void);

void gretl_exec_state_transcribe_flags (ExecState *s, CMD *cmd);

int csv_open_needs_matrix (gretlopt opt);

void gretl_exec_state_set_callback (ExecState *s, EXEC_CALLBACK callback, gretlopt opt);

int process_command_error (ExecState *s, int err);

void gretl_exec_state_init (ExecState *s, ExecFlags flags, char *line, CMD *cmd, MODEL *model, PRN *prn);

void gretl_exec_state_uncomment (ExecState *s);

int cli_help (const char *hlpword, const char *param, gretlopt opt, char **pbuf, PRN *prn);

gretl_matrix *get_tdisagg_matrix (struct tdisagg_info *tdi, DATASET *dset, gretl_bundle *b,
                                  gretl_bundle *r, PRN *prn, int *err);
}

#endif // GRETL4PY_INTERNALS_H
