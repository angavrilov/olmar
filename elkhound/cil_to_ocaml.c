/* Wes Weimer (weimer@cs.berkeley.edu)
 *
 * This file contains code to translate Scott McPeak's C++ AST into a
 * "C Intermediate Language", CIL.
 *
 * "value" is the "C" type for an OCAML [boxed] object
 */

#define OCAML_FALSE		(Val_int(0))
#define OCAML_DUMMY_LOCATION	(ocaml_location(-1,-1,"unknown"))
#define OCAML_DUMMY_TYPE	(Val_int(0))

/* forward declarations */
value ocaml_location(int line, int col, const char *file);
value ocaml_varinfo(int id, const char * name, int global, value typeinfo, value location);
value cil_variable_to_ocaml(Variable *v);
value ocaml_fieldinfo(const char *s_name, const char *f_name, value type);
value cil_lval_to_ocaml(CilLval *lval);
value ocaml_const_int(int i);
value cil_exp_to_ocaml(CilExpr *exp);
value cil_fncall_to_ocaml(CilFnCall *fnc);
value cil_inst_to_ocaml(CilInst *inst);
value cil_compound_to_ocaml(CilCompound *comp);
value cil_stmt_to_ocaml(CilStmt *stmt);
value cil_fndefn_to_ocaml(CilFnDefn * f);
/* value cil_program_to_ocaml(CilProgram * prog); */

/* return a "location" structure */
value ocaml_location(int line, int col, const char *file)
{
    CAMLparam0();
    CAMLlocal4(result, l, c, f);

    result = alloc(3,0);
    l = Val_int(line);
    c = Val_int(col);
    f = copy_string(file);
    Store_field(result, 0, l);
    Store_field(result, 1, c);
    Store_field(result, 2, f);

    CAMLreturn(result);
}

/* return a "varinfo" structure */
value ocaml_varinfo(int id, const char * name, int global,
	value typeinfo, value location)
{
    CAMLparam2(typeinfo, location);
    CAMLlocal4(result, i, n, g);

    result = alloc(5,0);
    i = Val_int(id);
    n = copy_string(name);
    g = OCAML_FALSE;		/* FIXME */
    Store_field(result, 0, i);
    Store_field(result, 1, n);
    Store_field(result, 2, g);
    Store_field(result, 3, typeinfo);
    Store_field(result, 4, location);

    CAMLreturn(result);
}

/* convert a CIL variable to an OCAML varinfo */
value cil_variable_to_ocaml(Variable *v)
{
    CAMLparam0();
    CAMLlocal4(result, a, b, c);

    a = OCAML_DUMMY_TYPE;	/* FIXME */
    b = OCAML_DUMMY_LOCATION;	/* FIXME */
    result = ocaml_varinfo(0, 
	    v->name.pcharc(), 0, a, b);
    CAMLreturn(result);
}

/* return a "fieldinfo" structure */
value ocaml_fieldinfo(const char *s_name, const char *f_name, value type)
{
    CAMLparam1(type);
    CAMLlocal3(result, s, f);

    result = alloc(3,0);
    f = copy_string(f_name);
    s = copy_string(s_name);
    Store_field(result, 0, s);
    Store_field(result, 1, f);
    Store_field(result, 2, type);

    CAMLreturn(result);
}

/* convert a CIL lvalue to OCAML */
value cil_lval_to_ocaml(CilLval *lval)
{
    CAMLparam0();
    CAMLlocal4(result,a,b,l);

    l = OCAML_DUMMY_LOCATION;	/* FIXME: lval locations? */ 

    switch (lval->ltag) {
	case CilLval::T_VARREF:
	    result = alloc(2,0);
	    a = cil_variable_to_ocaml(lval->varref.var);
	    Store_field(result, 0, a);
	    Store_field(result, 1, l);
	    break;
	case CilLval::T_DEREF:
	    result = alloc(2,1);
	    a = cil_exp_to_ocaml(lval->deref.addr);
	    Store_field(result, 0, a);
	    Store_field(result, 1, l);
	    break;
	case CilLval::T_FIELDREF:
	    result = alloc(3,2);
	    a = cil_lval_to_ocaml(lval->fieldref.record);
	    b = ocaml_fieldinfo("struct_name",
		    lval->fieldref.field->name.pcharc(), 
		    OCAML_DUMMY_TYPE);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;
	case CilLval::T_CASTL:
	    result = alloc(3,3);
	    a = OCAML_DUMMY_TYPE;
	    b = cil_exp_to_ocaml(lval->castl.lval);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;
	case CilLval::T_ARRAYELT:
	    result = alloc(2,4);
	    a = cil_exp_to_ocaml(lval->arrayelt.array);
	    b = cil_exp_to_ocaml(lval->arrayelt.index);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;
	default: xassert(0);
    }
    CAMLreturn(result);
}

