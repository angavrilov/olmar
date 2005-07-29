// cc_type_xml.h            see license.txt for copyright and terms of use

#include "xml.h"
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer
#include "exc.h"                // xBase


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

void LinkSatisfier::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    if (reader->convertList2SObjList(list, listKind, target)) {
      return;
    }
  }
  THROW(xBase(stringc << "no converter for SObjList type"));
}

void LinkSatisfier::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    if (reader->convertList2ObjList(list, listKind, target)) {
      return;
    }
  }
  THROW(xBase(stringc << "no converter for ObjList type"));
}

void LinkSatisfier::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    if (reader->convertNameMap2StringRefMap(map, mapKind, target)) {
      return;
    }
  }
  THROW(xBase(stringc << "no converter for name map type"));
}

void LinkSatisfier::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
  FOREACH_ASTLIST_NC(ReadXml, readers, iter) {
    ReadXml *reader = iter.data();
    if (reader->convertNameMap2StringSObjDict(map, mapKind, target)) {
      return;
    }
  }
  THROW(xBase(stringc << "no converter for name map type"));
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
  satisfyLinks_Nodes();
  satisfyLinks_Lists();
  satisfyLinks_Maps();
//    satisfyLinks_Bidirectional();
}

void LinkSatisfier::satisfyLinks_Nodes() {
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
}

void LinkSatisfier::satisfyLinks_Lists() {
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
      // the steal() has deleted the voidlist of the ASTList and it
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
      convertList2SObjList(obj, ul->kind, ul->ptr);
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
      convertList2ObjList(obj, ul->kind, ul->ptr);
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

void LinkSatisfier::satisfyLinks_Maps() {
  FOREACH_ASTLIST(UnsatLink, unsatLinks_NameMap, iter) {
    UnsatLink const *ul = iter.data();
    // NOTE: I rely on the fact that all StringRefMap-s just contain
    // pointers; otherwise this cast would cause problems; Note that I
    // have to use char instead of void because you can't delete a
    // pointer to void; see the note in the if below.
    StringRefMap<char> *obj = reinterpret_cast<StringRefMap<char>*>(id2obj.queryif(ul->id));
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
      xfailure("illegal name map kind");
      break;

    case KC_StringRefMap: {
      // FIX: this would be way more efficient if there were a
      // PtrMap::steal() method: I wouldn't need this convert call.
      convertNameMap2StringRefMap(obj, ul->kind, ul->ptr);
      break;
    }

    case KC_StringSObjDict: {
      convertNameMap2StringSObjDict(obj, ul->kind, ul->ptr);
      break;
    }

    }
  }
}

//  void LinkSatisfier::satisfyLinks_Bidirectional() {
//    FOREACH_ASTLIST(UnsatBiLink, unsatBiLinks, iter) {
//      UnsatBiLink const *ul = iter.data();
//      void **from = (void**)id2obj.queryif(ul->from);
//      // NOTE: these are different from unidirectional links: you really
//      // shouldn't make one unless both parties can be found.
//      if (!from) {
//        userError("Unsatisfied bidirectional link: 'from' not found");
//      }
//      void *to = id2obj.queryif(ul->to);
//      if (!to) {
//        userError("Unsatisfied bidirectional link: 'to' not found");
//      }
//      *(from) = to;
//    }
//  }

void LinkSatisfier::userError(char const *msg) {
  THROW(xBase(msg));
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
  // construct the tag object on the stack
  void *topTemp;
  bool sawOpenTag = true;
  switch(tag) {
  default:
    topTemp = ctorNodeFromTag(tag);
    break;
  // Slash: start of a close tag
  case XTOK_SLASH:
    sawOpenTag = false;
    break;
  // Item: a list element
  case XTOK___Item:
    topTemp = new Item();
    break;
  // Name: a map element
  case XTOK___Name:
    topTemp = new Name();
    break;
//    // Special case the <__Link/> tag
//    case XTOK___Link:
//      topTemp = new UnsatBiLink();
//      break;
  }
  if (sawOpenTag) {
    // NOTE: even if it is a stand-alone tag that will not stay on the
    // stack, we still have to put it here as readAttributes()
    // attaches attributes to the node on the top of the stack (my
    // parser is some sort of stack machine).
    xassert(topTemp);
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));

    // read the attributes
    bool sawContainerTag = readAttributes();

    // if it is a container tag, we just leave it on the stack
    if (sawContainerTag) {
      // state: we saw a container tag
      return;
    }

    // state: we saw a stand-alone tag.  FIX: I suppose I should
    // generalize this, but for now there is only one stand-alone tag
//      if (!tag == XTOK___Link) {
    userError("illegal stand-alone tag");
