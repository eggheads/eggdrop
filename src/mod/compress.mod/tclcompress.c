/*
 * tclcompress.c -- part of compress.mod
 *   contains all tcl functions
 * 
 * Written by Fabian Knittel <fknittel@gmx.de>
 * 
 * $Id: tclcompress.c,v 1.2 2000/03/04 20:49:45 fabian Exp $
 */
/* 
 * Copyright (C) 2000  Eggheads
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


static int tcl_compress_to_file STDVAR
{
  int	 mode_num = compress_level;

  BADARGS(3, 4, " src-file target-file ?mode?");
  if (argc == 4)
    mode_num = atoi(argv[3]);
  if (compress_to_file(argv[1], argv[2], mode_num))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_compress_file STDVAR
{
  int	 mode_num = 9;

  BADARGS(2, 3, " file ?mode?");
  if (argc == 3)
    mode_num = atoi(argv[2]);
  if (compress_file(argv[1], mode_num))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_uncompress_to_file STDVAR
{
  BADARGS(3, 3, " src-file target-file");
  if (uncompress_to_file(argv[1], argv[2]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_uncompress_file STDVAR
{
  BADARGS(2, 2, " file");
  if (uncompress_file(argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}


static tcl_cmds my_tcl_cmds[] =
{
  {"compresstofile",	tcl_compress_to_file},
  {"compressfile",	tcl_compress_file},
  {"uncompresstofile",	tcl_uncompress_to_file},
  {"uncompressfile",	tcl_uncompress_file},
  {NULL,		NULL}
};
