// ccgrmain.h            see license.txt for copyright and terms of use
// some prototypes I don't have a good place for, related
// to starting up the parser

#ifndef CCGRMAIN_H
#define CCGRMAIN_H

class CCLang;
class StringTable;
class UserActions;

// defined by the user somewhere
UserActions *makeUserActions(StringTable &table, CCLang &lang);

#endif // CCGRMAIN_H
