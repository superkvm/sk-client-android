

#ifndef UTLIST_H
#define UTLIST_H

#define UTLIST_VERSION 1.9.1




#ifdef _MSC_VER            
#if _MSC_VER >= 1600 && defined(__cplusplus)  
#define LDECLTYPE(x) decltype(x)
#else                     
#define NO_DECLTYPE
#define LDECLTYPE(x) char*
#endif
#else                      
#define LDECLTYPE(x) __typeof(x)
#endif


#ifdef NO_DECLTYPE
#define _SV(elt,list) _tmp = (char*)(list); {char **_alias = (char**)&(list); *_alias = (elt); }
#define _NEXT(elt,list) ((char*)((list)->next))
#define _NEXTASGN(elt,list,to) { char **_alias = (char**)&((list)->next); *_alias=(char*)(to); }
#define _PREV(elt,list) ((char*)((list)->prev))
#define _PREVASGN(elt,list,to) { char **_alias = (char**)&((list)->prev); *_alias=(char*)(to); }
#define _RS(list) { char **_alias = (char**)&(list); *_alias=_tmp; }
#define _CASTASGN(a,b) { char **_alias = (char**)&(a); *_alias=(char*)(b); }
#else 
#define _SV(elt,list)
#define _NEXT(elt,list) ((elt)->next)
#define _NEXTASGN(elt,list,to) ((elt)->next)=(to)
#define _PREV(elt,list) ((elt)->prev)
#define _PREVASGN(elt,list,to) ((elt)->prev)=(to)
#define _RS(list)
#define _CASTASGN(a,b) (a)=(b)
#endif


#define LL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,NULL); _RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL;                                     \
} while (0)

#define DL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _SV(_ls_e,list); _PREVASGN(_ls_e,list,_ls_tail); _RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _CASTASGN(list->prev, _ls_tail);                                                         \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,NULL); _RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL;                                     \
} while (0)

#define CDL_SORT(list, cmp)                                                                    \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  LDECLTYPE(list) _tmp2;                                                                       \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list);                                                                     \
          if (_NEXT(_ls_q,list) == _ls_oldhead) {                                              \
            _ls_q = NULL;                                                                      \
          } else {                                                                             \
            _ls_q = _NEXT(_ls_q,list);                                                         \
          }                                                                                    \
          _RS(list);                                                                           \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _SV(_ls_e,list); _PREVASGN(_ls_e,list,_ls_tail); _RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _CASTASGN(list->prev,_ls_tail);                                                          \
      _CASTASGN(_tmp2,list);                                                                   \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_tmp2); _RS(list);                           \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL;                                     \
} while (0)


#define LL_PREPEND(head,add)                                                                   \
do {                                                                                           \
  (add)->next = head;                                                                          \
  head = add;                                                                                  \
} while (0)

#define LL_APPEND(head,add)                                                                    \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  (add)->next=NULL;                                                                            \
  if (head) {                                                                                  \
    _tmp = head;                                                                               \
    while (_tmp->next) { _tmp = _tmp->next; }                                                  \
    _tmp->next=(add);                                                                          \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
} while (0)

#define LL_DELETE(head,del)                                                                    \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    _tmp = head;                                                                               \
    while (_tmp->next && (_tmp->next != (del))) {                                              \
      _tmp = _tmp->next;                                                                       \
    }                                                                                          \
    if (_tmp->next) {                                                                          \
      _tmp->next = ((del)->next);                                                              \
    }                                                                                          \
  }                                                                                            \
} while (0)


