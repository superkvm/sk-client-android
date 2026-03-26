



#if 1	
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		
	#endif
	#undef USE_LOGALL
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG		
#endif

#include <stdlib.h>

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"


IPipeline::IPipeline(const size_t &_default_frame_size)
:	state(PIPELINE_STATE_UNINITIALIZED),
	mIsRunning(false),
	default_frame_size(_default_frame_size),
	next_pipeline(NULL)
{
	ENTER();

	EXIT();
}


IPipeline::~IPipeline() {
	ENTER();

	EXIT();
}


const bool IPipeline::isRunning() const { return (mIsRunning); };


const pipeline_state_t IPipeline::getState() const { return (state); };


void IPipeline::setState(const pipeline_state_t &new_state) { state = new_state; }


int IPipeline::setPipeline(IPipeline *pipeline) {
	ENTER();

	Mutex::Autolock lock(pipeline_mutex);

	
	next_pipeline = pipeline;

	RETURN(0, int);
}


int IPipeline::chain_frame(uvc_frame_t *frame) {
	ENTER();

	int result = -1;
	Mutex::Autolock lock(pipeline_mutex);

	if (next_pipeline) {
		next_pipeline->queueFrame(frame);
		result = 0;
	}

	RETURN(result, int);
}
