\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{gretl4pydoc}
\DeclareOption*{
	\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions
\LoadClass[a4paper,11pt,twoside]{article}
\RequirePackage[utf8]{inputenc}
\RequirePackage[pdftex]{xcolor}
\RequirePackage[]{graphicx}

\RequirePackage{dirtree,xargs,xspace,url,listings,pifont,makeidx,ifthen,caption,subcaption,fancyvrb,tikz,pdflscape}
\RequirePackage[toc,title]{appendix}
\usepackage{gretlwp}

\usepackage[colorlinks,pdfusetitle]{hyperref}
\hypersetup{citecolor=magenta}
\usepackage[authoryear]{natbib}
\usetikzlibrary{positioning, arrows.meta, matrix}

\newcommand{\proglang}[1]{\texttt{#1}}
\newcommand{\pkg}[1]{{\fontseries{b}\selectfont #1}}
% \newcommand{\code}{\bgroup\@makeother\_\@makeother\~\@makeother\$\@codex}
\newcommand{\E}{\mathsf{E}}
\newcommand{\VAR}{\mathsf{VAR}}
\newcommand{\COV}{\mathsf{COV}}
\newcommand{\Prob}{\mathsf{P}}
\newcommand{\Code}[1]{\texttt{#1}}

\newenvironment{CodeChunk}{}{}

% \DefineVerbatimEnvironment{Code}{Verbatim}{}
\DefineVerbatimEnvironment{CodeInput}{Verbatim}{fontshape=sl,fontsize=\small}
\DefineVerbatimEnvironment{CodeOutput}{Verbatim}{}

\author{Marcin Błażejowski\\
	{\small \url{marcin@gretlconference.org}}\\
	Allin Cottrell\\
	{\small \url{cottrell@wfu.edu}}\\
	{\centering and}\\
	\pkg{gretl} developers (dream) Team}
\title{\pkg{gretl4py}: a package for calling \pkg{libgretl} from \proglang{Python}}
\prognum{6}

\date{\today}

\lstloadlanguages{[11]C++, [3]Python}

% C++ style
\lstdefinestyle{cppstyle}{
	language=[11]C++,
	basicstyle=\ttfamily\small,  % Use a monospaced font 
	keywordstyle=\color{blue},    % Keywords in blue 
	commentstyle=\color{grey},    % Comments in green 
	stringstyle=\color{red},      % Strings in red 
	numbers=left,                 % Line numbers on the left 
	numberstyle=\tiny\color{gray}, % Style for line numbers 
	stepnumber=1,                 % Number every line 
	numbersep=5pt,                % Distance between line numbers and code 
	frame=L,
	backgroundcolor=\color{white}, % Background color 
	showstringspaces=false,       % Don't show spaces in strings 
	tabsize=2                     % Tab size
}

\lstdefinestyle{pythonstyle}{
	language=Python,
	basicstyle=\ttfamily\small,
	morekeywords={self,True,False},
	keywordstyle=\color{blue},
	%emph={MyClass,__init__},      % Custom highlighting
	emphstyle=\color{red},     % Custom highlighting style
	stringstyle=\color{pythgreen},
	frame=tb,                      % Any extra options here
	showstringspaces=false,
	literate={{~}{$\sim$}2
		{x1}{$x_1$}3
		{x2}{$x_2$}2
		{xk}{$x_k$}1}
}

%% Hansl syntax highlight
\lstdefinelanguage{Hansl}{
	morekeywords={add, adf, anova, append, ar, ar1, arbond, arch, arima, bds, biprobit, boxplot, break,
		catch, chow, clear, coeffsum, coint, coint2, corr, corrgm, cusum, data, dataset, debug,
		delete, diff, difftest, discrete, dpanel, dummify, duration,  elif, else, end, endif, endloop,
		eqnprint, equation, estimate, fcast, foreign, fractint, freq, function, garch, genr, gmm, gnuplot,
		graphpg, hausman, heckit, help, hsk, hurst, if, include, info, intreg, join, kalman, kpss, labels,
		lad, lags, ldiff, leverage, levinlin, logistic, logit, logs, loop, mahal, makepkg, markers,
		meantest, mle, modeltab, modprint, modtest, mpols, negbin, nls, normtest, nulldata, ols,
		omit, open, orthdev, outfile, panel, pca, pergm, poisson, print, printf, probit, pvalue,
		qlrtest, qqplot, quantreg, quit, rename, reset, restrict, rmplot, run, runs, scatters,
		sdiff, set, setinfo, setobs, setmiss, shell, smpl, spearman, sprintf, square, sscanf, store,
		summary, system, tabprint, textplot, tobit, tsls, var, varlist, vartest, vecm, vif,
		wls, xcorrgm, xtab, mpi, gibbs, return, params, pkg, midasreg, johansen},
	morekeywords=[2]{abs, acos, acosh, aggregate, argname, asin, asinh, atan, atanh, bessel,
		BFGSmax, bkfilt, boxcox, bwfilt, cdemean, cdf, cdiv, ceil, cholesky, chowlin,
		cmult, cnorm, colname, colnames, cols, cos, cosh, cov,
		critical, cum, deseas, det, diag, diagcat, digamma, dnorm, dsort,
		eigengen, eigensym, eigsolve, epochday, errmsg, exp, fcstats, fdjac, fft,
		ffti, filter, firstobs, floor, fracdiff, gammafun, getenv, ghk, gini, ginv,
		halton, hdprod, hpfilt, I, imaxc, imaxr, imhof, iminc, iminr, inbundle,
		infnorm, inlist, int(, inv, invcdf, invmills, invpd, irf, irr, isconst,
		isnan, isnull, kdensity, kfilter, ksimul, ksmooth, kurtosis, lastobs, ldet,
		lincomb, ljungbox, lngamma, log, log10, log2, loess, lower,
		lrvar, max, maxc, maxr, mcorr, mcov, mcovg, mean, meanc, meanr,
		median, mexp, min, minc, minr, missing, misszero, mlag, mnormal, mols,
		monthlen, movavg, mrandgen, mread, mreverse, mrls, mshape, msortby, muniform,
		mwrite, mxtab, nadarwat, nelem, ngetenv, nobs, normal, npv, NRmax, nullspace,
		obs, obslabel, obsnum, ok, onenorm, ones, pdf, pmax,
		pmean, pmin, pnobs, polroots, polyfit, princomp, psd, psdroot, pshrink,
		pxsum, qform, qnorm, qrdecomp, quadtable, quantile, randgen, randgen1, randint, rank,
		ranking, rcond, readfile, remove, replace, resample, round, rownames, rows, sd,
		sdc, selifc, selifr, seq, simann, sin, sinh, skewness, sort,
		sortby, sqrt, sst, strlen, strncmp, strsplit, strstr, strstrip, strsub,
		sum, sumall, sumc, sumr, svd, tan, tanh, toepsolv, tolower, tr,
		transp, trimr, typestr, uniform, uniq, unvech, upper, urcpval, values,
		varname, varnum, varsimul, vec, vech, weekday, wmean, wsd, wvar, xmax,
		xmin, xpx, zeromiss, zeros, bwrite, bread, mpibcast, mpireduce, array, defbundle,
		mat2list, strftime, fzero, _},
	morekeywords=[3]{bool, int, list, matrix, scalar, series, string, bundle,
		void, bundles, null, const},
	morekeywords=[4]{\$nobs, \$stopwatch, \$sysinfo, \$now, \$model},
	%   alsoletter={-},
	sensitive=false,
	morecomment=[l]{\#},
	morecomment=[l][\color{hanslgreen}]{--},
	morecomment=[s]{/*}{*/},
	morestring=[b]"
}
\lstdefinestyle{hansl}{
	belowcaptionskip=1\baselineskip,
	breaklines=true,
	frame=L,
	xleftmargin=\parindent,
	language=Hansl,
	showstringspaces=false,
	basicstyle={\ttfamily\small},
	keywordstyle={\bfseries\color{hanslred}},
	keywordstyle=[2]{\color{hanslteal}},
	keywordstyle=[3]{\bfseries\color{hanslgreen}},
	keywordstyle=[4]{\bfseries\color{hanslgreen}},
	commentstyle={\color{hanslblue}},
	stringstyle={\color{hanslmagenta}}
}
\lstset{escapechar=@,style=hansl}
\def\inline{\lstinline[basicstyle={\normalsize\ttfamily}]}
% Colors
% \definecolor{hanslgreen}{cmyk}{.67,0,.37,.45}
\definecolor{hanslgreen}{cmyk}{.87,0,.91,.45}
\definecolor{hanslred}{cmyk}{0,.75,.75,.35}
\definecolor{hanslblue}{cmyk}{1,1,0,0} % {rgb}{0,0,1}
\definecolor{hanslteal}{cmyk}{1,0,.45,0} %{.01,0,.45,1}
\definecolor{hanslmagenta}{cmyk}{0,.96,0,0}
\definecolor{hanslblack}{cmyk}{0,0,0,1}
\definecolor{pythgreen}{cmyk}{.65,0,.65,.35}
