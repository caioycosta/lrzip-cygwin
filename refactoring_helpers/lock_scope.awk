## usage: ./preprocess.sh file.c | awk -f refactoring_helpers/lock_scope.awk

##
## checks if locks are acqred and released on the same scope
## bugs are aplenty
## maintain a stack of scopes and counts how many times each mutex 
##  is locked/unlocked. if it locked less times than unlocked or vice versa
##  an error message is shown
##
## not meant to be perfet, justfor this specific use case
## best i could do without having to write a parser
##
## for use on rzip.c and stream.c
## must pipe the code through preprocess.sh
## in order to always have newlines before "{"
##
## this: 
## if ()
## {
## }
## else
## {
## }
##
## NOT this
## if () {
## } else {
## }
##
##
##



BEGIN {
 bk_stack = 0
}

##
## just a workaround for incorrect usage
##
function inde(a,b)
{
return index(b,a)
}

function lock_found()
{
        print "lock found at :" FNR
        match($0, /_mutex\(.*\)/, rgx_out)
        locks[bk_stack][rgx_out[0]]++

}

function unlock_found()
{
    print "unlock found at:" FNR
        match($0, /_mutex\(.*\)/, rgx_out)
        locks[bk_stack][rgx_out[0]]--

}

function open_scope()
{
   bk_stack++
   identation[bk_stack] = inde("{",$0)
}

function close_scope()
{
   chk_lck(locks,bk_stack,FNR)

           if (inde("}",$0) != identation[bk_stack])
               print "warning. non matching block identation at :" FNR

        delete identation[bk_stack]
        bk_stack--

}

##
## checks if all locks were released in the current scope
## called at scope end (closing bracket)
##
function chk_lck(lckarr,stkvar,location)
{
	if (stkvar in lckarr) {

        for (lck in lckarr[stkvar]) {
                if (lckarr[stkvar][lck] != 0)
                        print "banana lock " lck " at: " location

        }
        delete lckarr[stkvar]
}
}

{

##
## lots of edge cases
##

if (inde("{",$0) > 0 && inde("}",$0) > 0) {
		print "brackets on same line at :" FNR
		exit 
}

##
##
if ((inde("{",$0) > 0 || inde("}",$0) > 0) && inde("lock_mutex",$0) > 0) {
	        print "brackets and mutex on same line at :" FNR
		exit
}

##
## scope start
##
if (inde("{",$0) > 0) {
open_scope()
}

##
## increases or decreases counter 
## depending on which mutex is being locked
##
if (inde("unlock_mutex(",$0) > 0) {
	unlock_found()
} else if (inde("lock_mutex(",$0) > 0) {
	lock_found()
}

##
## scope end
##
if (inde("}",$0) > 0) {
close_scope()
}


}

##
## last check at file end
##
END {
if (bk_stack != 0) 
	print "brackets non matching! " bk_stack

chk_lck(locks,bk_stack,"file end")


}
