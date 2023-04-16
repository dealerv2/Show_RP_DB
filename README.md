<p>README -- Show_RP_DB  --</p>
<p>Copyright 2023 by J G Morse Richmond BC Canada</p>
<p>[Intro]</p>
Richard Pavlicek has provided to the bridge community a
database of over 10 million solved deals.
By "solved" we mean that attached to each 52 card
deal, are the number of tricks that each seat:
North, East, South, and West, can take in any one of
the five strains: Clubs, Diamonds, Hearts, Spades and
No Trump.
See the file <b>rsd.txt</b> in this repository for RP's
explanation.
<p> </p>
<p>[Source] </p>
<p>This database, in embryonic form, can be downladed
from the <a href="www.rpbridge.net/rput.htm">RP Website</a>
You can also download RP's code to expand and create
the DB, and to selectively get various records from it.
Consult RP's website for more detail.</p>
<p> </p>
<p>RP's database is in a very efficient format taking only
13 bytes to represent the deal, and a further 10 bytes
to convey the tricks results for a total of 23 bytes
per deal.
The resulting database of over 10 million deals is thus
only 230 MiB. </p>
<p> </p>
<p> [This Repository] </p>
<p>RP's code comes as pre-compiled (no source) utilities
that run on the MS-DOS command line.
As such they are not useful to Linux, or other non
MS-DOS users.
<p>This repo, provides C code (source and binary) that
runs on Linux using only the command line.
It should run on any Linux system, and also
on Windows Subsytem for Linux aka WSL.</p>
<p>The <b>show_rpdd</b> binary allows the user to take any number of deals from RP's database and convert them to PBN format,complete with all 20 results for each deal.</p>
<p>This process expands each deal from 23 non-human readable bytes, to 136 human readable characters.</p>
<p> </p>
<p>[Example]</p>
<p>
This is a dump in hex format of the first 23 bytes in RP's DB.<br />
<q> 7252 7a0a e17a e4f9 52e0 41fc 7c49 4949 492b 2a39 3967 67</q>

And this is what the "show_rpdd" utility expands it to (all as single line):<br />
<q>n J873.J42.Q65.KT2 e AT652.A976.AJ82. s Q4.85.KT9.A87643 w K9.KQT3.743.QJ95;06,03,02,04,04,07,09,10,09,09,06,03,02,04,04,07,09,11,09,09</q>


</p><p> </p>
<p>[Uses]</p>
<p>The primary benefit of RP's database is that the deals are random and they are solved.
This allows students of Bridge to study questions such as how much difference does a 9 card fit make over an 8 card fit and so on. RP's website has several examples of how RP himself has used this DB in his own studies.
Calculating all 20 possible results for a bridge hand is, in computer terms, a relatively slow process. It took RP 2 years of computer time to solve all 10 million plus deals.
</p>
<p>
This repo allows non MSDOS users to extract the information in a format that can be processed further. The code can also be adapted to filter the deals based on some criteria.
In fact the DealerV2 program has been modified to do just that.
</p><p>
The <b>show_rpdd</b> utility also has the option of converting RP's 23 byte records to a format that is more compact than the PBN format above;
this is DealerV2's own internal format where each deal is represented by a 52 byte string to which is appended 20 'short integers', which take up 2 bytes.
The total size of the record is thus 92 bytes. This being a binary format file, there is no record separator between the records.
The expanded format allows for a more straight forward analysis of the various hands.
See the function  <B>get_dl52_rec</B> in the source  code for the way to access the binary file.
</p>
<p> </p>
<p>[Running the Program]</p>
<p>Install the <b>show_rpdd</b> binary in a directory in your PATH environment variable (e.g /home/youruser/bin).
Or cd to the directory where the binary is located.
From the command line type: <b>./show_rpdd -h</b> to get a usage message giving the various command line options.
The RP DB file is specified via the <b>-i</b> option.
The pbn formatted output is sent to stdout which you may redirect to a file.</p><p>
The number of deals you want to output is set by the <b>-g</b> option.
The starting position in the DB file is set with the <b>-s</b> option. The option specifies which thousands block to start at, so that you are not getting the same deals all the time as you would if you just read sequentially. E.G. -s5 would start the program at record number 5001.</p>
<p>
The output of the <b>-h</b> option follows:</p>
<p><b>./show_rpdd -h</b></p>
<p>Usage: ./show_rpdd -[g: i: o: s: v: D: hVX: ] [>ofile.pbn] </p>
<p>g={Number of records to get from RP Database -- Default 100}</p>
<p>i={Name of RP DB file. '=' uses the name rpdd.zrd in the default directory}</p>
<p>o={path name of DealerV2 binary deals file; if not given no binary file generated}</p>
<p>s={seed aka the thousand block number to start from; Default = 0}</p>
<p>v={1: print, 0:do not print end of run stats}</p>
<p>D={Debug Verbosity level}</p>
<p>h={show this msg}</p>
<p>V={show Version info}</p>
<p>X={Show the first N records of the Dealer binary file at end of run. Default=0 0=No show; +ve N Number to show. -ve N show all}</p>

<p>[The Database]<p>
Because Github does not allow files more than 100 MB, the database on this site comes as 3 separate files:
<q> rpdd_0-4Million.zrd rpdd_4-8Million.zrd  rpdd_8-10Million.zrd </q>.
The numbers in the file names show the range of deal numbers contained in the files.
If you append these files together when you get them you will have RP's complete database.</br />
Also included are the files <q>rpdd_10First.zrd and rpdd_10Last.zrd</q> which are the first 10 and the last 10 deals.
These two files can be used to test and debug with.</p>


