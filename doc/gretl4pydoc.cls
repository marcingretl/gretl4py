\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{gretl4pydoc}
\DeclareOption*{
	\PassOptionsToClass{\CurrentOption}{report}}
\ProcessOptions
\LoadClass[a4paper,11pt,twoside]{report}
\RequirePackage[utf8]{inputenc}
\RequirePackage[pdftex]{xcolor}
\RequirePackage[]{graphicx}

\RequirePackage{dirtree,xargs,xspace,url,pifont,makeidx,ifthen,caption,subcaption,fancyvrb,tikz,pdflscape,catchfile,amsmath,xstring}
\RequirePackage[toc,title]{appendix}
\RequirePackage[final]{listings}
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
	basicstyle=\ttfamily\small,	  % Use a monospaced font 
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
	morekeywords=[2]{
		int,
		float,
		complex,
		bool,
		str,
		bytes,
		list,
		tuple,
		set,
		dict,
		object
	},
	keywordstyle=[2]\color{pythtype},
	%emph={MyClass,__init__},      % Custom highlighting
	emphstyle=\color{red},     % Custom highlighting style
	stringstyle=\color{pythgreen},
	frame=tb,                      % Any extra options here
	showstringspaces=false,
	literate={{~}{$\sim$}2
		{x1}{$x_1$}2
		{x2}{$x_2$}2
		{xk}{$x_k$}2
		{.set}{.set}{3}
		{.type}{type}{4}
		{gretl.Model}{{\textcolor{pythtype}{gretl.Model}}}{11}
		{gretl.Matrix}{{\textcolor{pythtype}{gretl.Matrix}}}{12}
		{gretl.Bundle}{{\textcolor{pythtype}{gretl.Bundle}}}{12}
		{gretl.Dataset}{{\textcolor{pythtype}{gretl.Dataset}}}{13}
		{gretl.Package}{{\textcolor{pythtype}{gretl.Package}}}{13}
		{numpy.ndarray}{{\textcolor{pythtype}{numpy.ndarray}}}{13}
		{gretl.Model\_var}{{\textcolor{pythtype}{gretl.Model\_var}}}{16}
		{gretl.Model\_NSE}{{\textcolor{pythtype}{gretl.Model\_NSE}}}{16}
		{gretl.Model\_NME}{{\textcolor{pythtype}{gretl.Model\_NME}}}{16}
		{gretl.Model\_vecm}{{\textcolor{pythtype}{gretl.Model\_vecm}}}{17}
		{gretl.Model\_NSE\_nl}{{\textcolor{pythtype}{gretl.Model\_NSE\_nl}}}{20}
		{pandas.core.series.Series}{{\textcolor{pythtype}{pandas.core.series.Series}}}{25}
		}
}

\lstdefinestyle{pythonstylebold}{
	language=Python,
	basicstyle=\bf\small,
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
		{xk}{$x_k$}1
		{.type}{{\textcolor{black}{type}}}{4}
		{gretl.Matrix}{{\textcolor{pythtype}{gretl.Matrix}}}{12}
		{gretl.Bundle}{{\textcolor{pythtype}{gretl.Bundle}}}{12}
		{gretl.Dataset}{{\textcolor{pythtype}{gretl.Dataset}}}{13}
		{pandas.core.series.Series}{{\textcolor{pythtype}{pandas.core.series.Series}}}{25}
		{numpy.ndarray}{{\textcolor{pythtype}{numpy.ndarray}}}{13}}
}

