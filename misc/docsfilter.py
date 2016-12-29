#!/usr/bin/env python

from panflute import *

def action(elem, doc):
	if isinstance(elem, Header):
		elem.level=1

def main(doc=None):
    return run_filter(action, doc=doc)

if __name__ == "__main__":
    main()

