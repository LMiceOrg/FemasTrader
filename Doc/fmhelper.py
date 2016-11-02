#!/usr/bin/env python
# -*- coding: utf-8 -*-

#helper for pdf, md and etc...

from cStringIO import StringIO

from pdfminer.pdfpage import PDFPage
from pdfminer.pdfinterp import PDFResourceManager, PDFPageInterpreter
from pdfminer.converter import TextConverter
from pdfminer.layout import LAParams

import os

curpath = os.path.dirname(os.path.abspath(__file__) )
ftd1name = u"FTD协议1-证监会发布.pdf"
def ftd_parse():
    rsrcmgr = PDFResourceManager()
    retstr = StringIO()

    device = TextConverter(rsrcmgr, retstr, codec='utf-8', laparams=LAParams())
    interpreter = PDFPageInterpreter(rsrcmgr, device)

    with open(os.path.join(curpath, ftd1name)) as fp:
        for page in PDFPage.get_pages(fp, set()):
            interpreter.process_page(page)
        text = retstr.getvalue()
    device.close()
    retstr.close()
    print text

if __name__ == "__main__":
    ftd_parse()
