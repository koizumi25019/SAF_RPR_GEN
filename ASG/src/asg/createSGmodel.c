//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./createSGmodel.h"
#include "./target.h"
#include "../standard.h"
#include "./opb/opb.h"


//*************************************************************************************************************
//	@name		：　CreateSGmodel
//	@function	：	create the seed generation model
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateSGmodel(
	TARGET* target			  /**< target fault */
)
{
	//char*  lfsr     = (char*)NULL;
	char* minimize = (char*)NULL;


	/** create the tpg model */
	if (CreateTPGmodel(&minimize, target) != TPG_MODEL_OKAY) return SG_MODEL_ERROR;


	/** meke the problem file */
	//makeProbFile(&minimize, target);
	makePBOFile(&minimize, target);


	return SG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		：　CreateTPGmodel
//	@function	：	create the test pattern generation model
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateTPGmodel(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
)
{
	/** create the constraint for faulty-circuit */
	if (CreateConsFC(minimize, target) != TPG_MODEL_OKAY) return TPG_MODEL_OKAY;

	return TPG_MODEL_OKAY;
}