\lstdefinestyle{pythonstylefoot}{
	language=Python,
	basicstyle=\ttfamily\footnotesize,
	morekeywords={self,True,False,None},
	keywordstyle=\color{blue},
	morekeywords=[2]{
		int,
		float,
		complex,
		bool,
		str,
		bytes,
		list,
		tuple,
		set,
		dict,
		object
	},
	keywordstyle=[2]\color{pythtype},
	%emph={MyClass,__init__},      % Custom highlighting
	emphstyle=\color{red},     % Custom highlighting style
	stringstyle=\color{pythgreen},
	frame=tb,                      % Any extra options here
	showstringspaces=false,
	literate={
		{~}{$\sim$}1
		{x1}{$x_1$}2
		{x2}{$x_2$}2
		{xk}{$x_k$}2
		{.if}{if }3
		{.id}{.id }3
		{.set}{set}{3}
		{.type}{type}{4}
		{.vars}{vars}{4}
		{.iter}{iter}{4}
		{.list}{list }5
		{-sum}{-sum}{4}
		{all=}{all=}{4}
		{.print}{print}{5}
		{.range}{range}{5}
		{gretl.Matrix}{{\textcolor{pythtype}{gretl.Matrix}}}{12}
		{gretl.Bundle}{{\textcolor{pythtype}{gretl.Bundle}}}{12}
		{~gretl.Bundle}{gretl.Bundle}{13}
		{~gretl.Matrix}{gretl.Matrix}{13}
		{gretl.Dataset}{{\textcolor{pythtype}{gretl.Dataset}}}{13}
		{gretl.Model\_hsk}{{\textcolor{pythtype}{gretl.Model\_hsk}}}{17}
		{gretl.Model\_lad}{{\textcolor{pythtype}{gretl.Model\_lad}}}{17}
		{gretl.Model\_nls}{{\textcolor{pythtype}{gretl.Model\_nls}}}{17}
		{gretl.Model\_ols}{{\textcolor{pythtype}{gretl.Model\_ols}}}{17}
		{gretl.Model\_wls}{{\textcolor{pythtype}{gretl.Model\_wls}}}{17}
		{gretl.Model\_var}{{\textcolor{pythtype}{gretl.Model\_var}}}{17}
		{gretl.Model\_NME}{{\textcolor{pythtype}{gretl.Model\_NME}}}{17}
		{gretl.Model\_tsls}{{\textcolor{pythtype}{gretl.Model\_tsls}}}{18}
		{gretl.Model\_vecm}{{\textcolor{pythtype}{gretl.Model\_vecm}}}{18}
		{gretl.Model\_garch}{{\textcolor{pythtype}{gretl.Model\_garch}}}{19}
		{gretl.Model\_mpols}{{\textcolor{pythtype}{gretl.Model\_mpols}}}{19}
		{gretl.Model\_logit}{{\textcolor{pythtype}{gretl.Model\_logit}}}{19}
		{gretl.Model\_panel}{{\textcolor{pythtype}{gretl.Model\_panel}}}{19}
		{gretl.Model\_tobit}{{\textcolor{pythtype}{gretl.Model\_tobit}}}{19}
		{gretl.Model\_heckit}{{\textcolor{pythtype}{gretl.Model\_heckit}}}{20}
		{gretl.Model\_dpanel}{{\textcolor{pythtype}{gretl.Model\_dpanel}}}{20}
		{gretl.Model\_intreg}{{\textcolor{pythtype}{gretl.Model\_intreg}}}{20}
		{gretl.Model\_negbin}{{\textcolor{pythtype}{gretl.Model\_negbin}}}{20}
		{gretl.Model\_probit}{{\textcolor{pythtype}{gretl.Model\_probit}}}{20}
		{gretl.Model\_poisson}{{\textcolor{pythtype}{gretl.Model\_poisson}}}{21}
		{gretl.Model\_duration}{{\textcolor{pythtype}{gretl.Model\_duration}}}{22}
		{gretl.Model\_biprobit}{{\textcolor{pythtype}{gretl.Model\_biprobit}}}{22}
		{gretl.Model\_logistic}{{\textcolor{pythtype}{gretl.Model\_logistic}}}{22}
		{gretl.Model\_midasreg}{{\textcolor{pythtype}{gretl.Model\_midasreg}}}{22}
		{gretl.Model\_quantreg}{{\textcolor{pythtype}{gretl.Model\_quantreg}}}{22}
		{pandas.core.series.Series}{{\textcolor{pythtype}{pandas.core.series.Series}}}{25}
		{pandas.core.frame.DataFrame}{{\textcolor{pythtype}{pandas.core.frame.DataFrame}}}{27}
		{numpy.ndarray}{{\textcolor{pythtype}{numpy.ndarray}}}{13}
		{numpy.array}{{\textcolor{pythtype}{numpy.array}}}{11}}}

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
		wls, xcorrgm, xtab, mpi, gibbs, return, params, pkg, midasreg, johansen, bkw, panspec, stdize},
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
		xmin, xpx, zeromiss, zeros,
		bwrite, bread, mpibcast, mpireduce, array, defbundle,
		mat2list, strftime, fzero, tdisagg, _},
	morekeywords=[3]{bool, int, list, matrix, scalar, series, string, bundle,
		void, bundles, null, const},
	morekeywords=[4]{\$nobs, \$stopwatch, \$sysinfo, \$now, \$model, \$result,
		\$uhat, \$yhat, \$version, \$workdir, \$h, \$ahat, \$ec, \$gretldir,
		\$fcast, \$fcse},
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
\lstdefinestyle{hansllarge}{
	belowcaptionskip=1\baselineskip,
	breaklines=true,
	frame=L,
	xleftmargin=\parindent,
	language=Hansl,
	showstringspaces=false,
	basicstyle={\ttfamily\large},
	keywordstyle={\bfseries\color{hanslred}},
	keywordstyle=[2]{\color{hanslteal}},
	keywordstyle=[3]{\bfseries\color{hanslgreen}},
	keywordstyle=[4]{\bfseries\color{hanslgreen}},
	commentstyle={\color{hanslblue}},
	stringstyle={\color{hanslmagenta}}
}
\lstdefinestyle{hanslfoot}{
	belowcaptionskip=1\baselineskip,
	breaklines=true,
	frame=L,
	xleftmargin=\parindent,
	language=Hansl,
	showstringspaces=false,
	basicstyle={\ttfamily\footnotesize},
	keywordstyle={\bfseries\color{hanslred}},
	keywordstyle=[2]{\color{hanslteal}},
	keywordstyle=[3]{\bfseries\color{hanslgreen}},
	keywordstyle=[4]{\bfseries\color{hanslgreen}},
	commentstyle={\color{hanslblue}},
	stringstyle={\color{hanslmagenta}}
}
\lstset{
	columns=fullflexible,
	escapechar=@,
	style=hansl,
	literate=
	{ +i }{+i }{4}
	{.*}{ * }{2}
	{run-}{run-}{4}
	{.rows}{rows}{5}
	{.cols}{cols}{5}
	{~lags}{lags }5
	{~freq}{freq }5
	{pvalue(}{{\textcolor{hanslteal}{pvalue}(}}7
	{~sortby}{sortby }7
	{~varlist}{varlist }8
	{~varname}{varname }8
	{~series}{series}7
	{~series~}{series }8
	{~resample}{resample }9
	{~equation}{equation }9
}
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
\definecolor{pythtype}{cmyk}{.25,.45,0,0}
