unbind dcc - bots *dcc:bots
bind dcc - bots sortedbots

proc sortedbots {hand idx param} {
  putidx $idx [join [lsort [split [*dcc:bots $hand $idx $param]]]]
}

putlog "TCL loaded: sortedbots"
