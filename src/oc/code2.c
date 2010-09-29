#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/code2.c,v 1.12 1999/06/08 17:48:26 hines Exp */

#include	"hoc.h"
#include "hocstr.h"
#include	"parse.h"
#include	<stdio.h>
#include <stdlib.h>
#include	<errno.h>

int units_on_flag_;

extern FILE *fin;
extern char **gargv;
extern int gargc;
extern double chkarg();
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern Symbol *hoc_table_lookup();

extern char **hoc_pgargstr();

float* hoc_sym_domain(sym) Symbol* sym; {
	if (sym && sym->extra) {
		return sym->extra->parmlimits;
	}
	return (float*)0;
}

HocSymExtension* hoc_var_extra(name) char* name; {
	Symbol* sym;
	sym = hoc_lookup(name);
	if (sym) {
		return sym->extra;
	}else{
		return (HocSymExtension*)0;
	}
}

hoc_Symbol_limits() {
	Symbol* sym, *hoc_get_last_pointer_symbol();
	double* hoc_pgetarg();

	hoc_pgetarg(1);
	sym = hoc_get_last_pointer_symbol();
	assert(sym);
	hoc_symbol_limits(sym, *getarg(2), *getarg(3));
	ret();
	pushx(1.);
}

hoc_symbol_limits(sym, low, high)
	Symbol* sym;
	float low, high;
{
	sym_extra_alloc(sym);
	if (!sym->extra->parmlimits) {
		sym->extra->parmlimits = (float*)emalloc(2*sizeof(float));
	}
	sym->extra->parmlimits[0] = low;
	sym->extra->parmlimits[1] = high;
}

hoc_symbol_tolerance(sym, tol)
	Symbol* sym;
	double tol;
{
	sym_extra_alloc(sym);
	sym->extra->tolerance = tol;
}

double check_domain_limits(limits, val)
	float* limits;
	double val;
{
	if (limits) {
		if (val < limits[0]) {
			return (double)limits[0];
		}else if (val > limits[1]) {
			return (double)limits[1];
		}
	}
	return val; 
}


char* hoc_symbol_units(sym, units) Symbol* sym; char* units; {
	if (!sym) { return (char*)0; }
	if (units) {
		if (sym->extra && sym->extra->units) {
			free(sym->extra->units);
			sym->extra->units = (char*)0;
		}
		sym_extra_alloc(sym);
		sym->extra->units = (char*)emalloc(strlen(units)+1);
		strcpy(sym->extra->units, units);
	}
	if (sym->extra && sym->extra->units) {
		return sym->extra->units;
	}else{
		return (char*)0;
	}
}

Symbol* hoc_name2sym(char* name) {
	char* buf, *cp;
	Symbol* sym;
	buf = emalloc(strlen(name)+1);
	strcpy(buf, name);
	for (cp = buf; *cp; ++cp) {
		if (*cp == '.') {
			*cp = '\0';
			++cp;
			break;
		}
	}
	sym = hoc_table_lookup(buf, hoc_built_in_symlist);
	if (!sym) {
		sym = hoc_table_lookup(buf, hoc_top_level_symlist);
	}
	if (sym && *cp == '\0') {
		free(buf);
		return sym;
	}else if (sym && sym->type == TEMPLATE && *cp != '\0') {
		sym = hoc_table_lookup(cp, sym->u.template->symtable);
		if (sym) {
			free(buf);
			return sym;
		}
	}
	free(buf);
	return (Symbol*)0;
}

hoc_Symbol_units() {
	Symbol* sym, *hoc_get_last_pointer_symbol();
	double* hoc_pgetarg();
	int hoc_is_str_arg();
	char** hoc_temp_charptr();
	char** units = hoc_temp_charptr();

	if (hoc_is_double_arg(1)) {
		units_on_flag_ = (int)chkarg(1, 0., 1.);
		if (units_on_flag_) {
			*units = "on";
		}else{
			*units = "off";
		}
	}else{			
		if (hoc_is_str_arg(1)) {
			char* name = gargstr(1);
			sym = hoc_name2sym(name);
		}else{
			hoc_pgetarg(1);
			sym = hoc_get_last_pointer_symbol();
			assert(sym);
		}
		*units = (char*)0;
		if (ifarg(2)) {
			*units = gargstr(2);
		}
		*units = hoc_symbol_units(sym, *units);
		if (*units == (char*)0) {
			*units = "";
		}
	}
	hoc_ret();
	hoc_pushstr(units);
}

