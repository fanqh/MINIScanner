#include <pio.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <panic.h>
#include "spp_dev_private.h"
#include "spp_dev_parse.h"


void echoRfcommCommand(Sink sink, Task task) {
	
	uint16 size;
	Source source;
	
	if (sink == 0) {
		Panic();
	}
	
	source = StreamSourceFromSink(sink);
	if (source == 0) {
		Panic();
	}
	
	/**
	  check official pbap_client example for parse
	  **/
	size = SourceSize(source);
	while (size > 0) {	
		
		/**
		  this will enqueue message
		  **/
		parseSource(source, task);
		
		size = SourceSize(source);
	}
}



















