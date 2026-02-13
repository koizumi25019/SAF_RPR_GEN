//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./createSGmodel.h"
#include "./target.h"
#include "./opb/opb.h"


//*************************************************************************************************************
//	@name		�F�@WriteTPGModel
//	@function	�F	Write the test pattern generation model
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool WriteTPGModel(
	TARGET* target
)
{

	/** create the tpg model */
	if (CreateTPGmodel(target) != TPG_MODEL_OKAY) return W_TPG_MODEL_ERROR;

	/** meke the problem file */
	makePBOFile(target);


	return W_TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@CreateTPGmodel
//	@function	�F	create the test pattern generation model
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool CreateTPGmodel(
	TARGET* target			  /**< target fault */
)
{
	/** create the constraint for faulty-circuit */
	if (CreateConsFC(target) != TPG_MODEL_OKAY) return TPG_MODEL_OKAY;

	return TPG_MODEL_OKAY;
}