char* neuronhome_forward() {
	extern char* neuron_home;
#if defined(WIN32)
	static char* buf;
	extern char* hoc_back2forward();
	extern void hoc_forward2back();
	if (!buf) {
		buf = emalloc(strlen(neuron_home)+1);
		strcpy(buf, neuron_home);
	}
	hoc_back2forward(buf);
	return buf;
#else
	return neuron_home;
#endif
}

char *neuron_home_dos;
hoc_neuronhome() {
	extern char* neuron_home;
#if defined(WIN32)||defined(CYGWIN)
	if (ifarg(1) && (int)chkarg(1, 0., 1.) == 1) {
		hoc_ret();
		hoc_pushstr(&neuron_home_dos);
	}else{
		hoc_ret();
		hoc_pushstr(&neuron_home);
	}
#else
	hoc_ret();
	hoc_pushstr(&neuron_home);
#endif
}

char *
gargstr(narg)	/* Return pointer to string which is the narg argument */
	int narg;
{
	return *hoc_pgargstr(narg);
}

hoc_Strcmp()
{
	char *s1, *s2;
	s1=gargstr(1);
	s2=gargstr(2);
	ret();
	pushx((double)strcmp(s1 ,s2));
}

int hoc_sscanf() {
	int n;
	n = hoc_vsscanf(gargstr(1));
	ret();
	pushx((double)n);
}