#define LL_APPEND_VS2008(head,add)                                                             \
do {                                                                                           \
  if (head) {                                                                                  \
    (add)->next = head;                                  \
    while ((add)->next->next) { (add)->next = (add)->next->next; }                             \
    (add)->next->next=(add);                                                                   \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
  (add)->next=NULL;                                                                            \
} while (0)

#define LL_DELETE_VS2008(head,del)                                                             \
do {                                                                                           \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while (head->next && (head->next != (del))) {                                              \
      head = head->next;                                                                       \
    }                                                                                          \
    if (head->next) {                                                                          \
      head->next = ((del)->next);                                                              \
    }                                                                                          \
    {                                                                                          \
      char **_head_alias = (char**)&(head);                                                    \
      *_head_alias = _tmp;                                                                     \
    }                                                                                          \
  }                                                                                            \
} while (0)
#ifdef NO_DECLTYPE
#undef LL_APPEND
#define LL_APPEND LL_APPEND_VS2008
#undef LL_DELETE
#define LL_DELETE LL_DELETE_VS2008
#endif


#define LL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->next)

#define LL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)

#define LL_SEARCH_SCALAR(head,out,field,val)                                                   \
do {                                                                                           \
    LL_FOREACH(head,out) {                                                                     \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define LL_SEARCH(head,out,elt,cmp)                                                            \
do {                                                                                           \
    LL_FOREACH(head,out) {                                                                     \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 


#define DL_PREPEND(head,add)                                                                   \
do {                                                                                           \
 (add)->next = head;                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (head)->prev = (add);                                                                       \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

#define DL_APPEND(head,add)                                                                    \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->prev = (head)->prev;                                                              \
      (head)->prev->next = (add);                                                              \
      (head)->prev = (add);                                                                    \
      (add)->next = NULL;                                                                      \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->prev = (head);                                                                   \
      (head)->next = NULL;                                                                     \
  }                                                                                            \
} while (0);

#define DL_DELETE(head,del)                                                                    \
do {                                                                                           \
  if ((del)->prev == (del)) {                                                                  \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->next->prev = (del)->prev;                                                         \
      (head) = (del)->next;                                                                    \
  } else {                                                                                     \
      (del)->prev->next = (del)->next;                                                         \
      if ((del)->next) {                                                                       \
          (del)->next->prev = (del)->prev;                                                     \
      } else {                                                                                 \
          (head)->prev = (del)->prev;                                                          \
      }                                                                                        \
  }                                                                                            \
} while (0);


#define DL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->next)


#define DL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)


#define DL_SEARCH_SCALAR LL_SEARCH_SCALAR
#define DL_SEARCH LL_SEARCH


#define CDL_PREPEND(head,add)                                                                  \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (add)->next = (head);                                                                       \
   (head)->prev = (add);                                                                       \
   (add)->prev->next = (add);                                                                  \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
   (add)->next = (add);                                                                        \
 }                                                                                             \
(head)=(add);                                                                                  \
} while (0)

#define CDL_DELETE(head,del)                                                                   \
do {                                                                                           \
  if ( ((head)==(del)) && ((head)->next == (head))) {                                          \
      (head) = 0L;                                                                             \
  } else {                                                                                     \
     (del)->next->prev = (del)->prev;                                                          \
     (del)->prev->next = (del)->next;                                                          \
     if ((del) == (head)) (head)=(del)->next;                                                  \
  }                                                                                            \
} while (0);

#define CDL_FOREACH(head,el)                                                                   \
    for(el=head;el;el=(el->next==head ? 0L : el->next)) 

#define CDL_FOREACH_SAFE(head,el,tmp1,tmp2)                                                    \
  for((el)=(head), ((tmp1)=(head)?((head)->prev):NULL);                                        \
      (el) && ((tmp2)=(el)->next, 1);                                                          \
      ((el) = (((el)==(tmp1)) ? 0L : (tmp2))))

#define CDL_SEARCH_SCALAR(head,out,field,val)                                                  \
do {                                                                                           \
    CDL_FOREACH(head,out) {                                                                    \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define CDL_SEARCH(head,out,elt,cmp)                                                           \
do {                                                                                           \
    CDL_FOREACH(head,out) {                                                                    \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 

#endif 

