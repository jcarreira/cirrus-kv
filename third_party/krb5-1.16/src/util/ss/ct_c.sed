
:FIRST
y/	/ /
s/^ *//
s/#.*$//
/; *$/!{
N
y/	/ /
s/\n */ /
bFIRST
}
s/, */, /g
/^$/d
/^unimplemented/ {
	s/^unimplemented [A-Za-z_0-9]*/request ss_unimplemented/
	s/;/, (dont_list, dont_summarize);/
}
/^unknown/ {
	s/^unknown /request ss_unknown, "", /
}
/^command_table /bCMD
/^request /bREQUEST
/^end;/bEND
s/ .*//
s/^/ERROR: unknown keyword: /
=
b
:CMD
s/;$//
p
d
b
:REQUEST
s/^request *//
h
i\
BOR
s/^/sub: /
s/,.*//
p
g
s/^[^,]*, *//
h
/^"/ {
	s/^"//
	s/".*//
	x
	s/^"[^"]*", *//
	x
	b EMITHLP
}
s/[^a-zA-Z0-9].*//
x
s/[a-zA-Z0-9]*, *//
x
:EMITHLP
s/^/hlp: /
p
:CMDLIST
g
/^(/b OPTIONS
/^;/b EOR
/^"/ {
	s/^"//
	s/".*//
	x
	s/^"[^"]*"//
	s/, *//
	x
	b EMITREQ
}
s/[^A-Za-z_0-9].*//
x
s/[A-Za-z_0-9]*//
s/, *//
x
:EMITREQ
s/^/cmd: /
p
b CMDLIST
: OPTIONS
g
s/^(//
h
: OPTLIST
/^)/ b EOR
/^[^A-Za-z_0-9]/ {
	=
	c\
ERROR: parse error in options list
}
s/[^A-Za-z_0-9].*//
x
s/[A-Za-z_0-9]*//
s/, *//
x
s/^/opt: /
p
g
b OPTLIST
: EOR
c\
EOR\

d
b
:END
d
b