int hoc_vsscanf(buf) char* buf;{
	/* assumes arg2 format string from hoc as well as remaining args */
	char *pf, *format, errbuf[100], **hoc_pgargstr();
	void* arglist[20];
	int n, iarg, i, islong, convert, sawnum;
   double* hoc_pgetarg();
	struct {
		union {
				double d;
				float f;
				long l;
				int i;
				char* s;
				char c;
		}u;
		int type;
	} arg[20];
	for (i=0; i < 20; ++i) {
		arglist[i] = (void*)0;
	}
	format = gargstr(2);
	iarg = 0;
	errbuf[0] = '\0';
	for (pf = format; *pf; ++pf) {
		if (*pf == '%') {
			convert = 1;
			islong = 0;
			sawnum = 0;
			++pf;
			if (! *pf) goto incomplete;
			if (*pf == '*') {
				convert = 0;
				++pf;
				if (! *pf) goto incomplete;
			}
			if (convert && iarg >= 19) {
				goto too_many;
			}
			while(isdigit(*pf)) {
				sawnum = 1;
				++pf;
				if (! *pf) goto incomplete;
			}
			if (*pf == 'l') {
				islong = 1;
				++pf;
				if (! *pf) goto incomplete;
			}
			if (convert) switch(*pf) {
			case '%':
				convert = 0;
				break;
			case 'd':
			case 'i':
			case 'o':
			case 'u':
			case 'x':
				if (islong) {
					arg[iarg].type = 'l';
					arglist[iarg] = (void*)&arg[iarg].u.l;
				}else{
					arg[iarg].type = 'i';
					arglist[iarg] = (void*)&arg[iarg].u.i;
				}
				break;
			case 'e':
			case 'f':
			case 'g':
				if (islong) {
					arg[iarg].type = 'd';
					arglist[iarg] = (void*)&arg[iarg].u.d;
				}else{
					arg[iarg].type = 'f';
					arglist[iarg] = (void*)&arg[iarg].u.f;
				}
				break;
			case '[':
				if (islong) {
					goto bad_specifier;
				}
				i=0; /* watch out for []...] and [^]...] */
				for (;;) {
					if (! *pf) goto incomplete;
					if (*pf == ']') {
						if (!( (i == 1) || ((i == 2) && (pf[-1] == '^')))) {
							break;
						}
					}
					++i;
					++pf;
				}
			case 's':
				if (islong) {
					goto bad_specifier;
				}
				arg[iarg].type = 's';
				arg[iarg].u.s = (char*)emalloc(strlen(buf) + 1);
				arglist[iarg] = (void*)arg[iarg].u.s;
				break;
			case 'c':
				if (islong || sawnum) {
					goto bad_specifier;
				}
				arg[iarg].type = 'c';
				arglist[iarg] = (void*)&arg[iarg].u.c;
				break;
			default:
         	goto bad_specifier;
				/*break;*/
			}
			if (convert) {
				++iarg;
				if (!ifarg(iarg+2)) {
					goto missing_arg;
				}
				switch (arg[iarg-1].type) {
				case 's':
					if (!hoc_is_str_arg(iarg+2)) {
						sprintf(errbuf, "arg %d must be a string", iarg+2);
						goto bad_arg;
					}
					break;
				default:
					if (!hoc_is_pdouble_arg(iarg+2)) {
               	sprintf(errbuf, "arg %d must be a pointer to a number", iarg+2);
						goto bad_arg;
					}
					break;
				}
			}
		}
	}

#if 0
	n = vsscanf(buf, format, arglist);
#else
	if (iarg < 4) {
		n = sscanf(buf, format, arglist[0], arglist[1], arglist[2]);
	}else if (iarg < 13) {
		n = sscanf(buf, format, arglist[0], arglist[1], arglist[2],
			arglist[3], arglist[4], arglist[5], arglist[6], arglist[7],
			arglist[8], arglist[9], arglist[10], arglist[11]);
	}else{
		goto too_many;
	}
#endif
	assert(n <= iarg);

	for (i = 0; i < n; ++i) {
		switch (arg[i].type) {
		case 'd':
  			*hoc_pgetarg(i+3) = arg[i].u.d;
			break;
		case 'i':
			*hoc_pgetarg(i+3) = (double)arg[i].u.i;
			break;
		case 'l':
			*hoc_pgetarg(i+3) = (double)arg[i].u.l;
			break;
		case 'f':
			*hoc_pgetarg(i+3) = (double)arg[i].u.f;
			break;
		case 's':
			hoc_assign_str(hoc_pgargstr(i+3), arg[i].u.s);
			break;
		case 'c':
			*hoc_pgetarg(i+3) = (double)arg[i].u.c;
			break;
		}
	}
	goto normal;
incomplete:
	sprintf(errbuf, "incomplete format specifier for arg %d", iarg + 3);
	goto normal;
bad_specifier:
	sprintf(errbuf, "unknown conversion specifier for arg %d", iarg + 3);
	goto normal;
missing_arg:
	sprintf(errbuf, "missing arg %d", iarg + 2);
	goto normal;
bad_arg:
	goto normal;
too_many:
	sprintf(errbuf, "too many ( > %d) args", iarg + 2);
	goto normal;
normal:
	for (i=0; i < iarg; ++i) {
		if (arg[i].type == 's') {
			free(arg[i].u.s);
		}
	}
	if (errbuf[0]) {
		hoc_execerror("scan error:", errbuf);
	}
	return n;
}

System()
{
	extern int hoc_plttext;
#if defined(WIN32) && !defined(CYGWIN)
	static char stdoutfile[]="\\systmp.tmp";
#else
	static char stdoutfile[]="/systmp.tmp";
#endif
	double d;
	int system();
	FILE *fp;

	if (hoc_plttext && !strchr(gargstr(1), '>')) {
		int n;
		HocStr* st;
		n = strlen(gargstr(1)) + strlen(stdoutfile);
		st = hocstr_create(n + 256);
		Sprintf(st->buf, "%s > %s", gargstr(1), stdoutfile);
		d = (double) system(st->buf);
#if 1
		if ((fp = fopen(stdoutfile, "r")) == (FILE *)0) {
hoc_execerror("Internal error in System(): can't open", stdoutfile);
		}
		while (fgets(st->buf, 255, fp) == st->buf) {
			plprint(st->buf);
		}
#endif
		hocstr_delete(st);
		IGNORE(unlink(stdoutfile));
	} else if (ifarg(2)) {
		extern HocStr* hoc_tmpbuf;
		HocStr* line;
		int i;
		fp = popen(gargstr(1), "r");
		if (!fp) {
			hoc_execerror("could not popen the command:", gargstr(1));
		}
		line = hocstr_create(1000);
		i = 0;
		hoc_tmpbuf->buf[0] = '\0';
		while (fgets_unlimited(line, fp)) {
			i += strlen(line->buf);
			if (hoc_tmpbuf->size <= i) {
				hocstr_resize(hoc_tmpbuf, 2*hoc_tmpbuf->size);
			}
			strcat(hoc_tmpbuf->buf, line->buf);
		}
		hocstr_delete(line);
		d = (double)pclose(fp);
		hoc_assign_str(hoc_pgargstr(2), hoc_tmpbuf->buf);
	} else {
		d = (double) system(gargstr(1));
	}
	errno = 0;
	ret();
	pushx(d);
}

