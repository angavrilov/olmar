/* lexer1.lex
   flex scanner for Lexer 1 of C++ compiler
   see lexer1.txt for specification */

/******************/
/* C declarations */
/******************/

  #include "lexer1.h"        /* Lexer 1 stuff */


/****************/
/* flex options */
/****************/

%option noyywrap
%option nounput
%option outfile="lexer1yy.cc"
%option prefix="lexer1_"


/********************/
/* C++ declarations */
/********************/

  #include "growbuf.h"       // GrowBuffer
  GrowBuffer collector;      // place to assemble big tokens

  // used for 2nd and 3rd arguments to lexer1Emit
  #define COLLECTOR (char*)collector.getDataC(), collector.getDataLen()
  
  // declare the external interface to the lexer
  #define YY_DECL int lexer1_lex(Lexer1 &lexer, FILE *inputFile)

  // what to do when lexer starts lexing a new file
  #define YY_USER_INIT lexer1_in = inputFile;


/***************/
/* sub-regexps */
/***************/

/* newline */
NL            "\n"

/* anything but newline */
NOTNL         .

/* any of 256 source characters */
ANY           ({NOTNL}|{NL})

/* backslash */
BACKSL        "\\"

/* beginnging of line (must be start of a pattern) */
BOL           ^

/* end of line (would like EOF to qualify also, but flex doesn't allow it */
EOL           {NL}

/* letter or underscore */
LETTER        [A-Za-z_]

/* letter or underscore or digit */
ALNUM         [A-Za-z_0-9]

/* decimal digit */
DIGIT         [0-9]

/* sequence of decimal digits */
DIGITS        ({DIGIT}+)

/* sign of a number */
SIGN          ("+"|"-")

