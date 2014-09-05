----------------------------------------------------------------------
IDA FLAIR helpers for making .SIG files from Borland sources by servil
----------------------------------------------------------------------

Intro: when making pattern files out of Borland runtimes or object files,
resulting pattern file doesnot contain full name notation but only localized
name lacking module prefix. This is ok for basic disassembly orientation but
for more demanding purposes the names may look invalid. For these cases i wrote
two tools to fix names as much as possible.

1st tool is needs perl interpreter to run (http://www.activestate.com/). Use
this script only for converting from borland objfiles. The tool requires
borland TDUMP util accessible within the PATH scope. (Afaik TDUMP is delivered
with almost all Borland development products). The method is very simple only
module name is prefixed public symbols but no mangling is cared. Also only
completness of public names is assured but doesnot care about any referred
names, This may be or may be not substantial for signatures application (for
this case I wrote second tool, described later). TDUMP is necessary for
acquiring safely module name (the script may work even without TDUMP but I
didnot test, anyway going with TDUMP is better by reason of module name may
theoretically differ from objname). Syntax for running the script is

  omf2pat [--outpath <targetdir>] [--recursive] <objspec> [<objspec> [...]]

If targetdir is not stated, patternfiles will be saved to current dir,
<objspec>`s may contain wildcards (e.g. *.obj).
The script produces post-processed pattern files already having public names
correct such that the pattern file can be further processed usual way.

2nd tool is for completing any names used in finished pattern file(s) with
application lookup method from known names base. As for name base I decided for
Borland dynamic runtimes as they are widely available and provide complete
names notation, insofar that at least one *.bpl module is required, (ofcourse
the more the better, I recommend using comlpete set of modules for according
VCL build). This method is not 100% but is able to complete majority of
localized names (let's say about 70-90%). For failed lookups the tool saves
logfile to current directory with detailed information about unmatched names.
Calling syntax of the tool is

  omfpat <patspec> <runtime1.bpl> [<runtime2.bpl> [...]]

<patspec> argument is required and may contain wildcards (e.g. *.pat)
All runtimes are stated after patspec, wildcards are not allowed (use only
corresponding runtimes for respective VCL version).
All pattern files processed by omfpat can be further processed the usual way.

The tools can be chained for max effect, a sample of usage for Delphi2005:

  omf2pat *.obj
  omfpat *.pat adortl90.bpl bdertl90.bpl ...... websnap90.bpl xmlrtl90.bpl
  sigmake ...

however omf2pat may expand public names not fully on arglist mangling so that
they can't be completed fully neither by omfpat any later, thus better call
the tools in this order:

  for %i in (*.obj) do plb "%i" "%~Ni.pat"
  omfpat *.pat adortl90.bpl bdertl90.bpl ...... websnap90.bpl xmlrtl90.bpl
  omf2pat *.pat
  copy patfilefirst.pat+...+patfilelast.pat Delphi9.pat
  sigmake ...

omf2pat can accept as input files pattern ready made pattern files also, in
that case plb stage is skipped and only obj files are used to retrieve module
name (obj files must have same base name and same directory)

Any bugs or suggestions to improve pm me at exetools forum or to
servil@gmx.net.