int
Xred()	/* read with prompt string and default and limits */
{
	double d;
	double xred();

	d = xred(gargstr(1), *getarg(2), *getarg(3), *getarg(4));
	ret();
	pushx(d);
}

static struct { /* symbol types */
	char	*name;
	short	t_type;
} type_sym[] = {
	"Builtins",	BLTIN,
	"Other Builtins",	FUN_BLTIN,
	"Functions",	FUNCTION,
	"Procedures",	PROCEDURE,
	"Undefined",	UNDEF,
	"Scalars",	VAR,
	0,		0
};

extern Symlist *symlist;
extern Symlist	*p_symlist;
extern int	zzdebug;

static int
symdebug(s, list)	/* for debugging display the symbol lists */
	char *s;
	Symlist *list;
{
	Symbol *sp;

	Printf("\n\nSymbol list %s\n\n", s);
	if (list) for (sp = list->first; sp != (Symbol *)0; sp = sp->next)
	{
		Printf("name:%s\ntype:", sp->name);
		switch (sp->type)
		{
		case VAR:
if(!ISARRAY(sp)) {
			if (sp->subtype == USERINT)
				Printf("VAR USERINT  %8d", *(sp->u.pvalint));
			else if (sp->subtype == USERDOUBLE)
				Printf("VAR USERDOUBLE  %.8g", *(OPVAL(sp)));
			else
				Printf("VAR   %.8g", *(OPVAL(sp)));
}else{
			if (sp->subtype == USERINT)
				Printf("ARRAY USERINT");
			else if (sp->subtype == USERDOUBLE)
				Printf("ARRAY USERDOUBLE");
			else Printf("ARRAY");
}
			break;
		case NUMBER: Printf("NUMBER   %.8g", *(OPVAL(sp)));break;
		case STRING: Printf("STRING   %s", *(OPSTR(sp)));break;
		case UNDEF: Printf("UNDEF");break;
		case BLTIN: Printf("BLTIN");break;
		case AUTO: Printf("AUTO");break;
		case FUNCTION: Printf("FUNCTION");
			symdebug(sp->name, sp->u.u_proc->list);
			break;
		case PROCEDURE: Printf("PROCEDURE");
			symdebug(sp->name, sp->u.u_proc->list);
			break;
		case FUN_BLTIN: Printf("FUN_BLTIN");break;
		default: Printf("%d", sp->type);break;
		}
		Printf("\n");
	}
}

symbols()	/* display the types above */
{
	int i, j;
	Symbol *sp;

	if (zzdebug == 0)
	for (i = 0; type_sym[i].t_type != 0; i++)
	{
		Printf("\n%s\n", type_sym[i].name);
		for (sp = symlist->first; sp != (Symbol *)0; sp = sp->next)
			if (sp->type == type_sym[i].t_type)
			{
				Printf("\t%s", sp->name);
				switch (sp->type)
				{

				case VAR:
if(ISARRAY(sp)){
					for (j=0; j < sp->arayinfo->nsub; j++)
						Printf("[%d]",
						  sp->arayinfo->sub[j]);
}
					break;

				default: break;

				}
			}
		Printf("\n");
	}
	else{
		symdebug("p_symlist", p_symlist);
		symdebug("symlist", symlist);
	}	
	ret();
	pushx(0.);
}

double
chkarg(arg, low, high) /* argument checking for user functions */
	int arg;
	double low, high;
{
	double *getarg(), val;

	val = *getarg(arg);
	if (val > high || val < low) {
		hoc_execerror("Arg out of range in user function", (char *)0);
	}
	return val;
}



