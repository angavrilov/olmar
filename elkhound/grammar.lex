/* grammar.lex
 * lexical anayzer for grammar files
 *  basically, this is C lexical structure
 */

/* ----------------- C definitions -------------------- */
%{

struct BackslashCode {
  char code;         // the char that follows a backslash
  char meaning;      // its meaning
};
static BackslashCode backslashCodes[] = {
  { 'a', '\a' },      // bell
  { 'b', '\b' },      // backspace
  { 'f', '\f' },      // formfeed
  { 'n', '\n' },      // newline
  { 'r', '\r' },      // carriage return
  { 't', '\t' },      // tab
  { 'v', '\v' },      // vertical tab
};


%}

/* ------------------- definitions -------------------- */
/* any character, including newline */
ANY       (.|"\n")


/* --------------- start conditions ------------------- */


/* ---------------------- rules ----------------------- */
%%

  /* -------- comments -------- */
"/*" {
  /* C-style comments */
  commentStartLine = yylineno;
  BEGIN(C_COMMENT);
}

<C_COMMENT>{
  "*/" {
    /* end of comment */
    BEGIN(INITIAL);
  }

  ANY {
    /* anything else -- eat it */
  }

  <<EOF>> {
    printf("unterminated comment, beginning on line %d\n", commentStartLine);
    BEGIN(INITIAL);
  }
}


"//"."\n" {
  /* C++-style comment -- eat it */
}


  /* -------- strings -------- */
"\"" {
  /* string literal */
  stringLiteral = "";
  BEGIN(STRING);
}

<STRING>{
  "\"" {
    /* end of string */
    /* caller can retrieve string text as stringLiteral */
    return TOK_STRING;
  }

  "\\\n"(WHITESP)* {
    /* escaped newline: eat it, and all the leading whitespace on the
     * next line */
  }

  "\\"(OCTAL)(OCTAL)(OCTAL) {
    stringLiteral << (char)strtol(string(yytext+1,3), NULL, 8 /*radix*/);
  }

  "\\"(HEX)(HEX) {
    stringLiteral << (char)strtol(string(yytext+1,2), NULL, 16 /*radix*/);
  }

  "\\". {
    /* one-char backslash code */
    int i;
    for (i=0; i<TABLESIZE(backslashCodes); i++) {
      if (backslashCodes[i].code == yytext[1]) {
        stringLiteral << backslashCodes[i].meaning;
        break;
      }
    }
    if (i == TABLESIZE(backslashCodes)) {
      /* code not found -- the character itself is what we want (e.g. "\"") */
      stringLiteral << yytext[i];
    }
  }

  "\n" {
    /* unescaped newline */
    printf("unterminated string literal on line %d\n", yylineno);
    BEGIN(INITIAL);
    return TOK_STRING;
  }

  . {
    /* ordinary text character */
    stringLiteral << yytext[0];
  }
}

  /* -------- identifiers, numbers, etc. -------- */
(LETTER)(LETTER|DIGIT)* {
  /* identifier (or keyword) */
  return TOK_IDENTIFIER;
}

(DIGIT)+ { 
  /* number literal */
  return TOK_NUMBER;
}

"+"|"-"|"*"|"/"|"%"|"!"|"~"|"^"|"&"|"("|")"|"{"|"}"|"["|"]"   |
"<"|">"|"<="|">="|";"|":"|","|"."|"?"|"="|"|"|"<<"|">>"|"!="  |
"+="|"-="|"*="|"/="|"%="|"|="|"&="                            {
  /* operator (or punctuator) */
  /* (I don't envision uses for all of them, but it's the basic
   * set of operators from which I will draw) */
  return TOK_OPERATOR;
}