//      }
//      UnsatBiLink *ul = (UnsatBiLink*) topTemp;
//      if (!ul->from) {
//        userError("missing 'from' field on __Link tag");
//      }
//      if (!ul->to) {
//        userError("missing 'to' field on __Link tag");
//      }
//      linkSat.unsatBiLinks.append(ul);
//      // we don't need it on the stack anymore
//      nodeStack.pop();
//      kindStack.pop();            // FIX: delete the return?
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
  lastNode = nodeStack.pop();
  lastKind = *kindStack.pop();
  if (lastKind != closeTag) {
    userError(stringc << "close tag " << lexer.tokenKindDesc(closeTag)
              << " does not match open tag " << lexer.tokenKindDesc(lastKind));
  }

  // state: read the '>' after a close tag
  int closeGreaterThan = lexer.getToken();
  switch(closeGreaterThan) {
  default: userError("unexpected token while looking for '>' of a close tag");
  case 0: userError("unexpected file termination while looking for '>' of a close tag");
  case XTOK_GREATERTHAN:
    break;
  }

  // deal with list entries
  if (closeTag == XTOK___Item) {
    // save the item tag
    Item *itemNode = (Item*)lastNode;
    int itemKind = lastKind;
    xassert(itemKind == XTOK___Item);

    // what kind of thing is next on the stack?
    if (nodeStack.isEmpty()) {
      userError("a __Item tag not immediately under a List");
    }
    void *listNode = nodeStack.top();
    int listKind = *kindStack.top();
    KindCategory listKindCat;
    bool foundList = kind2kindCat(listKind, &listKindCat);
    // FIX: maybe this should be an assertion
    if (!foundList) {
      userError("no category found for this list kind");
    }
    if (!(listNode &&
          (listKindCat == KC_ASTList
           || listKindCat == KC_FakeList
           || listKindCat == KC_ObjList
           || listKindCat == KC_SObjList)
        )) {
      userError("a __Item tag not immediately under a List");
    }

    // find the Node pointed to by the item; it should have been seen
    // by now
    if (!itemNode->item) {
      userError("no 'item' field for this __Item tag");
    }
    void *pointedToItem = linkSat.id2obj.queryif(itemNode->item);
    if (!pointedToItem) {
      userError("no Node pointed to by Item");
    }
    append2List(listNode, listKind, pointedToItem);
  }

  // deal with map entries
  else if (closeTag == XTOK___Name) {
    // save the name tag
    Name *nameNode = (Name*)lastNode;
    int nameKind = lastKind;
    xassert(nameKind == XTOK___Name);

    // what kind of thing is next on the stack?
    if (nodeStack.isEmpty()) {
      userError("a __Name tag not immediately under a Map");
    }
    void *mapNode = nodeStack.top();
    int mapKind = *kindStack.top();
    KindCategory mapKindCat;
    bool foundMap = kind2kindCat(mapKind, &mapKindCat);
    // FIX: maybe this should be an assertion
    if (!foundMap) {
      userError("no category found for this map kind");
    }
    if (!(mapNode && (mapKindCat == KC_StringRefMap || mapKindCat == KC_StringSObjDict))) {
      userError("a __Name tag not immediately under a Map");
    }

    // find the Node pointed to by the item; it should have been seen
    // by now
    void *pointedToItem = linkSat.id2obj.queryif(nameNode->item);
    if (!pointedToItem) {
      userError("no Node pointed to by Name");
    }
    insertIntoNameMap(mapNode, mapKind, nameNode->name, pointedToItem);
  }

  // otherwise we are a normal node; just pop it off; no further
  // processing is required
  else {
  }
}

// state: read the attributes
bool ReadXml::readAttributes() {
  while(1) {
    int attr = lexer.getToken();
    switch(attr) {
    default: break;             // go on; assume it is a legal attribute tag
    case 0: userError("unexpected file termination while looking for an attribute name");
    case XTOK_GREATERTHAN:
      return true;              // container tag
    case XTOK_SLASH:
      attr = lexer.getToken();  // eat the '>' token
      if (attr!=XTOK_GREATERTHAN) {
        userError("expected '>' after '/' that terminates a stand-alone tag");
      }
      return false;             // non-container tag
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
    // special case the __Name node and its one attribute
    else if (*kindStack.top() == XTOK___Name) {
      switch(attr) {
      default:
        userError("illegal attribute for __Name");
        break;
      case XTOK_name:
        static_cast<Name*>(nodeStack.top())->name =
          strTable(parseQuotedString(lexer.currentText()));
        break;
      case XTOK_item:
        static_cast<Name*>(nodeStack.top())->item =
          strTable(parseQuotedString(lexer.currentText()));
        break;
      }
    }
    // special case the __Item node and its one attribute
    else if (*kindStack.top() == XTOK___Item) {
      switch(attr) {
      default:
        userError("illegal attribute for __Item");
        break;
      case XTOK_item:
        static_cast<Item*>(nodeStack.top())->item =
          strTable(parseQuotedString(lexer.currentText()));
        break;
      }
    }
//      // special case the __Link node and its attributes
//      else if (*kindStack.top() == XTOK___Link) {
//        switch(attr) {
//        default:
//          userError("illegal attribute for __Link");
//          break;
//        case XTOK_from:
//          static_cast<UnsatBiLink*>(nodeStack.top())->from =
//            strTable(parseQuotedString(lexer.currentText()));
//          break;
//        case XTOK_to:
//          static_cast<UnsatBiLink*>(nodeStack.top())->to =
//            strTable(parseQuotedString(lexer.currentText()));
//          break;
//        }
//      }
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

bool ReadXml::kind2kindCat(int kind, KindCategory *kindCat) {
  xassert(kind != -1);          // this means you shouldn't be asking

  switch(kind) {
  // special list element __Item
  case XTOK___Item: *kindCat = KC_Item; return true; break;
  // special map element __Name
  case XTOK___Name: *kindCat = KC_Name; return true; break;
  default:
    // fallthrough the switch
    break;
  }

  return kind2kindCat0(kind, kindCat);
}
