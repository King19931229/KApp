# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000,2008 Oracle.  All rights reserved.
#
# $Id: test090.tcl,v 12.6 2008/01/08 20:58:53 bostic Exp $
#
# TEST	test090
# TEST	Test for functionality near the end of the queue using test001.
proc test090 { method {nentries 10000} {tnum "090"} args} {
	if { [is_queueext $method ] == 0 } {
		puts "Skipping test$tnum for $method."
		return;
	}
	eval {test001 $method $nentries 4294967000 0 $tnum} $args
}