/* normal string character: any but quote, newline, or backslash */
STRCHAR       [^"\n\\]

/* (start of) an escape sequence */
ESCAPE        ({BACKSL}{ANY})

/* double quote */
QUOTE         ["]

/* normal character literal character: any but single-quote, newline, or backslash */
CCCHAR        [^'\n\\]

/* single quote */
TICK          [']

/* space or tab */
SPTAB         [ \t]

/* preprocessor "character" -- any but escaped newline */
PPCHAR        ([^\\\n]|{BACKSL}{NOTNL})


/********************/
/* start conditions */
/********************/

%x ST_C_COMMENT
%x ST_STRING


/**************************/
/* token definition rules */
/**************************/
%%

  /* identifier: e.g. foo */
{LETTER}{ALNUM}* {
  lexer.emit(L1_IDENTIFIER, yytext, yyleng);
}

  /* integer literal; dec, oct, or hex */
[1-9][0-9]*           |
[0][0-7]*             |
[0][xX][0-9A-Fa-f]+   {
  lexer.emit(L1_INT_LITERAL, yytext, yyleng);
}

  /* floating literal */
{DIGITS}"."{DIGITS}?([eE]{SIGN}?{DIGITS})?  |
{DIGITS}"."?([eE]{SIGN}?{DIGITS})?	    |
"."{DIGITS}([eE]{SIGN}?{DIGITS})?	    {
  lexer.emit(L1_FLOAT_LITERAL, yytext, yyleng);
}

  /* ----- string literal ------- */
  /* intial */
{QUOTE}   {
  collector.setFromBlock(yytext, yyleng);
  BEGIN(ST_STRING);
}

  /* continuation */
<ST_STRING>({STRCHAR}|{ESCAPE})*   {
  collector.append(yytext, yyleng);
}

  /* final */
<ST_STRING>{QUOTE} {
  collector.append(yytext, yyleng);
  lexer.emit(L1_STRING_LITERAL, COLLECTOR);
  BEGIN(INITIAL);
}

  /* final, error */
<ST_STRING>{EOL}   |
<ST_STRING><<EOF>> {
  lexer.error("unterminated string literal");

  if (yytext[0] == '\n') {
    collector.append(yytext, yyleng);
  }
  else {
    // when matching <<EOF>>, yytext[0]=0 and yyleng=1 (possibly
    // a bug in flex; its man page doesn't specify what it does), so we
    // get an extra NUL in the collected token, which I don't want
  }
  lexer.emit(L1_STRING_LITERAL, COLLECTOR);

  BEGIN(INITIAL);
  if (yytext[0] != '\n') {
    yyterminate();     	  // flex man page says to do this for <<EOF>>
  }
}


  /* character literal */
{TICK}({CCCHAR}|{ESCAPE})*{TICK}   {
  lexer.emit(L1_CHAR_LITERAL, yytext, yyleng);
}


  /* operator */
"("|")"|"["|"]"|"->"|"::"|"."|"!"|"~"|"+"|"-"|"++"|"--"|"&"|"*"  |
".*"|"->*"|"/"|"%"|"<<"|">>"|"<"|"<="|">"|">="     	         |
"=="|"!="|"^"|"|"|"&&"|"||"|"?"|":"|"="|"*="|"/="|"%="|"+="      |
"-="|"&="|"^="|"|="|"<<="|">>="|","|"..."|";"|"{"|"}"            {
  lexer.emit(L1_OPERATOR, yytext, yyleng);
}

  /* preprocessor */
  /* technically, if this isn't at the start of a line (possibly after
   * some whitespace, it should be an error.. I'm not sure right now how
   * I want to deal with that (I originally was using '^', but that
   * interacts badly with the whitespace rule) */
"#"{PPCHAR}*({BACKSL}{NL}{PPCHAR}*)*   {
  lexer.emit(L1_PREPROCESSOR, yytext, yyleng);
}

  /* whitespace */
[ \t\n\f\v]+  {
  lexer.emit(L1_WHITESPACE, yytext, yyleng);
}

  /* C++ comment */
  /* we don't match the \n because that way this works at EOF */
"//"{NOTNL}*    {
  lexer.emit(L1_COMMENT, yytext, yyleng);
}

  /* ------- C comment --------- */
  /* initial */
"/*"     {
  collector.setFromBlock(yytext, yyleng);
  BEGIN(ST_C_COMMENT);
}

  /* continuation */
<ST_C_COMMENT>([^*]|"*"[^/])*   {
  collector.append(yytext, yyleng);
}

  /* final */
<ST_C_COMMENT>"*/"     {
  collector.append(yytext, yyleng);
  lexer.emit(L1_COMMENT, COLLECTOR);
  BEGIN(INITIAL);
}

  /* final, error */
<ST_C_COMMENT><<EOF>>     {
  lexer.error("unterminated /**/ comment");
  lexer.emit(L1_COMMENT, COLLECTOR);
  BEGIN(INITIAL);
}

  /* illegal */
.  {
  lexer.emit(L1_ILLEGAL, yytext, yyleng);
}


%%
/**************/
/* extra code */
/**************/




/*********/
/* trash */
/*********/
#if 0

  /* Notes
   *
   * 1) Contrary to usual lexer practice, I want this lexer to match potentially huge
   *    tokens, like entire blocks of C comments.  This is because I want those
   *    to become single L1 tokens, and it's easier to let flex create them
   *    (inefficiently) than to go write code to do so efficiently.  I'd reconsider
   *    if I decided some tool (like an editor) would rather break comments up into
   *    smaller pieces.
   *
   * 2) To avoid backtracking over these large tokens, they all have rules to cover
   *    all the possible endings, including <<EOF>>.
   */

{QUOTE}({STRCHAR}|{ESCAPE})*{QUOTE}   {
  lexer.emit(L1_STRING_LITERAL);
}

  /* unterminated string literal */
{QUOTE}({STRCHAR}|{ESCAPE})*{EOL}     |
{QUOTE}({STRCHAR}|{ESCAPE})*<<EOF>>   {
  lexer.emit(L1_STRING_LITERAL);
}


  /* unterminated character literal */
{TICK}({CCCHAR}|{ESCAPE})*{EOL}     |
{TICK}({CCCHAR}|{ESCAPE})*<<EOF>>   {
  lexer.error("unterminated character literal");
  lexer.emit(L1_CHAR_LITERAL);
}

%x ST_PREPROC

  /* ------- preprocessor ---------- */
  /* initial */
{BOL}{SPTAB}*"#"    {
  collector.setFromBlock(yytext, yyleng);
  BEGIN(ST_PREPROC);
}

  /* continuation */
<ST_PREPROC>({BACKSL}{NL}|{PPCHAR})+     {
  collector.append(yytext, yyleng);
}

  /* final */
<ST_PREPROC>{EOL}   |
<ST_PREPROC><<EOF>> {
  collector.append(yytext, yyleng);
  lexer.emit(L1_PREPROCESSOR, COLLECTOR);
  BEGIN(INITIAL);
}


#endif // 0


