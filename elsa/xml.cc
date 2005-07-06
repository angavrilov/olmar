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

void LinkSatisfier::satisfyLinks() {
  // Nodes
  FOREACH_ASTLIST(UnsatLink, unsatLinks, iter) {
    UnsatLink const *ul = iter.data();
    void *obj = id2obj.queryif(ul->id);
    if (obj) {
      *(ul->ptr) = obj;         // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied node link: " << ul->id << endl;
    }
  }

  // FakeLists
  FOREACH_ASTLIST(UnsatLink, unsatLinks_FakeList, iter) {
    UnsatLink const *ul = iter.data();
    void *obj = id2obj.queryif(ul->id);
    if (obj) {
      *(ul->ptr) = obj;         // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied FakeList link: " << ul->id << endl;
    }
  }

  // ASTLists
  FOREACH_ASTLIST(UnsatLink, unsatLinks_ASTList, iter) {
    UnsatLink const *ul = iter.data();
    // NOTE: I rely on the fact that all ASTLists just contain
    // pointers; otherwise this cast would cause problems; Note that I
    // have to use char instead of void because you can't delete a
    // pointer to void; see the note in the if below.
    ASTList<char> *obj = reinterpret_cast<ASTList<char>*>(id2obj.queryif(ul->id));
    if (obj) {
      ASTList<char> *ptr = reinterpret_cast<ASTList<char>*>(ul->ptr);
      xassert(ptr->isEmpty());
      // this is particularly tricky because the steal contains a
      // delete, however there is nothing to delete, so we should be
      // ok.  If there were something to delete, we would be in
      // trouble because you can't call delete on a pointer to an
      // object the size of which is different from the size you think
      // it is due to a type you cast it too.
      ptr->steal(obj);          // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied ASTList link: " << ul->id << endl;
    }
  }
}


// -------------------- ReadXml -------------------

void ReadXml::reset() {
  lastNode = NULL;
  lastKind = 0;
  lastFakeListId = NULL;
  xassert(nodeStack.isEmpty());
  xassert(kindStack.isEmpty());
}

void ReadXml::userError(char const *msg) {
  THROW(xBase(stringc << inputFname << ":" << lexer.linenumber << ":" << msg));
}

bool ReadXml::parse() {
  reset();
  while(1) {
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
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));

    // This funnyness is due to the fact that when we see a FakeList
    // tag, we make an empty FakeList, so there is nothing to put into
    // in the id2obj map, so we don't put anything there.  Instead we
    // store the id in lastFakeListId and then when we get the first
    // child, we finally know the FakeList address so we file that
    // under the id.
    if (lastFakeListId) {
      if (linkSat.id2obj.isMapped(lastFakeListId)) {
        userError(stringc << "this id is taken " << lastFakeListId);
      }
//        cout << "(FakeList) id2obj.add(" << lastFakeListId
//             << ", " << static_cast<void const*>(nodeStack.top())
//             << ")\n";
      linkSat.id2obj.add(lastFakeListId, nodeStack.top());
      delete lastFakeListId;
      lastFakeListId = NULL;
    }

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

    // if the last kind was a fakelist, reverse it
    if (kind2kindCat(lastKind) == KC_FakeList) {
      reverseFakeList(lastNode, lastKind);
      // NOTE: it may seem rather bizarre, but we just throw away the
      // reversed list.  The first node in the list was the one that
      // we saw first and it was filed in the hashtable under the id
      // of the first element.  Therefore, whatever points to the
      // first element in the list will do so when the unsatisfied
      // links are filled in from the hashtable.
    }

    // if the node up the stack is a list, put this element onto that
    // list
    if (nodeStack.isEmpty()) {
      // If the stack is empty, return
      xassert(kindStack.isEmpty());
      return false;
    } else {
      int topKind = *kindStack.top();
      int topKindCat = kind2kindCat(topKind);
      if (topKindCat == KC_FakeList) {
        // there is no way to reset the top so I just pop and push again
//          cout << "prepend2FakeList nodeStack.top(): " << static_cast<void const*>(nodeStack.top());
        void *tmpNewTop = prepend2FakeList(nodeStack.pop(), topKind, lastNode, lastKind);
//          cout << ", tmpNewTop: " << static_cast<void const*>(tmpNewTop) << endl;
        nodeStack.push(tmpNewTop);
      } else if (topKindCat == KC_ASTList) {
        append2ASTList(nodeStack.top(), topKind, lastNode, lastKind);
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
      if (strcmp(id0.c_str(), "FL0") == 0
          // these should not be possible and the isMapped check below
          // would catch it if they occured:
          //  || strcmp(id0.c_str(), "0AL") == 0
          //  || strcmp(id0.c_str(), "0ND") == 0
          ) {
        // otherwise its null and there is nothing to record; UPDATE:
        // I no longer understand this code here
        if (nodeStack.top()) {
          userError("FakeList with FL0 id should be empty");
        }
      } else if (strncmp(id0.c_str(), "FL", 2) == 0) {
        xassert(!lastFakeListId);
        lastFakeListId = strdup(id0.c_str());
      } else {
        if (linkSat.id2obj.isMapped(id0)) {
          userError(stringc << "this id is taken " << id0);
        }
//          cout << "id2obj.add(" << id0
//               << ", " << static_cast<void const*>(nodeStack.top())
//               << ")\n";
        linkSat.id2obj.add(id0, nodeStack.top());
      }
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