extern Inst *hoc_pc;
extern int hoc_nopop(), hoc_print();
extern int hoc_xopen_run();
extern double hoc_xpop();

extern double hoc_ac_;

double hoc_run_expr(sym) /* value of expression in sym made by hoc_parse_expr*/
	Symbol* sym;
{
	Inst *pcsav = hoc_pc;	
	hoc_execute(sym->u.u_proc->defn.in);
	hoc_pc = pcsav;
	return hoc_ac_;
}

Symbol* hoc_parse_expr(str, psymlist)
	char* str;
	Symlist** psymlist;
{
	Symbol* sp;
	char s[BUFSIZ];
	extern Symlist* hoc_top_level_symlist;
	
	if (!psymlist) {
		psymlist = &hoc_top_level_symlist;
	}
	sp = hoc_install(str,PROCEDURE,0.,psymlist);
	sp->u.u_proc->defn.in = STOP;
	sp->u.u_proc->list = (Symlist *)0;
	sp->u.u_proc->nauto = 0;
	sp->u.u_proc->nobjauto = 0;
	if (strlen(str) > BUFSIZ - 20) {
		HocStr* s;
		s = hocstr_create(strlen(str) + 20);
		sprintf(s->buf, "hoc_ac_ = %s\n", str);
		hoc_xopen_run(sp, s->buf);
		hocstr_delete(s);
	}else{
		sprintf(s, "hoc_ac_ = %s\n", str);
		hoc_xopen_run(sp, s);
	}
	return sp;
}

hoc_run_stmt(sym)
	Symbol* sym;
{
	Inst *pcsav = hoc_pc;	
	hoc_execute(sym->u.u_proc->defn.in);
	hoc_pc = pcsav;
}

Symbol* hoc_parse_stmt(str, psymlist)
	char* str;
	Symlist** psymlist;
{
	Symbol* sp;
	char s[BUFSIZ];
	extern Symlist* hoc_top_level_symlist;
	
	if (!psymlist) {
		psymlist = &hoc_top_level_symlist;
	}
	sp = hoc_install(str,PROCEDURE,0.,psymlist);
	sp->u.u_proc->defn.in = STOP;
	sp->u.u_proc->list = (Symlist *)0;
	sp->u.u_proc->nauto = 0;
	sp->u.u_proc->nobjauto = 0;
	if (strlen(str) > BUFSIZ - 10) {
		HocStr* s;
		s = hocstr_create(strlen(str) + 10);
		sprintf(s->buf, "{%s}\n", str);
		hoc_xopen_run(sp, s->buf);
		hocstr_delete(s);
	}else{
		sprintf(s, "{%s}\n", str);
		hoc_xopen_run(sp, s);
	}
	return sp;
}


extern double* hoc_varpointer;

hoc_pointer() {
	double* hoc_pgetarg();
	hoc_varpointer = hoc_pgetarg(1);
	ret();
	pushx(1.);
}

double* hoc_val_pointer(s) char* s;
{
	char buf[BUFSIZ];
	hoc_varpointer = 0;
	if (strlen(s) > BUFSIZ - 20) {
		HocStr* buf;
		buf = hocstr_create(strlen(s) + 20);
		sprintf(buf->buf, "{hoc_pointer_(&%s)}\n", s);
		hoc_oc(buf->buf);
		hocstr_delete(buf);
	}else{
		sprintf(buf, "{hoc_pointer_(&%s)}\n", s);
		hoc_oc(buf);
	}
	return hoc_varpointer;
}

hoc_name_declared() {
	Symbol* s;
	extern Symlist* hoc_top_level_symlist;
	Symlist* slsav;
	int x;

	if (ifarg(2) && *getarg(2) == 1.) {
		s = hoc_lookup(gargstr(1));
	}else{
		slsav = hoc_symlist;
		hoc_symlist = hoc_top_level_symlist;
		s = hoc_lookup(gargstr(1));
		hoc_symlist = slsav;
	}
	x = s ? 1. : 0.;
	if (s) {
		x = (s->type == OBJECTVAR) ? 2 : x;
		x = (s->type == SECTION) ? 3 : x;
		x = (s->type == STRING) ? 4 : x;
		x = (s->type == VAR) ? 5 : x;
	}
	ret();
	pushx((double)x);
}
