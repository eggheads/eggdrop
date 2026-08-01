#!/usr/bin/env python

from panflute import *

def upper(elem, doc):
    if isinstance(elem, Str):
        elem.text = elem.text.upper()

def action(elem, doc):
    if isinstance(elem, Header):
        if elem.level == 2:
            return elem.walk(upper)

def main(doc=None):
    return run_filter(action, doc=doc)

if __name__ == "__main__":
    main()
