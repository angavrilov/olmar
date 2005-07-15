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

void *LinkSatisfier::convertList2SObjList(ASTList<char> *list, int listKind) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    void *target;
    if (reader->convertList2SObjList(list, listKind, &target)) {
      return target;
    }
  }
  THROW(xBase(stringc << "no converter for SObjList type"));
}

void *LinkSatisfier::convertList2ObjList(ASTList<char> *list, int listKind) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    void *target;
    if (reader->convertList2ObjList(list, listKind, &target)) {
      return target;
    }
  }
  THROW(xBase(stringc << "no converter for ObjList type"));
}

bool LinkSatisfier::kind2kindCat(int kind, KindCategory *kindCat) {
  xassert(kind != -1);          // this means you shouldn't be asking
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
    xassert(ul->kind == -1);
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
      // FIX: I suppose I don't need to do this steal: I can just cast
      // it to the result
#warning try just the cast here

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

    case KC_SObjList: {
      // Convert the ASTList we used to store the SObjList into a real
      // SObjList and hook in all of the pointers.  This is
      // type-specific, so generated code must do it that can switch
      // on the templatized type of the SObjList.
      *(ul->ptr) = convertList2SObjList(obj, ul->kind);
      // Make the list dis-own all of its contents so it doesn't delete
      // them when we delete it.  Yes, I should have used a non-owning
      // constant-time-append list.
      obj->removeAll_dontDelete();
      // delete the ASTList
      delete obj;
      break;
    }

    case KC_ObjList: {
      // Convert the ASTList we used to store the ObjList into a real
      // ObjList and hook in all of the pointers.  This is
      // type-specific, so generated code must do it that can switch
      // on the templatized type of the ObjList.
      *(ul->ptr) = convertList2ObjList(obj, ul->kind);
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

void ReadXml::parseOneTopLevelTag() {
  // FIX: a do-while is always a bug
  do parseOneTag();
  while(!atTopLevel());
  xassert(kindStack.isEmpty()); // the stacks are synchronized
}

void ReadXml::parseOneTag() {
  // state: looking for a tag start
  if (lexer.haveSeenEof()) {
    userError("unexpected EOF while looking for '<' of an open tag");
  }
  int start = lexer.getToken();
  //      printf("start:%s\n", lexer.tokenKindDesc(start).c_str());
  switch(start) {
  default:
    userError("unexpected token while looking for '<' of an open tag");
    break;
  case 0:                     // eof
    return;
    break;
  case XTOK_LESSTHAN:
    break;                    // continue parsing
  }

  // state: read a tag name
  int tag = lexer.getToken();
  void *topTemp;
  bool sawCloseTag = ctorNodeFromTag(tag, topTemp);
  if (!sawCloseTag) {
    xassert(topTemp);
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));
    readAttributes();
    return;
  }

  // state: read a close tag name
  int closeTag = lexer.getToken();
  if (!closeTag) {
    userError("unexpected file termination while looking for a close tag name");
  }
  if (nodeStack.isEmpty()) {
    userError("too many close tags");
  }
  if (*kindStack.top() != closeTag) {
    userError("close tag does not match open tag");
  }

  // state: read the '>' after a close tag
  int closeGreaterThan = lexer.getToken();
  switch(closeGreaterThan) {
  default: userError("unexpected token while looking for '>' of a close tag");
  case 0: userError("unexpected file termination while looking for '>' of a close tag");
  case XTOK_GREATERTHAN:
    break;
  }

  // state: figure out if we are done
  lastNode = nodeStack.pop();
  // FIX: do I delete this int on the heap or does the stack do it?
  lastKind = *kindStack.pop();
  if (nodeStack.isEmpty()) {
    // If the stack is empty, return
    xassert(kindStack.isEmpty());
    return;
  }

  // state: if the node up the stack is a list, put this element
  // onto that list
  xassert(nodeStack.isNotEmpty());
  xassert(kindStack.isNotEmpty());
  int topKind = *kindStack.top();

  // what kind of thing is on the top of the stack?
  KindCategory topKindCat;
  bool found = kind2kindCat(topKind, &topKindCat);
  // FIX: maybe this should be an assertion
  if (!found) {
    userError("no category found for this kind");
  }

  if (lastKind == XTOK_Name) {
    // we want to delete these name things after they have served
    // their purpose
    delete ((Name*)lastNode);
  }

  // if the top of the stack is a container, put the last tag into
  // that container; otherwise do nothing
  if (topKindCat == KC_FakeList ||
      topKindCat == KC_ASTList  ||
      topKindCat == KC_SObjList ||
      topKindCat == KC_ObjList  ){
    // the top is a list
    xassert(nodeStack.top());
    append2List(nodeStack.top(), topKind, lastNode, lastKind);
  } else if (topKindCat == KC_StringRefMap ||
             topKindCat == KC_StringRefMap ){
    // the top is a map; we had better be a __Name tag
    if (lastKind != XTOK_Name) {
      userError("element of a map that is not a name tag");
    }
  }
  // FIX: in the future there should be an else clause here that
  // files contained objects into their parents even if they are not
  // containers; this would be done by storing the unsatisfied link
  // in the parent object's pointer and also setting the low bit of
  // the pointer to indicate that it is not a real pointer but an
  // identifier to be satisified; look out for failures of number
  // identifiers to be unique due to class embedding (which is why
  // there are letter prefixes on the identifiers).

  // deal with map entries
  if (topKindCat == KC_Name) {
    // file the current object under the map which is one further away
    // in the stack behind the name tag
    //
    // save the map tag
    Name *nameNode = (Name*)nodeStack.pop();
    int nameKind = *kindStack.pop();
    xassert(nameKind == XTOK_Name);
    // check a map is there
    if (nodeStack.isEmpty()) {
      userError("a __Name tag not immediately under a Map");
    }
    void *mapNode = nodeStack.top();
    int mapKind = *kindStack.top();
    if (!(mapNode && (mapKind == KC_StringRefMap || topKindCat == KC_StringSObjDict))) {
      userError("a __Name tag not immediately under a Map");
    }
    // what kind of thing is next on the stack?
    KindCategory mapKindCat;
    bool foundMap = kind2kindCat(mapKind, &mapKindCat);
    // FIX: maybe this should be an assertion
    if (!foundMap) {
      userError("no category found for this map kind");
    }
    insertIntoNameMap(mapNode, mapKind, nameNode->name, lastNode, lastKind);
    // push the name back on the stack so we catch it later
    nodeStack.push(nameNode);
    kindStack.push(new int(nameKind));
  }
}