/* convert a CIL constant int to OCAML */
value ocaml_const_int(int i)
{
    CAMLparam0();
    CAMLlocal2(result,a);
    result = alloc(1,0);
    a = Val_int(i);
    Store_field(result, 0, a);
    CAMLreturn(result);
}

/* convert a CIL expression to OCAML */
value cil_exp_to_ocaml(CilExpr *exp)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,l);
    int code;

    l = OCAML_DUMMY_LOCATION;	/* FIXME: exp locations? */ 

    switch (exp->etag) {
	case CilExpr::T_LITERAL: 
	    result = alloc(2,0);
	    a = ocaml_const_int(exp->lit.value);
	    Store_field(result, 0, a);
	    Store_field(result, 1, l);
	    break;

	case CilExpr::T_LVAL: 
	    result = alloc(1,1);
	    a = cil_lval_to_ocaml(exp->lval);
	    Store_field(result, 0, a);
	    break;

	case CilExpr::T_UNOP: 
	    result = alloc(3,2);
	    switch (exp->unop.op) {
		case OP_NEGATE: code = 0; break;
		case OP_NOT: code = 1; break;
		case OP_BITNOT: code = 2; break;
		default: xassert(0);
	    }
	    a = Val_int(code);
	    b = cil_exp_to_ocaml(exp->unop.exp);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;

	case CilExpr::T_BINOP: 
	    result = alloc(4,3);
	    switch (exp->binop.op) {
		case OP_PLUS: code = 0; break;
		case OP_MINUS: code = 1; break;
		case OP_TIMES: code = 2; break;
		case OP_DIVIDE: code = 3; break;
		case OP_MOD: code = 4; break;
		case OP_LSHIFT: code = 5; break;
		case OP_RSHIFT: code = 6; break;
		case OP_LT: code = 7; break;
		case OP_GT: code = 8; break;
		case OP_LTE: code = 9; break;
		case OP_GTE: code = 10; break;
		case OP_EQUAL: code = 11; break;
		case OP_NOTEQUAL: code = 12; break;
		case OP_BITAND: code = 13; break;
		case OP_BITXOR: code = 14;  break;
		case OP_BITOR: code = 15; break;
		case OP_AND: code = 16;  break;
		case OP_OR: code = 17; break;
		default: xassert(0);
	    }
	    a = Val_int(code);
	    b = cil_exp_to_ocaml(exp->binop.left);
	    c = cil_exp_to_ocaml(exp->binop.right);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, c);
	    Store_field(result, 3, l);
	    break;

	case CilExpr::T_CASTE: 
	    result = alloc(3,4);
	    a = OCAML_DUMMY_TYPE;
	    b = cil_exp_to_ocaml(exp->caste.exp);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;


	case CilExpr::T_ADDROF: 
	    result = alloc(2,5);
	    a = cil_lval_to_ocaml(exp->addrof.lval);
	    Store_field(result, 0, a);
	    Store_field(result, 1, l);
	    break;

	default: xassert(0);
    }

    CAMLreturn(result);
}

/* convert a CIL "FnCall" into OCAML:
 * does not set the location field, that is done by cil_inst_to_ocaml() */
value cil_fncall_to_ocaml(CilFnCall *fnc)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,d);
    int i;

    result = alloc(4,1); /* tag 1 means "call" */

    a = cil_lval_to_ocaml(fnc->result);
    b = cil_exp_to_ocaml(fnc->func);
    Store_field(result, 0, a);
    Store_field(result, 1, b);

    /* now gather the argument list */
    c = Val_int(0);	/* empty list */
    for (i=fnc->args.count(); i>0 ; i--) {
	CilExpr * arg = fnc->args.nth(i-1);

	d = alloc(2,0); /* cons cell */
	a = cil_exp_to_ocaml(arg);
	Store_field(d, 0, a);
	Store_field(d, 1, c); /* store old tail */
	c = d;
    }
    Store_field(result, 2, c);
}


/* convert a CIL instruction into OCAML */
value cil_inst_to_ocaml(CilInst *inst)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,l);

    l = OCAML_DUMMY_LOCATION;	/* FIXME: exp locations? */ 

    switch (inst->itag) {
	case CilInst::T_ASSIGN:
	    result = alloc(3,0);
	    a = cil_lval_to_ocaml(inst->assign.lval);
	    b = cil_exp_to_ocaml(inst->assign.expr);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, l);
	    break;

	case CilInst::T_CALL:
	    result = cil_fncall_to_ocaml(inst->call);
	    Store_field(result, 3, l);

	    break;
	default: xassert(0);
    }

    CAMLreturn(result);
}

