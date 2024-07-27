#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

enum typecodes {
    gretl_clist,
    gretl_list,
    gretl_dset,
    gretl_opt,
    gretl_prn,
    gretl_other
};

enum basics {
    B_TRUE     = 1 << 0,
    B_NONCONST = 1 << 1,
    B_NO_PRN   = 1 << 2
};

struct arg_data {
    char argtype[32];
    char argname[32];
    int typecode;
};

struct model_func {
    char gretl_funcname[32];
    int argc;
    struct arg_data *args;
    int basic;
};

FILE *calls_h;
FILE *calls_cpp;
FILE *mod_cpp;

struct mf_info {
    const char *gretl_funcname;
    const char *cmdword;
    const char *symbol;
};

struct mf_info mf_infos[] = {
    { "ar_model", "ar", "AR" },
    { "ar1_model", "ar1", "AR1" },
    { "biprobit_model", "biprobit", "BIPROBIT" },
    { "duration_model", "duration", "DURATION" },
    { "garch", "garch", "GARCH" },
    { "heckit_model", "heckit", "HECKIT" },
    { "hsk_model" "hsk", "HSK" },
    { "interval_model" "intreg", "INTREG" },
    { "lad_model", "lad", "LAD" },
    { "logistic_driver", "logistic", "LOGISTIC" },
    { "mp_ols", "mpols", "MPOLS" },
    { "panel_model", "panel", "PANEL" },
    { "ivreg", "tsls", "IVREG" },
    { NULL, NULL, NULL }
};

const struct mf_info *get_mf_info (const char *func)
{
    int i;

    for (i=0; mf_infos[i].gretl_funcname != NULL; i++) {
        if (!strcmp(func, mf_infos[i].gretl_funcname)) {
            return &mf_infos[i];
        }
    }

    return NULL;
}

/* numeric codes for common model-function argument types */

int get_type_code (const char *argtype)
{
    if (!strcmp(argtype, "const int *")) {
        return gretl_clist;
    } else if (!strcmp(argtype, "int *")) {
        return gretl_list;
    } else if (!strcmp(argtype, "DATASET *")) {
        return gretl_dset;
    } else if (!strcmp(argtype, "gretlopt")) {
        return gretl_opt;
    } else if (!strcmp(argtype, "PRN *")) {
        return gretl_prn;
    } else {
        return gretl_other;
    }
}

/* Determine whether a model function can be slotted into
   the existing regression_wrapper() in gretl_calls.cpp
*/

int has_basic_model_sig (struct model_func *mf)
{
    int ret = 0;

    if (mf->argc == 4 || mf->argc == 3) {
        ret = B_TRUE;
        if ((mf->args[0].typecode != gretl_clist &&
             mf->args[0].typecode != gretl_list) ||
            mf->args[1].typecode != gretl_dset ||
            mf->args[2].typecode != gretl_opt) {
            ret = 0;
        } else if (mf->argc == 4 && mf->args[3].typecode != gretl_prn) {
            ret = 0;
        }
        if (ret && mf->args[0].typecode != gretl_clist) {
            ret |= B_NONCONST;
        }
        if (ret && mf->argc == 3) {
            ret |= B_NO_PRN;
        }
    }

    return ret;
}

void model_func_init (struct model_func *mf,
                      const char *name,
                      int argc)
{

    strcpy(mf->gretl_funcname, name);
    mf->argc = argc;
    mf->args = calloc(argc, sizeof *mf->args);
}

void model_func_clear (struct model_func *mf)
{
    free(mf->args);
}

/* output block for inclusion in gretl_to_python.cpp */

void output_basic_module_info (const struct mf_info *mfi)
{
    fprintf(mod_cpp,
            "    m.def(\"%s\", &%s,\n"
            "          py::arg(\"formula\").none(false),\n"
            "          py::return_value_policy::take_ownership);\n",
            mfi->cmdword, mfi->cmdword);
}

/* output signature for inclusion in gretl_calls.h */

void output_basic_pysig (const struct mf_info *mfi, FILE *fp)
{
    fprintf(fp,
            "\npy::dict *%s (const std::string formula, const py::kwargs &kwargs)",
            mfi->cmdword);
    if (fp == calls_h) {
        fputs(";\n", fp);
    } else {
        fputc('\n', fp);
    }
}

/* output code block for inclusion in gretl_calls.cpp */

void output_basic_pywrap (const struct model_func *mf,
                          const struct mf_info *mfi)
{
    output_basic_pysig(mfi, calls_cpp);

    if (mf->basic == B_TRUE) {
        fprintf(calls_cpp,
                "{\n    return regression_wrapper(formula, kwargs, %s, &%s, NULL);\n}\n",
                mfi->symbol, mf->gretl_funcname);
    } else {
        fprintf(calls_cpp,
                "{\n    return regression_wrapper(formula, kwargs, %s, NULL, &%s);\n}\n",
                mfi->symbol, mf->gretl_funcname);
    }
}

