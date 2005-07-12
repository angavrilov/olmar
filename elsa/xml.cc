// cc_type_xml.h            see license.txt for copyright and terms of use

#include "xml.h"
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer
#include "exc.h"                // xBase


string toXml_bool(bool b) {
  if (b) return "true";
  else return "false";
}

void fromXml_bool(bool &b, string str) {
  b = (strcmp(parseQuotedString(str), "true") == 0);
}


// -------------------- LinkSatisfier -------------------

UnsatLink::UnsatLink(void **ptr0, string id0, int kind0)
  : ptr(ptr0), id(id0), kind(kind0)
{};

void LinkSatisfier::registerReader(ReadXml *reader) {
  xassert(reader);
  readers.append(reader);
}

void *LinkSatisfier::convertList2FakeList(ASTList<char> *list, int listKind) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    void *target;
    if (reader->convertList2FakeList(list, listKind, &target)) {
      return target;
    }
  }
  THROW(xBase(stringc << "no converter for FakeList type"));
}

bool LinkSatisfier::kind2kindCat(int kind, KindCategory *kindCat) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    if (reader->kind2kindCat(kind, kindCat)) {
      return true;
    }
  }
//    THROW(xBase(stringc << "no kind category registered for this kind"));
  return false;
}

void LinkSatisfier::satisfyLinks() {
  // AST
  FOREACH_ASTLIST(UnsatLink, unsatLinks, iter) {
    UnsatLink const *ul = iter.data();
    void *obj = id2obj.queryif(ul->id);
    if (obj) {
      *(ul->ptr) = obj;
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied node link: " << ul->id << endl;
    }
  }

  // Lists
  FOREACH_ASTLIST(UnsatLink, unsatLinks_List, iter) {
    UnsatLink const *ul = iter.data();
    // NOTE: I rely on the fact that all ASTLists just contain
    // pointers; otherwise this cast would cause problems; Note that I
    // have to use char instead of void because you can't delete a
    // pointer to void; see the note in the if below.
    ASTList<char> *obj = reinterpret_cast<ASTList<char>*>(id2obj.queryif(ul->id));
    if (!obj) {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
      //        cout << "unsatisfied List link: " << ul->id << endl;
      continue;
    }

    KindCategory kindCat;
    bool foundIt = kind2kindCat(ul->kind, &kindCat);
    if (!foundIt) {
      THROW(xBase(stringc << "no kind category registered for this kind"));
    }
    switch (kindCat) {
    default:
      xfailure("illegal list kind");
      break;

    case KC_ASTList: {
      // Recall that ASTLists are used in a class by embeding them.
      // Therefore, a pointer to the field to be filled in is a
      // pointer to an ASTList, not a pointer to a pointer to an
      // ASTList, as one might expect for most other classes that were
      // used in a host class by being pointed to.
      ASTList<char> *ptr = reinterpret_cast<ASTList<char>*>(ul->ptr);
      xassert(ptr->isEmpty());
      // this is particularly tricky because the steal contains a
      // delete, however there is nothing to delete, so we should be
      // ok.  If there were something to delete, we would be in
      // trouble because you can't call delete on a pointer to an
      // object the size of which is different from the size you think
      // it is due to a type you cast it too.
      ptr->steal(obj);
      // it seems that I should not subsequently delete the list as
      // the steal has deleted the voidlist of the ASTList and it
      // seems to be a bug to try to then delete the ASTList that has
      // been stolen from
      break;
    }

    case KC_FakeList: {
      // Convert the ASTList we used to store the FakeList into a real
      // FakeList and hook in all of the pointers.  This is
      // type-specific, so generated code must do it that can switch
      // on the templatized type of the FakeList.
      *(ul->ptr) = convertList2FakeList(obj, ul->kind);
      // Make the list dis-own all of its contents so it doesn't delete
      // them when we delete it.  Yes, I should have used a non-owning
      // constant-time-append list.
      obj->removeAll_dontDelete();
      // delete the ASTList
      delete obj;
      break;
    }
    }
  }
}


// -------------------- ReadXml -------------------

void ReadXml::reset() {
  lastNode = NULL;
  lastKind = 0;
  xassert(nodeStack.isEmpty());
  xassert(kindStack.isEmpty());
}