// state: read the attributes
void ReadXml::readAttributes() {
  while(1) {
    int attr = lexer.getToken();
    switch(attr) {
    default: break;             // go on; assume it is a legal attribute tag
    case 0: userError("unexpected file termination while looking for an attribute name");
    case XTOK_GREATERTHAN:
      return;
    }

    int eq = lexer.getToken();
    switch(eq) {
    default: userError("unexpected token while looking for an '='");
    case 0: userError("unexpected file termination while looking for an '='");
    case XTOK_EQUAL:
      break;                    // go on
    }

    int value = lexer.getToken();
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
      string id0 = parseQuotedString(lexer.currentText());
      if (linkSat.id2obj.isMapped(id0)) {
        userError(stringc << "this id is taken " << id0);
      }
      linkSat.id2obj.add(id0, nodeStack.top());
    }
    // special case the Name node and its one attribute
    else if (*kindStack.top() == XTOK_Name) {
      if (attr != XTOK_name) {
        userError("illegal attribute for Name");
      }
      static_cast<Name*>(nodeStack.top())->name =
        strTable(parseQuotedString(lexer.currentText()));
    }
    // not a built-in attribute or tag
    else {
      registerAttribute(nodeStack.top(), *kindStack.top(), attr, lexer.currentText());
    }
  }
  if (!nodeStack.isEmpty()) {
    userError("missing closing tags at eof");
  }
  // stacks should not be out of sync
  xassert(kindStack.isEmpty());
}