int skip_it (const char *name)
{
    if (!strcmp(name, "ols") ||
        !strcmp(name, "logit") ||
        !strcmp(name, "probit") ||
        !strncmp(name, "binary_", 7) ||
        !strncmp(name, "multino", 7) ||
        !strncmp(name, "ordered_", 8)) {
        /* these are special, and already handled */
        return 1;
    } else if (!strncmp(name, "real_", 5) ||
               !strncmp(name, "single_", 6) ||
               !strcmp(name, "GNR")) {
        /* don't expose these? */
        return 1;
    } else {
        return 0;
    }
}

void model_func_print (struct model_func *mf)
{
    int i;

    mf->basic = has_basic_model_sig(mf);

    /* note: "basic" means that the model function signature
       is such that the function can be handled by the existing
       regression_wrapper() in gretl_calls.cpp
    */

    /* show what we got from the header */
    printf("\nMODEL function %s (basic =", mf->gretl_funcname);
    if (mf->basic == 0) {
        printf(" false");
    } else if (mf->basic == B_TRUE) {
        printf(" true");
    } else {
        if (mf->basic & B_NONCONST) {
            printf(" non-const");
        }
        if (mf->basic & B_NO_PRN) {
            printf(" no_prn");
        }
    }
    printf("):\n");

    for (i=0; i<mf->argc; i++) {
        printf("  arg %d: type '%s', name '%s'\n", i,
               mf->args[i].argtype, mf->args[i].argname);
    }

    if (mf->basic && !skip_it(mf->gretl_funcname)) {
        const struct mf_info *mfi = get_mf_info(mf->gretl_funcname);

        if (mfi != NULL) {
            output_basic_module_info(mfi);
            output_basic_pysig(mfi, calls_h);
            output_basic_pywrap(mf, mfi);
        }
    }
}

int process_args (const char *name, GString *gs)
{
    struct model_func mf = {0};
    gchar **args;
    gchar *ai, *p;
    int i, n;

    /* get array of comma-separated argument specs */
    args = g_strsplit(gs->str, ",", 0);
    n = g_strv_length(args);

    model_func_init(&mf, name, n);

    /* record the types and names of the arguments */
    for (i=0; i<n; i++) {
        ai = g_strstrip(args[i]);
        if ((p = strrchr(ai, '*')) != NULL) {
            strncpy(mf.args[i].argtype, ai, p-ai+1);
            strcpy(mf.args[i].argname, p+1);
        } else if ((p = strrchr(ai, ' ')) != NULL) {
            strncpy(mf.args[i].argtype, ai, p-ai);
            strcpy(mf.args[i].argname, p+1);
        }
        mf.args[i].typecode = get_type_code(mf.args[i].argtype);
    }

    model_func_print(&mf);
    model_func_clear(&mf);
    g_strfreev(args);

    return 0;
}

void trim_line (char *line)
{
    int i, n = strlen(line);

    for (i=n-1; i>0; i--) {
        if (line[i] == ')' || line[i] == ';' || line[i] == '\n') {
            line[i] = '\0';
        } else {
            break;
        }
    }
}

int process_header (FILE *fp)
{
    GString *args;
    char line[1024];
    char fnname[64];
    char *s, *rp;

    while (fgets(line, sizeof line, fp)) {
        if (!strncmp(line, "MODEL ", 6)) {
            if (line[6] == '*') {
                /* disregard funcs that return pointer-to-model */
                continue;
            }
            s = line + 6;
            sscanf(s, "%s", fnname);
            s = strchr(s, '(');
            if (s == NULL) {
                continue;
            } else {
                s++;
            }
            /* look for right parenthesis */
            rp = strchr(s, ')');
            trim_line(s);
            args = g_string_new(s);
            while (rp == NULL) {
                /* the signature continues on the next line */
                fgets(line, sizeof line, fp);
                rp = strchr(line, ')');
                trim_line(line);
                args = g_string_append(args, g_strchug(line));
            }
            process_args(fnname, args);
            g_string_free(args, TRUE);
        }
    }

    return 0;
}

/* Carry out an analysis of a gretl header, looking for functions
   that return a MODEL struct.
*/

int main (int argc, char **argv)
{
    FILE *fp;

    if (argc < 2) {
        fprintf(stderr, "%s: supply a gretl header filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "%s: couldn't open %s\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }

    calls_h =   fopen("more_calls.h", "a");
    calls_cpp = fopen("more_calls.cpp", "a");
    mod_cpp =   fopen("more_mod.cpp", "a");

    if (calls_h == NULL || calls_cpp == NULL || mod_cpp == NULL) {
        fprintf(stderr, "%s: failed to open output files\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    process_header(fp);

    fclose(fp);
    fclose(calls_h);
    fclose(calls_cpp);
    fclose(mod_cpp);

    return 0;
}