/* convert a CIL compount statement into an OCAML stmt list */
value cil_compound_to_ocaml(CilCompound *comp)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,d);
    int i;

    result = Val_int(0);	/* the empty list */

    for (i=comp->stmts.count(); i>0; i--) {
	/* create a new cons cell */
	b = alloc(2,0);
	c = cil_stmt_to_ocaml(comp->stmts.nth(i-1));
	Store_field(b, 0, c);
	Store_field(b, 1, result);

	result = b;
    }

    CAMLreturn(result);
}

/* convert a CIL stament (similar to an AST node) into OCAML */
value cil_stmt_to_ocaml(CilStmt *stmt)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,d);

    switch (stmt->stag) {
	case CilStmt::T_COMPOUND:
	    result = alloc(1,0);
	    a = cil_compound_to_ocaml(stmt->comp);
	    Store_field(result, 0, a);
	    break;

	case CilStmt::T_LOOP:
	    result = alloc(2,1);
	    a = cil_exp_to_ocaml(stmt->loop.cond);
	    b = cil_stmt_to_ocaml(stmt->loop.body);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    break;

	case CilStmt::T_IFTHENELSE:
	    result = alloc(3,2);
	    a = cil_exp_to_ocaml(stmt->ifthenelse.cond);
	    b = cil_stmt_to_ocaml(stmt->ifthenelse.thenBr);
	    c = cil_stmt_to_ocaml(stmt->ifthenelse.elseBr);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    Store_field(result, 2, c);
	    break;

	case CilStmt::T_LABEL:
	    result = alloc(1,3);
	    a = copy_string(stmt->label.name->pcharc());
	    Store_field(result, 0, a);
	    break;

	case CilStmt::T_JUMP:
	    result = alloc(1,4);
	    a = copy_string(stmt->jump.dest->pcharc());
	    Store_field(result, 0, a);
	    break;

	case CilStmt::T_RET:
	    result = alloc(1,5);
	    a = cil_exp_to_ocaml(stmt->ret.expr);
	    Store_field(result, 0, a);
	    break;

	case CilStmt::T_SWITCH:
	    result = alloc(2,6);
	    a = cil_exp_to_ocaml(stmt->switchStmt.expr);
	    b = cil_stmt_to_ocaml(stmt->switchStmt.body);
	    Store_field(result, 0, a);
	    Store_field(result, 1, b);
	    break;

	case CilStmt::T_CASE:
	    result = alloc(1,7);
	    a = Val_int(stmt->caseStmt.value);
	    Store_field(result, 0, a);
	    break;

	case CilStmt::T_DEFAULT:
	    result = Val_int(0);
	    break;

	case CilStmt::T_INST:
	    result = alloc(1,8);
	    a = cil_inst_to_ocaml(stmt->inst.inst);
	    Store_field(result, 0, a);
	    break;

	default: xassert(0);
    }

    CAMLreturn(result);
}

/* convert a CIL function declaration to OCAML */
value cil_fndefn_to_ocaml(CilFnDefn * f)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,d);
    int i;

    result = alloc(5,0);

    a = copy_string(f->var->name.pcharc());
    Store_field(result, 0, a);
    b = OCAML_DUMMY_TYPE;	/* FIXME */
    Store_field(result, 1, b);
    c = alloc(1,0);
    d = cil_compound_to_ocaml(& (f->bodyStmt));
    Store_field(c, 0, d);
    Store_field(result, 2, c);

    /* now handle the arg list */
    d = Val_int(0);
    /* FIXME: arg lists? */
    Store_field(result, 3, d);

    /* now handle the decl list */
    d = Val_int(0);
    for (i=f->locals.count(); i>0 ; i--) {
	/* make a new cons cell */
	a = alloc(2,0);
	b = cil_variable_to_ocaml(f->locals.nth(i-1));
	Store_field(a, 0, b);
	Store_field(a, 1, d);
	d = a;
    }
    Store_field(result, 4, d);

    CAMLreturn(result);
}

/* convert a CIL program to OCAML */
value cil_program_to_ocaml(CilProgram * prog)
{
    CAMLparam0();
    CAMLlocal5(result,a,b,c,d);
    int i;

    result = alloc(2,0);

    /* first do the fun decl list */
    a = Val_int(0);
    for (i=prog->funcs.count(); i>0 ; i--) {
	/* make a cons cell */
	b = alloc(2,0);
	c = cil_fndefn_to_ocaml(prog->funcs.nth(i-1));
	Store_field(b, 0, c);
	Store_field(b, 1, a);
	a = b;
    }

    Store_field(result, 0, a);

    /* now do the global variable decls */
    a = Val_int(0);
    for (i=prog->globals.count(); i>0 ;i--) {
	/* make a cons cell */
	b = alloc(2,0);
	c = cil_variable_to_ocaml( prog->globals.nth(i-1) );
	Store_field(b, 0, c);
	Store_field(b, 1, a);
	a = b;
    }

    Store_field(result, 1, a);

    CAMLreturn(result);
}