void ReadXml::userError(char const *msg) {
  THROW(xBase(stringc << inputFname << ":" << lexer.linenumber << ":" << msg));
}

bool ReadXml::parse() {
  reset();
  while(true) {
    // state: looking for a tag start
    int start = lexer.yylex();
//      printf("start:%s\n", lexer.tokenKindDesc(start).c_str());
    switch(start) {
    default: userError("unexpected token while looking for '<' of an open tag");
    case 0: return true;        // we are done
    case XTOK_LESSTHAN:
      break;                    // go to next state
    }

    // state: read a tag name
    int tag = lexer.yylex();
    void *topTemp;
    bool sawCloseTag = ctorNodeFromTag(tag, topTemp);
    if (sawCloseTag) goto close_tag;
    xassert(topTemp);
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));
    readAttributes();
    continue;

    // state: read a close tag name
  close_tag:
    int closeTag = lexer.yylex();
    if (!closeTag) {
      userError("unexpected file termination while looking for a close tag name");
    }
    if (nodeStack.isEmpty()) {
      userError("too many close tags");
    }
    if (*kindStack.top() != closeTag) {
      userError("close tag does not match open tag");
    }

    int closeGreaterThan = lexer.yylex();
    switch(closeGreaterThan) {
    default: userError("unexpected token while looking for '>' of a close tag");
    case 0: userError("unexpected file termination while looking for '>' of a close tag");
    case XTOK_GREATERTHAN:
      break;
    }

    lastNode = nodeStack.pop();
    // FIX: do I delete this int on the heap or does the stack do it?
    lastKind = *kindStack.pop();
    // if the node up the stack is a list, put this element onto that
    // list
    if (nodeStack.isEmpty()) {
      // If the stack is empty, return
      xassert(kindStack.isEmpty());
      return false;
    } else {
      xassert(nodeStack.isNotEmpty());
      xassert(kindStack.isNotEmpty());
      int topKind = *kindStack.top();
      KindCategory topKindCat;
      bool found = kind2kindCat(topKind, &topKindCat);
      // FIX: maybe this should be an assertion
      if (!found) {
        userError("no category found for this kind");
      }
      if (topKindCat == KC_FakeList ||
          topKindCat == KC_ASTList  ||
          topKindCat == KC_SObjList ||
          topKindCat == KC_ObjList  ){
        xassert(nodeStack.top());
        append2List(nodeStack.top(), topKind, lastNode, lastKind);
      }
    }
  }
  if (lastKind != XTOK_TranslationUnit) {
    userError("top tag is not a TranslationUnit");
  }
}

// state: read the attributes
void ReadXml::readAttributes() {
  while(1) {
    int attr = lexer.yylex();
    switch(attr) {
    default: break;             // go on; assume it is a legal attribute tag
    case 0: userError("unexpected file termination while looking for an attribute name");
    case XTOK_GREATERTHAN:
      return;
    }

    int eq = lexer.yylex();
    switch(eq) {
    default: userError("unexpected token while looking for an '='");
    case 0: userError("unexpected file termination while looking for an '='");
    case XTOK_EQUAL:
      break;                    // go on
    }

    int value = lexer.yylex();
    switch(value) {
    default: userError("unexpected token while looking for an attribute value");
    case 0: userError("unexpected file termination while looking for an attribute value");
    case XTOK_INT_LITERAL:
      // get it out of yytext below
      break;
    case XTOK_STRING_LITERAL:
      // get it out of yytext below
      break;                    // go on
    }

    // register the attribute
    xassert(nodeStack.isNotEmpty());
    // special case the .id attribute
    if (attr == XTOK_DOT_ID) {
      // FIX: I really hope the map makes a copy of this string
      string id0 = parseQuotedString(lexer.YYText());
      if (linkSat.id2obj.isMapped(id0)) {
        userError(stringc << "this id is taken " << id0);
      }
      linkSat.id2obj.add(id0, nodeStack.top());
    }

    // attribute other than '.id'
    else {
      registerAttribute(nodeStack.top(), *kindStack.top(), attr, lexer.YYText());
    }
  }
  if (!nodeStack.isEmpty()) {
    userError("missing closing tags at eof");
  }
  // stacks should not be out of sync
  xassert(kindStack.isEmpty());
}
